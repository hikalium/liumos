#pragma once

#include "generic.h"

class EFI;
namespace CoreFunc {
void PutChar(char c);
EFI& GetEFI();
};  // namespace CoreFunc
