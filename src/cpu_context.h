#pragma once

#include "asm.h"
#include "generic.h"

struct CPUContext {
  InterruptInfo int_info;
  // scratch registers
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t rax;
  uint64_t r8;
  uint64_t rdx;
  uint64_t rcx;
  // rest of general registers
  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
};
