#include "ClickController.h"

#include <math.h>

#include <esp_system.h>

ClickController::ClickController(int tapPin, int activeLevel, int idleLevel)
    : tapPin_(tapPin), activeLevel_(activeLevel), idleLevel_(idleLevel) {}

void ClickController::begin() {
  pinMode(tapPin_, OUTPUT);
  randomSeed(static_cast<unsigned long>(esp_random()));
  releaseTap();
}

void ClickController::setGrade(uint8_t nextGrade, unsigned long nowMs) {
  if (nextGrade > AppConfig::kMaxGrade) {
    nextGrade = AppConfig::kMaxGrade;
  }

  grade_ = nextGrade;
  releaseTap();
  phaseStartedMs_ = nowMs;
  adaptiveRunStarted_ = false;
  adaptiveRunLocked_ = false;
  adaptiveCompletionPending_ = false;
  adaptiveStartedUs_ = 0;
  adaptiveCycleActive_ = false;
  adaptiveTapEndsUs_ = 0;
  adaptiveCycleEndsUs_ = 0;
  adaptiveStep_ = (nextGrade == AppConfig::kAdaptiveGrade) ? AdaptiveStep::Calculate : AdaptiveStep::Idle;
}

void ClickController::restart(unsigned long nowMs) {
  setGrade(grade_, nowMs);
}

void ClickController::update(unsigned long nowMs) {
  if (grade_ == 0) {
    releaseTap();
    return;
  }

  if (grade_ == AppConfig::kAdaptiveGrade) {
    updateAdaptiveRun();
    return;
  }

  const ClickProfile& profile = currentProfile();

  if (!tapping_ && nowMs - phaseStartedMs_ >= profile.restMs) {
    startTap();
    phaseStartedMs_ = nowMs;
    return;
  }

  if (tapping_ && nowMs - phaseStartedMs_ >= profile.tapMs) {
    releaseTap();
    phaseStartedMs_ = nowMs;
  }
}

uint8_t ClickController::grade() const {
  return grade_;
}

bool ClickController::isRunning() const {
  if (grade_ == AppConfig::kAdaptiveGrade) {
    return !adaptiveRunLocked_;
  }
  return grade_ > 0;
}

bool ClickController::isTapping() const {
  return tapping_;
}

bool ClickController::isAdaptiveMode() const {
  return grade_ == AppConfig::kAdaptiveGrade;
}

bool ClickController::isAdaptiveComplete() const {
  return grade_ == AppConfig::kAdaptiveGrade && adaptiveRunLocked_;
}

const ClickProfile& ClickController::currentProfile() const {
  if (fixedProfileOverrideEnabled_ && grade_ > 0 && grade_ != AppConfig::kAdaptiveGrade) {
    return fixedProfileOverride_;
  }
  return AppConfig::profileForGrade(grade_);
}

void ClickController::setFixedProfileOverride(uint16_t tapMs, uint16_t restMs) {
  fixedProfileOverride_ = {
      tapMs,
      restMs,
      static_cast<uint8_t>(1000U / max<uint16_t>(1, tapMs + restMs)),
  };
  fixedProfileOverrideEnabled_ = true;
}

void ClickController::clearFixedProfileOverride() {
  fixedProfileOverrideEnabled_ = false;
}

bool ClickController::hasFixedProfileOverride() const {
  return fixedProfileOverrideEnabled_;
}

ClickProfile ClickController::fixedProfileOverride() const {
  return fixedProfileOverride_;
}

bool ClickController::consumeAdaptiveCompletion() {
  const bool pending = adaptiveCompletionPending_;
  adaptiveCompletionPending_ = false;
  return pending;
}

ClickController::AdaptiveRunStats ClickController::lastAdaptiveRun() const {
  return lastAdaptiveRun_;
}

unsigned long ClickController::adaptiveRemainingMs() const {
  if (grade_ != AppConfig::kAdaptiveGrade) {
    return 0;
  }

  if (adaptiveRunLocked_) {
    return 0;
  }

  if (!adaptiveRunStarted_) {
    return AppConfig::kAdaptiveRunDurationUs / 1000UL;
  }

  const unsigned long elapsedUs = micros() - adaptiveStartedUs_;
  if (elapsedUs >= AppConfig::kAdaptiveRunDurationUs) {
    return 0;
  }

  return (AppConfig::kAdaptiveRunDurationUs - elapsedUs + 999UL) / 1000UL;
}

AdaptiveStep ClickController::adaptiveStep() const {
  return adaptiveStep_;
}

