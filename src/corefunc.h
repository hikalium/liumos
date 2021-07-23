#pragma once

#include "generic.h"

class EFI;
namespace CoreFunc {
void PutChar(char c);
void PutString(const char* s);
EFI& GetEFI();
};  // namespace CoreFunc
