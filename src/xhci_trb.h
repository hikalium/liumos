#pragma once

#include "util.h"

struct BasicTRB {
  volatile uint64_t data;
  volatile uint32_t option;
  volatile uint32_t control;
  uint8_t GetTRBType() const { return GetBits<15, 10, uint8_t>(control); }
  uint8_t GetSlotID() const { return GetBits<31, 24, uint8_t>(control); }
  uint8_t GetCompletionCode() const { return GetBits<31, 24, uint8_t>(option); }
  int GetTransferSize() const { return GetBits<23, 0, int>(option); }
};
static_assert(sizeof(BasicTRB) == 16);
