#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class ClickController {
 public:
  ClickController(int tapPin, int activeLevel, int idleLevel);

  void begin();
  void setGrade(uint8_t nextGrade, unsigned long nowMs);
  void update(unsigned long nowMs);

  uint8_t grade() const;
  bool isRunning() const;
  bool isTapping() const;
  const ClickProfile& currentProfile() const;

 private:
  void releaseTap();
  void startTap();

  int tapPin_;
  int activeLevel_;
  int idleLevel_;
  uint8_t grade_ = 0;
  bool tapping_ = false;
  unsigned long phaseStartedMs_ = 0;
};

