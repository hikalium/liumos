#pragma once

#include "util.h"

struct BasicTRB {
  volatile uint64_t data;
  volatile uint32_t option;
  volatile uint32_t control;
  uint8_t GetTRBType() const { return GetBits<15, 10, uint8_t>(control); }
};
static_assert(sizeof(BasicTRB) == 16);
