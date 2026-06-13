#include "ClickController.h"

#include <math.h>
#include <stdlib.h>

#include <esp_system.h>
#include <esp_timer.h>

namespace {

float gaussianRandom(float mu, float sigma) {
  float u1;
  float u2;
  do {
    u1 = static_cast<float>(rand()) / RAND_MAX;
  } while (u1 <= 1e-7f);
  u2 = static_cast<float>(rand()) / RAND_MAX;
  const float z = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
  return mu + sigma * z;
}

}  // namespace

ClickController::ClickController(int tapPin, int activeLevel, int idleLevel)
    : tapPin_(tapPin), activeLevel_(activeLevel), idleLevel_(idleLevel) {}

void ClickController::begin() {
  pinMode(tapPin_, OUTPUT);
  srand(static_cast<unsigned int>(esp_random()));
  releaseTap();
}

void ClickController::setGrade(uint8_t nextGrade, unsigned long nowMs) {
  if (nextGrade > AppConfig::kMaxGrade) {
    nextGrade = AppConfig::kMaxGrade;
  }

  grade_ = nextGrade;
  rushMode_ = false;
  rushPhase_ = RUSH_IDLE;
  rushCycleActive_ = false;
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

void ClickController::startRush(unsigned long nowMs) {
  setGrade(0, nowMs);
  rushMode_ = true;
  rushPhase_ = RUSH_BURST;
  rushStartTimeUs_ = esp_timer_get_time();
  adaptiveStep_ = AdaptiveStep::Calculate;
}

void ClickController::stopRush(unsigned long nowMs) {
  setGrade(0, nowMs);
}

void ClickController::restart(unsigned long nowMs) {
  setGrade(grade_, nowMs);
}

void ClickController::update(unsigned long nowMs) {
  if (rushMode_) {
    updateRush();
    return;
  }

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
  if (rushMode_) {
    return true;
  }
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

bool ClickController::isRushMode() const {
  return rushMode_;
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

  const unsigned long elapsedUs = static_cast<unsigned long>(esp_timer_get_time()) - adaptiveStartedUs_;
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
    adaptiveStartedUs_ = static_cast<unsigned long>(esp_timer_get_time());
    lastAdaptiveRun_ = {0, 0, 0.0f};
    adaptiveStep_ = AdaptiveStep::Calculate;
  }

  const unsigned long nowUs = static_cast<unsigned long>(esp_timer_get_time());
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
    const float pauseMeanUs =
        (static_cast<float>(AppConfig::kAdaptiveLongPauseMinUs) + AppConfig::kAdaptiveLongPauseMaxUs) / 2.0f;
    const float pauseSigmaUs =
        (static_cast<float>(AppConfig::kAdaptiveLongPauseMaxUs) - AppConfig::kAdaptiveLongPauseMinUs) / 6.0f;
    totalCycleUs = static_cast<unsigned long>(
        max<long>(AppConfig::kAdaptiveLongPauseMinUs, lroundf(gaussianRandom(pauseMeanUs, pauseSigmaUs))));
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

void ClickController::updateRush() {
  const int64_t nowUs = esp_timer_get_time();
  const int64_t elapsedMs = (nowUs - rushStartTimeUs_) / 1000;

  if (elapsedMs >= AUTO_STOP_MS) {
    stopRush(static_cast<unsigned long>(nowUs / 1000));
    return;
  }

  if (rushPhase_ == RUSH_BURST && elapsedMs >= BURST_DURATION_MS) {
    rushPhase_ = RUSH_HOLD;
  }

  if (!rushCycleActive_) {
    startRushCycle(nowUs);
    return;
  }

  if (tapping_ && nowUs >= rushTapEndsUs_) {
    releaseTap();
    adaptiveStep_ = AdaptiveStep::Release;
  }

  if (!tapping_ && nowUs >= rushCycleEndsUs_) {
    rushCycleActive_ = false;
    adaptiveStep_ = AdaptiveStep::Calculate;
  }
}

void ClickController::startRushCycle(int64_t nowUs) {
  const bool holdPhase = rushPhase_ == RUSH_HOLD;
  const int cycleMinMs = holdPhase ? HOLD_CYCLE_MIN_MS : BURST_CYCLE_MIN_MS;
  const int cycleMaxMs = holdPhase ? HOLD_CYCLE_MAX_MS : BURST_CYCLE_MAX_MS;
  const float pressMeanMs = holdPhase ? HOLD_PRESS_MEAN_MS : BURST_PRESS_MEAN_MS;
  const float pressSigmaMs = holdPhase ? HOLD_PRESS_SIGMA_MS : BURST_PRESS_SIGMA_MS;

  const int totalMs = cycleMinMs + rand() % (cycleMaxMs - cycleMinMs + 1);
  int pressMs = max(static_cast<int>(lroundf(gaussianRandom(pressMeanMs, pressSigmaMs))), 0);

  if (pressMs > totalMs - MIN_RELEASE_MS) {
    pressMs = totalMs - MIN_RELEASE_MS;
  }

  rushCycleActive_ = true;
  rushTapEndsUs_ = nowUs + static_cast<int64_t>(pressMs) * 1000;
  rushCycleEndsUs_ = nowUs + static_cast<int64_t>(totalMs) * 1000;
  if (pressMs > 0) {
    startTap();
    adaptiveStep_ = AdaptiveStep::Press;
  } else {
    releaseTap();
    adaptiveStep_ = AdaptiveStep::Release;
  }
}

float ClickController::randomUnit() const {
  return static_cast<float>(rand()) / RAND_MAX;
}

float ClickController::randomGaussian() const {
  return gaussianRandom(0.0f, 1.0f);
}

void ClickController::releaseTap() {
  digitalWrite(tapPin_, idleLevel_);
  tapping_ = false;
}

void ClickController::startTap() {
  digitalWrite(tapPin_, activeLevel_);
  tapping_ = true;
}
