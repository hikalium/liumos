#pragma once

#include "generic.h"

extern "C" {

namespace CPUIDIndex {
constexpr uint32_t kXTopology = 0x0B;
constexpr uint32_t kMaxAddr = 0x8000'0008;
}  // namespace CPUIDIndex

constexpr uint32_t kCPUID01H_EDXBitAPIC = (1 << 9);
constexpr uint32_t kCPUID01H_ECXBitx2APIC = (1 << 21);
constexpr uint32_t kCPUID01H_EDXBitMSR = (1 << 5);
constexpr uint64_t kIOAPICRegIndexAddr = 0xfec00000;
constexpr uint64_t kIOAPICRegDataAddr = kIOAPICRegIndexAddr + 0x10;
constexpr uint64_t kLocalAPICBaseBitAPICEnabled = (1 << 11);
constexpr uint64_t kLocalAPICBaseBitx2APICEnabled = (1 << 10);

constexpr uint64_t kRFlagsInterruptEnable = (1ULL << 9);

packed_struct CPUFeatureSet {
  uint64_t max_phy_addr;
  uint64_t phy_addr_mask;               // = (1ULL << max_phy_addr) - 1
  uint64_t kernel_phys_page_map_begin;  // = ~((1ULL << (max_phy_addr - 1) - 1))
  uint32_t max_cpuid;
  uint32_t max_extended_cpuid;
  uint8_t family, model, stepping;
  bool x2apic;
  bool clfsh;
  bool clflushopt;
  char brand_string[48];
};

packed_struct CPUID {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

packed_struct GDTR {
  uint16_t limit;
  uint64_t* base;
};

packed_struct GeneralRegisterContext {
  uint64_t rax;
  uint64_t rdx;
  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  //
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  //
  uint64_t rcx;
};
static_assert(sizeof(GeneralRegisterContext) == (16 - 1) * 8);

packed_struct InterruptContext {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};
static_assert(sizeof(InterruptContext) == 40);

packed_struct CPUContext {
  uint64_t cr3;
  GeneralRegisterContext greg;
  InterruptContext int_ctx;
};

packed_struct InterruptInfo {
  GeneralRegisterContext greg;
  uint64_t error_code;
  InterruptContext int_ctx;
};
static_assert(sizeof(InterruptInfo) == (16 + 4 + 1) * 8);

enum class IDTType {
  kInterruptGate = 0xE,
  kTrapGate = 0xF,
};

packed_struct IDTGateDescriptor {
  uint16_t offset_low;
  uint16_t segment_descriptor;
  unsigned interrupt_stack_table : 3;
  unsigned reserved0 : 5;
  unsigned type : 4;
  unsigned reserved1 : 1;
  unsigned descriptor_privilege_level : 2;
  unsigned present : 1;
  unsigned offset_mid : 16;
  uint32_t offset_high;
  uint32_t reserved2;
};

packed_struct IDTR {
  uint16_t limit;
  IDTGateDescriptor* base;
};

packed_struct IA32_EFER_BITS {
  unsigned syscall_enable : 1;
  unsigned reserved0 : 7;
  unsigned LME : 1;
  unsigned reserved1 : 1;
  unsigned LMA : 1;
  unsigned NXE : 1;
};

packed_struct IA32_EFER {
  union {
    uint64_t data;
    IA32_EFER_BITS bits;
  };
};

packed_struct IA32_MaxPhyAddr_BITS {
  uint8_t physical_address_bits;
  uint8_t linear_address_bits;
};

packed_struct IA32_MaxPhyAddr {
  union {
    uint64_t data;
    IA32_MaxPhyAddr_BITS bits;
  };
};

packed_struct IA_CR3_BITS {
  uint64_t ignored0 : 3;
  uint64_t PWT : 1;
  uint64_t PCD : 1;
  uint64_t ignored1 : 7;
  uint64_t pml4_addr : 52;
};

packed_struct IA_TSS64 {
  uint32_t reserved0;
  uint64_t rsp[3];
  uint64_t ist[9];
  uint16_t reserved1;
  uint16_t io_map_base_addr_ofs;
};
static_assert(sizeof(IA_TSS64) == 104);

enum class MSRIndex : uint32_t {
  kLocalAPICBase = 0x1b,
  kx2APICEndOfInterrupt = 0x80b,
  kEFER = 0xC0000080,
  kSTAR = 0xC0000081,
  kLSTAR = 0xC0000082,
  kFMASK = 0xC0000084,
  kFSBase = 0xC000'0100,
  kKernelGSBase = 0xC0000102,
};

__attribute__((ms_abi)) void Sleep(void);
__attribute__((ms_abi)) void ReadCPUID(CPUID*, uint32_t eax, uint32_t ecx);

__attribute__((ms_abi)) uint64_t ReadMSR(MSRIndex);
__attribute__((ms_abi)) void WriteMSR(MSRIndex, uint64_t);

__attribute__((ms_abi)) void ReadGDTR(GDTR*);
__attribute__((ms_abi)) void WriteGDTR(GDTR*);
__attribute__((ms_abi)) void ReadIDTR(IDTR*);
__attribute__((ms_abi)) void WriteIDTR(IDTR*);
__attribute__((ms_abi)) void WriteTaskRegister(uint16_t);
__attribute__((ms_abi)) void Int03(void);
__attribute__((ms_abi)) uint8_t ReadIOPort8(uint16_t);
__attribute__((ms_abi)) void WriteIOPort8(uint16_t, uint8_t);
__attribute__((ms_abi)) void StoreIntFlag(void);
__attribute__((ms_abi)) void StoreIntFlagAndHalt(void);
__attribute__((ms_abi)) void ClearIntFlag(void);
[[noreturn]] __attribute__((ms_abi)) void Die(void);
__attribute__((ms_abi)) uint16_t ReadCSSelector(void);
__attribute__((ms_abi)) uint16_t ReadSSSelector(void);
__attribute__((ms_abi)) void WriteCSSelector(uint16_t);
__attribute__((ms_abi)) void WriteSSSelector(uint16_t);
__attribute__((ms_abi)) void WriteDataAndExtraSegmentSelectors(uint16_t);
__attribute__((ms_abi)) uint64_t ReadCR2(void);
__attribute__((ms_abi)) uint64_t ReadCR3(void);
__attribute__((ms_abi)) void WriteCR3(uint64_t);
__attribute__((ms_abi)) uint64_t CompareAndSwap(uint64_t*, uint64_t);
__attribute__((ms_abi)) void SwapGS(void);
__attribute__((ms_abi)) uint64_t ReadRSP(void);
__attribute__((ms_abi)) void ChangeRSP(uint64_t);
__attribute__((ms_abi)) void RepeatMoveBytes(size_t count,
                                             const void* dst,
                                             const void* src);
__attribute__((ms_abi)) void RepeatMove4Bytes(size_t count,
                                              const void* dst,
                                              const void* src);
__attribute__((ms_abi)) void RepeatStore4Bytes(size_t count,
                                               const void* dst,
                                               uint32_t data);
__attribute__((ms_abi)) void RepeatMove8Bytes(size_t count,
                                              const void* dst,
                                              const void* src);
__attribute__((ms_abi)) void RepeatStore8Bytes(size_t count,
                                               const void* dst,
                                               uint64_t data);
__attribute__((ms_abi)) void CLFlushOptimized(const void*);
__attribute__((ms_abi)) void JumpToKernel(void* kernel_entry_point,
                                          void* vram_sheet,
                                          uint64_t kernel_stack_pointer);
__attribute__((ms_abi)) void AsmSyscallHandler(void);
__attribute__((ms_abi)) void AsmIntHandler00_DivideError(void);
__attribute__((ms_abi)) void AsmIntHandler03(void);
__attribute__((ms_abi)) void AsmIntHandler06(void);
__attribute__((ms_abi)) void AsmIntHandler07_DeviceNotAvailable(void);
__attribute__((ms_abi)) void AsmIntHandler08(void);
__attribute__((ms_abi)) void AsmIntHandler0D(void);
__attribute__((ms_abi)) void AsmIntHandler0E(void);
__attribute__((ms_abi)) void AsmIntHandler10_x87FPUError(void);
__attribute__((ms_abi)) void AsmIntHandler13_SIMDFPException(void);
__attribute__((ms_abi)) void AsmIntHandler20(void);
__attribute__((ms_abi)) void AsmIntHandler21(void);
__attribute__((ms_abi)) void AsmIntHandlerNotImplemented(void);
__attribute__((ms_abi)) void Disable8259PIC(void);
}
