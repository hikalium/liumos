#pragma once

#include "generic.h"

packed_struct GUID {
  uint32_t id1;
  uint16_t id2;
  uint16_t id3;
  uint8_t id4[8];
};

bool IsEqualGUID(const GUID* guid1, const GUID* guid2);
void PutGUID(const GUID* guid);
