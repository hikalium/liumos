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

packed_struct CPUFeatureSet {
  uint32_t max_cpuid;
  uint32_t max_extended_cpuid;
  bool x2apic;
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

packed_struct InterruptInfo {
  uint64_t rip;
  uint64_t cs;
  uint64_t eflags;
  uint64_t rsp;
  uint64_t ss;
};
static_assert(sizeof(InterruptInfo) == 40);

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

struct IA_PML4;
packed_struct IA_CR3 {
  union {
    uint64_t data;
    IA_CR3_BITS bits;
  };
  IA_PML4* GetPML4Addr() {
    return reinterpret_cast<IA_PML4*>(bits.pml4_addr << 12);
  }
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
  kKernelGSBase = 0xC0000102,
};

void ReadCPUID(CPUID*, uint32_t eax, uint32_t ecx);

uint64_t ReadMSR(MSRIndex);
void WriteMSR(MSRIndex, uint64_t);

void ReadGDTR(GDTR*);
void WriteGDTR(GDTR*);

void ReadIDTR(IDTR*);
void WriteIDTR(IDTR*);

void WriteTaskRegister(uint16_t);

void Int03(void);

uint8_t ReadIOPort8(uint16_t);
void WriteIOPort8(uint16_t, uint8_t);

void StoreIntFlag(void);
void StoreIntFlagAndHalt(void);
void ClearIntFlag(void);
[[noreturn]] void Die(void);

uint16_t ReadCSSelector(void);
uint16_t ReadSSSelector(void);

void WriteCSSelector(uint16_t);
void WriteSSSelector(uint16_t);
void WriteDataAndExtraSegmentSelectors(uint16_t);

uint64_t ReadCR3(void);
void WriteCR3(uint64_t);

uint64_t CompareAndSwap(uint64_t*, uint64_t);
void SwapGS(void);

void AsmIntHandler03(void);
void AsmIntHandler0D(void);
void AsmIntHandler0E(void);
void AsmIntHandler20(void);
void AsmIntHandler21(void);
void AsmIntHandlerNotImplemented(void);

void Disable8259PIC(void);
}
