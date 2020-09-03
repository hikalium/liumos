#pragma once

#include "efi.h"

constexpr int kNumOfRootFiles = 16;
packed_struct LoaderInfo {
  EFIFile root_files[kNumOfRootFiles];
  int root_files_used;
  EFI* efi;

  int FindFile(const char* name) {
    for (int i = 0; i < root_files_used; i++) {
      if (IsEqualString(root_files[i].GetFileName(), name)) {
        return i;
      }
    }
    return -1;
  }
};
