#pragma once

#include <stddef.h>
#include <stdint.h>

struct ClickProfile {
  uint16_t tapMs;
  uint16_t restMs;
  uint8_t rate;
};

struct DisplaySnapshot {
  uint8_t grade;
  ClickProfile profile;
};

