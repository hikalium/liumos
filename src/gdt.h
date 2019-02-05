#pragma once
#include "asm.h"

class GDT {
 public:
  static constexpr uint64_t kDescBitTypeData = 0b10ULL << 43;
  static constexpr uint64_t kDescBitTypeCode = 0b11ULL << 43;

  static constexpr uint64_t kDescBitForSystemSoftware = 1ULL << 52;
  static constexpr uint64_t kDescBitPresent = 1ULL << 47;
  static constexpr uint64_t kDescBitOfsDPL = 45;
  static constexpr uint64_t kDescBitMaskDPL = 0b11ULL << kDescBitOfsDPL;
  static constexpr uint64_t kDescBitAccessed = 1ULL << 40;

  static constexpr uint64_t kCSDescBitLongMode = 1ULL << 53;
  static constexpr uint64_t kCSDescBitReadable = 1ULL << 41;

  static constexpr uint64_t kDSDescBitWritable = 1ULL << 41;

  static constexpr uint64_t kKernelCSIndex = 1;
  static constexpr uint64_t kKernelDSIndex = 2;

  static constexpr uint64_t kKernelCSSelector = kKernelCSIndex << 3;
  static constexpr uint64_t kKernelDSSelector = kKernelDSIndex << 3;

  static constexpr uint64_t kNumOfDescriptors = 3;

  void Init(void);
  void Print(void);

 private:
  GDTR gdtr_;
  uint64_t descriptors_[kNumOfDescriptors];
};
