#pragma once

#include <Arduino.h>

class EncoderInput {
 public:
  EncoderInput(int clkPin, int dtPin);

  void begin();
  int pollDelta();

 private:
  int clkPin_;
  int dtPin_;
  int lastClkState_ = HIGH;
};

