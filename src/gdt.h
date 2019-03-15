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
  static constexpr uint64_t kUserCS32Index = 3;
  static constexpr uint64_t kUserDSIndex = 4;
  static constexpr uint64_t kUserCS64Index = 5;
  static constexpr uint64_t kTSS64Index = 6;

  static constexpr uint64_t kKernelCSSelector = kKernelCSIndex << 3;
  static constexpr uint64_t kKernelDSSelector = kKernelDSIndex << 3;
  static constexpr uint64_t kUserCS32Selector = kUserCS32Index << 3 | 3;
  static constexpr uint64_t kUserDSSelector = kUserDSIndex << 3 | 3;
  static constexpr uint64_t kUserCS64Selector = kUserCS64Index << 3 | 3;
  static constexpr uint64_t kTSS64Selector = kTSS64Index << 3;

  void Init(uint64_t kernel_stack_pointer, uint64_t ist1_pointer);
  void Print(void);

 private:
  GDTR gdtr_;
  packed_struct GDTDescriptors {
    uint64_t null_segment;
    uint64_t kernel_code_segment;
    uint64_t kernel_data_segment;
    uint64_t user_code_segment_32;
    uint64_t user_data_segment;
    uint64_t user_code_segment_64;
    packed_struct TSS64Entry {
      uint16_t limit_low;
      uint16_t base_low;
      uint8_t base_mid_low;
      uint16_t attr;
      uint8_t base_mid_high;
      uint32_t base_high;
      uint32_t reserved;
      void SetBaseAddr(void* base_addr) {
        uint64_t base_addr_int = reinterpret_cast<uint64_t>(base_addr);
        base_low = base_addr_int & 0xffff;
        base_mid_low = (base_addr_int >> 16) & 0xff;
        base_mid_high = (base_addr_int >> 24) & 0xff;
        base_high = (base_addr_int >> 32) & 0xffffffffUL;
      };
      void SetLimit(uint16_t limit) { limit_low = limit; };
    }
    task_state_segment;
    static_assert(sizeof(TSS64Entry) == 16);
  }
  descriptors_;
  IA_TSS64 tss64_;
};
