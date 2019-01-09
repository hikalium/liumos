#pragma once
#include "ring_buffer.h"
#include "scheduler.h"

extern Scheduler* scheduler;
extern RingBuffer<uint8_t, 16> keycode_buffer;
void InitIDT(void);
