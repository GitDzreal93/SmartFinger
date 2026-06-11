#include "App.h"

#include "AppConfig.h"

App::App()
    : clickController_(AppConfig::kTapPin, AppConfig::kTapActiveLevel, AppConfig::kTapIdleLevel),
      encoder_(AppConfig::kEncoderClkPin, AppConfig::kEncoderDtPin),
      display_(&Wire,
               AppConfig::kOledWidth,
               AppConfig::kOledHeight,
               AppConfig::kOledResetPin,
               AppConfig::kOledAddress) {}

void App::begin() {
  Serial.begin(115200);
  delay(300);

  encoder_.begin();
  clickController_.begin();
  clickController_.setGrade(0, millis());

  if (!display_.begin(AppConfig::kOledSdaPin, AppConfig::kOledSclPin)) {
    Serial.println("OLED init failed. Check GND/VCC/SCL/SDA or I2C address.");
  }

  Serial.println("SmartFinger firmware init OK");
  refreshDisplay();
}

void App::loop() {
  const unsigned long nowMs = millis();
  const int delta = encoder_.pollDelta();
  if (delta != 0) {
    const int nextGrade = constrain(static_cast<int>(clickController_.grade()) + delta, 0, AppConfig::kMaxGrade);
    applyGrade(static_cast<uint8_t>(nextGrade), nowMs);
  }

  clickController_.update(nowMs);
}

void App::applyGrade(uint8_t nextGrade, unsigned long nowMs) {
  if (nextGrade == clickController_.grade()) {
    return;
  }

  clickController_.setGrade(nextGrade, nowMs);
  refreshDisplay();
}

void App::refreshDisplay() {
  const DisplaySnapshot snapshot = {
      clickController_.grade(),
      clickController_.currentProfile(),
  };
  display_.render(snapshot,
                  AppConfig::kTapPin,
                  AppConfig::kTapActiveLevel == HIGH,
                  AppConfig::kOledSdaPin,
                  AppConfig::kOledSclPin);
}
