#pragma once

#include <Arduino.h>

#include "ClickTypes.h"

namespace AppConfig {

constexpr int kTapPin = 3;
constexpr int kEncoderClkPin = 4;
constexpr int kEncoderDtPin = 5;
constexpr int kOledSdaPin = 8;
constexpr int kOledSclPin = 9;
constexpr int kOledWidth = 128;
constexpr int kOledHeight = 64;
constexpr int kOledResetPin = -1;
constexpr uint8_t kOledAddress = 0x3C;
constexpr int kTapActiveLevel = HIGH;
constexpr int kTapIdleLevel = LOW;

constexpr ClickProfile kClickProfiles[] = {
  {0, 0, 0},
  {70, 260, 3},
  {55, 195, 4},
  {45, 155, 5},
  {35, 105, 7},
  {25, 75, 10},
};

constexpr size_t kProfileCount = sizeof(kClickProfiles) / sizeof(kClickProfiles[0]);
constexpr uint8_t kMaxGrade = static_cast<uint8_t>(kProfileCount - 1);

inline constexpr const ClickProfile& profileForGrade(uint8_t grade) {
  return kClickProfiles[grade];
}

static_assert(kProfileCount == 6, "Expected Grade 0-5 profiles.");

}  // namespace AppConfig

