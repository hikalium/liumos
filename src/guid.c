#include "guid.h"

bool IsEqualGUID(const GUID* guid1, const GUID* guid2) {
  return guid1->data1 == guid2->data1 && guid1->data2 == guid2->data2;
}