void ClickController::updateAdaptiveRun() {
  if (adaptiveRunLocked_) {
    releaseTap();
    adaptiveStep_ = AdaptiveStep::Complete;
    return;
  }

  if (!adaptiveRunStarted_) {
    adaptiveRunStarted_ = true;
    adaptiveStartedUs_ = micros();
    lastAdaptiveRun_ = {0, 0, 0.0f};
    adaptiveStep_ = AdaptiveStep::Calculate;
  }

  const unsigned long nowUs = micros();
  const unsigned long elapsedUs = nowUs - adaptiveStartedUs_;
  if (elapsedUs >= AppConfig::kAdaptiveRunDurationUs) {
    completeAdaptiveRun(nowUs);
    return;
  }

  if (!adaptiveCycleActive_) {
    startAdaptiveCycle(nowUs);
    return;
  }

  if (tapping_ && nowUs >= adaptiveTapEndsUs_) {
    releaseTap();
    adaptiveStep_ = AdaptiveStep::Release;
  }

  if (!tapping_ && adaptiveCycleActive_ && nowUs >= adaptiveCycleEndsUs_) {
    adaptiveCycleActive_ = false;
    lastAdaptiveRun_.clickCount++;
    adaptiveStep_ = AdaptiveStep::Calculate;

    if (nowUs - adaptiveStartedUs_ >= AppConfig::kAdaptiveRunDurationUs) {
      completeAdaptiveRun(nowUs);
    }
  }
}

void ClickController::startAdaptiveCycle(unsigned long nowUs) {
  const AdaptiveCycle cycle = buildAdaptiveCycle(nowUs - adaptiveStartedUs_);
  adaptiveCycleActive_ = true;
  adaptiveTapEndsUs_ = nowUs + cycle.tapUs;
  adaptiveCycleEndsUs_ = nowUs + cycle.totalCycleUs;
  startTap();
  adaptiveStep_ = AdaptiveStep::Press;
}

ClickController::AdaptiveCycle ClickController::buildAdaptiveCycle(unsigned long elapsedUs) const {
  const unsigned long burstDurationUs =
      static_cast<unsigned long>(AppConfig::kAdaptiveRunDurationUs * AppConfig::kAdaptiveBurstRatio);

  float fatigueRatio = 0.0f;
  if (elapsedUs > burstDurationUs) {
    const float fatigueElapsedUs = static_cast<float>(elapsedUs - burstDurationUs);
    const float fatigueWindowUs = static_cast<float>(AppConfig::kAdaptiveRunDurationUs - burstDurationUs);
    fatigueRatio = fatigueElapsedUs / fatigueWindowUs;
    if (fatigueRatio > 1.0f) {
      fatigueRatio = 1.0f;
    }
  }

  const float fatigueBaseCycleUs = static_cast<float>(AppConfig::kAdaptiveBaseCycleUs) *
                                   (1.0f + AppConfig::kAdaptiveSlowdownRatio * fatigueRatio);

  unsigned long totalCycleUs = 0;
  if (randomUnit() < AppConfig::kAdaptiveLongPauseChance) {
    totalCycleUs =
        static_cast<unsigned long>(random(AppConfig::kAdaptiveLongPauseMinUs, AppConfig::kAdaptiveLongPauseMaxUs + 1));
  } else {
    const long jitteredCycleUs =
        lroundf(fatigueBaseCycleUs + randomGaussian() * AppConfig::kAdaptiveCycleJitterStdUs);
    totalCycleUs = static_cast<unsigned long>(max<long>(AppConfig::kAdaptiveMinCycleUs, jitteredCycleUs));
  }

  long tapUs =
      lroundf(static_cast<float>(AppConfig::kAdaptiveMeanTapUs) + randomGaussian() * AppConfig::kAdaptiveTapJitterStdUs);
  tapUs = max<long>(AppConfig::kAdaptiveMinTapUs, tapUs);

  if (static_cast<long>(totalCycleUs) - tapUs < AppConfig::kAdaptiveMinReleaseUs) {
    tapUs = static_cast<long>(totalCycleUs) - AppConfig::kAdaptiveMinReleaseUs;
  }

  if (tapUs < AppConfig::kAdaptiveMinTapUs) {
    tapUs = AppConfig::kAdaptiveMinTapUs;
  }

  return {
      totalCycleUs,
      static_cast<unsigned long>(tapUs),
      totalCycleUs - static_cast<unsigned long>(tapUs),
  };
}

void ClickController::completeAdaptiveRun(unsigned long completedAtUs) {
  releaseTap();
  adaptiveRunStarted_ = false;
  adaptiveRunLocked_ = true;
  adaptiveCompletionPending_ = true;
  adaptiveCycleActive_ = false;
  adaptiveStep_ = AdaptiveStep::Complete;
  lastAdaptiveRun_.elapsedUs = completedAtUs - adaptiveStartedUs_;
  if (lastAdaptiveRun_.elapsedUs > 0) {
    lastAdaptiveRun_.averageCps =
        static_cast<float>(lastAdaptiveRun_.clickCount) / (static_cast<float>(lastAdaptiveRun_.elapsedUs) / 1000000.0f);
  } else {
    lastAdaptiveRun_.averageCps = 0.0f;
  }
}

float ClickController::randomUnit() const {
  return static_cast<float>(random(0L, 10001L)) / 10000.0f;
}

float ClickController::randomGaussian() const {
  float sum = 0.0f;
  for (int i = 0; i < 6; ++i) {
    sum += randomUnit();
  }
  return (sum - 3.0f) * 1.41421356f;
}

void ClickController::releaseTap() {
  digitalWrite(tapPin_, idleLevel_);
  tapping_ = false;
}

void ClickController::startTap() {
  digitalWrite(tapPin_, activeLevel_);
  tapping_ = true;
}
