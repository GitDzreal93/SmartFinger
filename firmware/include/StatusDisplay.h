#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "ClickTypes.h"

class StatusDisplay {
 public:
  StatusDisplay(TwoWire* wire, int width, int height, int resetPin, uint8_t address);

  bool begin(int sdaPin, int sclPin);
  void render(const DisplaySnapshot& snapshot, int tapPin, bool activeHigh, int sdaPin, int sclPin);
  bool isReady() const;

 private:
  TwoWire* wire_;
  Adafruit_SSD1306 display_;
  uint8_t address_;
  bool ready_ = false;
};
