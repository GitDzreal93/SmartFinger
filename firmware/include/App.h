#pragma once

#include "ClickController.h"
#include "EncoderInput.h"
#include "StatusDisplay.h"

class App {
 public:
  App();

  void begin();
  void loop();

 private:
  void applyGrade(uint8_t nextGrade, unsigned long nowMs);
  void refreshDisplay();

  ClickController clickController_;
  EncoderInput encoder_;
  StatusDisplay display_;
};

