#include "EncoderInput.h"

EncoderInput::EncoderInput(int clkPin, int dtPin) : clkPin_(clkPin), dtPin_(dtPin) {}

void EncoderInput::begin() {
  pinMode(clkPin_, INPUT_PULLUP);
  pinMode(dtPin_, INPUT_PULLUP);
  lastClkState_ = digitalRead(clkPin_);
}

int EncoderInput::pollDelta() {
  const int clkState = digitalRead(clkPin_);
  int delta = 0;

  if (clkState != lastClkState_ && clkState == LOW) {
    const int dtState = digitalRead(dtPin_);
    delta = (dtState == HIGH) ? 1 : -1;
  }

  lastClkState_ = clkState;
  return delta;
}

