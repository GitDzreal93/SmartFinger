#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class ClickController {
 public:
  struct AdaptiveRunStats {
    unsigned long elapsedUs;
    uint16_t clickCount;
    float averageCps;
  };

  ClickController(int tapPin, int activeLevel, int idleLevel);

  void begin();
  void setGrade(uint8_t nextGrade, unsigned long nowMs);
  void restart(unsigned long nowMs);
  void update(unsigned long nowMs);
  void setFixedProfileOverride(uint16_t tapMs, uint16_t restMs);
  void clearFixedProfileOverride();

  uint8_t grade() const;
  bool isRunning() const;
  bool isTapping() const;
  bool isAdaptiveMode() const;
  bool isAdaptiveComplete() const;
  const ClickProfile& currentProfile() const;
  bool hasFixedProfileOverride() const;
  ClickProfile fixedProfileOverride() const;
  bool consumeAdaptiveCompletion();
  AdaptiveRunStats lastAdaptiveRun() const;
  unsigned long adaptiveRemainingMs() const;
  AdaptiveStep adaptiveStep() const;

 private:
  struct AdaptiveCycle {
    unsigned long totalCycleUs;
    unsigned long tapUs;
    unsigned long releaseUs;
  };

  void updateAdaptiveRun();
  void startAdaptiveCycle(unsigned long nowUs);
  AdaptiveCycle buildAdaptiveCycle(unsigned long elapsedUs) const;
  void completeAdaptiveRun(unsigned long completedAtUs);
  float randomUnit() const;
  float randomGaussian() const;

  void releaseTap();
  void startTap();

  int tapPin_;
  int activeLevel_;
  int idleLevel_;
  uint8_t grade_ = 0;
  bool tapping_ = false;
  unsigned long phaseStartedMs_ = 0;
  bool fixedProfileOverrideEnabled_ = false;
  ClickProfile fixedProfileOverride_ = {45, 155, 5};
  bool adaptiveRunStarted_ = false;
  bool adaptiveRunLocked_ = false;
  bool adaptiveCompletionPending_ = false;
  unsigned long adaptiveStartedUs_ = 0;
  bool adaptiveCycleActive_ = false;
  unsigned long adaptiveTapEndsUs_ = 0;
  unsigned long adaptiveCycleEndsUs_ = 0;
  AdaptiveStep adaptiveStep_ = AdaptiveStep::Idle;
  AdaptiveRunStats lastAdaptiveRun_ = {0, 0, 0.0f};
};
