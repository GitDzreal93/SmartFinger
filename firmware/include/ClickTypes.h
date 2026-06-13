#pragma once

#include <stddef.h>
#include <stdint.h>

struct ClickProfile {
  uint16_t tapMs;
  uint16_t restMs;
  uint8_t rate;
};

enum class AdaptiveStep : uint8_t {
  Idle,
  Calculate,
  Press,
  Release,
  Complete,
};

struct DisplaySnapshot {
  uint8_t grade;
  ClickProfile profile;
  bool running;
  bool adaptiveMode;
  bool rushMode;
  bool armed;
  unsigned long remainingMs;
  AdaptiveStep adaptiveStep;
};
