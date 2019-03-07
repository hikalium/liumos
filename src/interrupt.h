#pragma once
#include "ring_buffer.h"
#include "scheduler.h"

using InterruptHandler = void (*)(uint64_t intcode,
                                  uint64_t error_code,
                                  InterruptInfo* info);

extern Scheduler* scheduler;
void InitIDT(void);
void SetIntHandler(uint64_t intcode, InterruptHandler handler);
