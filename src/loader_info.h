#pragma once

#include "efi.h"
#include "phys_page_allocator.h"

constexpr int kNumOfRootFiles = 32;
packed_struct LoaderInfo {
  PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>* dram_allocator;
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

LoaderInfo& GetLoaderInfo();  // Implemented in loader.cc and kernel.cc
