#pragma once
#include "generic.h"

class PersistentMemoryManager {
 public:
  bool IsValid() { return signature_ == kSignature; }
  uint64_t Allocate(uint64_t bytesize);
  void Init();
  void Print();

 private:
  static constexpr uint64_t kSignature = 0x4D50534F6D75696CULL;
  static constexpr uint64_t kGlobalHeaderPages = 1;
  uint64_t signature_;
  uint64_t byte_size_;
  uint64_t free_pages_;
};
