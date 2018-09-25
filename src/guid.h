#pragma once

#include "generic.h"

typedef struct {
  uint64_t data1;
  uint64_t data2;
} GUID;

bool IsEqualGUID(const GUID* guid1, const GUID* guid2);
