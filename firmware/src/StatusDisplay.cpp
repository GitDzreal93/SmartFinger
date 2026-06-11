#include "StatusDisplay.h"

namespace {

const char* adaptiveStepLabel(AdaptiveStep step) {
  switch (step) {
    case AdaptiveStep::Calculate:
      return "CALC";
    case AdaptiveStep::Press:
      return "PRESS";
    case AdaptiveStep::Release:
      return "RELEASE";
    case AdaptiveStep::Complete:
      return "DONE";
    case AdaptiveStep::Idle:
    default:
      return "WAIT";
  }
}

}  // namespace

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
  if (snapshot.armed) {
    display_.println("ARMED");
  } else {
    display_.println(snapshot.running ? "RUN" : "STOP");
  }
  display_.print("Grade: ");
  display_.println(snapshot.grade);
  if (!snapshot.adaptiveMode) {
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
  } else {
    display_.print("Left: ");
    display_.print(static_cast<float>(snapshot.remainingMs) / 1000.0f, 1);
    display_.println("s");
    display_.print("Step: ");
    display_.println(adaptiveStepLabel(snapshot.adaptiveStep));
    display_.print("Tap GPIO");
    display_.print(tapPin);
    display_.println(activeHigh ? " HIGH" : " LOW");
  }
  display_.display();
}

bool StatusDisplay::isReady() const {
  return ready_;
}
