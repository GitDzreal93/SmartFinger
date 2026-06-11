#pragma once

#include <stdint.h>

#include "ClickController.h"
#include "EncoderInput.h"
#include "StatusDisplay.h"

class App {
 public:
  App();

  void begin();
  void loop();

 private:
  struct ClockState {
    bool valid = false;
    unsigned long syncedMillis = 0;
    uint32_t secondsOfDay = 0;
  };

  struct ScheduleState {
    bool armed = false;
    unsigned long targetMillis = 0;
    uint32_t targetSecondsOfDay = 0;
    bool hasClockTarget = false;
  };

  void processSerial(unsigned long nowMs);
  void handleCommand(const String& line, unsigned long nowMs);
  void emitOk(const String& message);
  void emitError(const String& message);
  void emitState(unsigned long nowMs);
  void armRelativeStart(unsigned long delayMs, unsigned long nowMs);
  void armClockStart(uint32_t targetSecondsOfDay, unsigned long nowMs);
  void cancelSchedule();
  void updateSchedule(unsigned long nowMs);
  uint32_t currentSecondsOfDay(unsigned long nowMs) const;
  unsigned long scheduleRemainingMs(unsigned long nowMs) const;
  bool parseTimeOfDay(const String& token, uint32_t* secondsOfDayOut) const;
  String currentClockLabel(unsigned long nowMs) const;
  String currentModeLabel() const;
  String currentStepLabel() const;

  void applyGrade(uint8_t nextGrade, unsigned long nowMs);
  void refreshDisplay(unsigned long nowMs);

  ClickController clickController_;
  EncoderInput encoder_;
  StatusDisplay display_;
  ClockState clockState_;
  ScheduleState scheduleState_;
  String serialBuffer_;
  unsigned long lastStateEmitMs_ = 0;
  uint8_t selectedFixedGrade_ = 3;
};
