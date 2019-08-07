#pragma once

#include "efi_file.h"

class EFIFileManager {
 public:
  static void Load(EFIFile& dst, const wchar_t* file_name);
};
