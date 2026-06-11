#include "StatusDisplay.h"

StatusDisplay::StatusDisplay(TwoWire* wire, int width, int height, int resetPin, uint8_t address)
    : wire_(wire), display_(width, height, wire, resetPin), address_(address) {}

bool StatusDisplay::begin(int sdaPin, int sclPin) {
  wire_->begin(sdaPin, sclPin);
  ready_ = display_.begin(SSD1306_SWITCHCAPVCC, address_);
  return ready_;
}

void StatusDisplay::render(const DisplaySnapshot& snapshot,
                           int tapPin,
                           bool activeHigh,
                           int sdaPin,
                           int sclPin) {
  if (!ready_) {
    return;
  }

  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.println("SmartFinger Tap");
  display_.print("Mode: ");
  display_.println(snapshot.grade > 0 ? "RUN" : "STOP");
  display_.print("Grade: ");
  display_.println(snapshot.grade);
  display_.print("Rate: ");
  display_.print(snapshot.profile.rate);
  display_.println("/s");
  display_.print("Tap GPIO");
  display_.print(tapPin);
  display_.println(activeHigh ? " HIGH" : " LOW");
  display_.print("I2C SDA");
  display_.print(sdaPin);
  display_.print(" SCL");
  display_.println(sclPin);
  display_.display();
}

bool StatusDisplay::isReady() const {
  return ready_;
}
