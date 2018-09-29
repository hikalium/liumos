#pragma once

#include "acpi.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"

// @asm.S
#define CPUID_01_EDX_APIC (1 << 9)
#define CPUID_01_EDX_MSR (1 << 5)
#define MAX_PHY_ADDR_BITS 36
#define IO_APIC_BASE_ADDR 0xfec00000

typedef packed_struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
}
CPUID;

void ReadCPUID(CPUID*, uint32_t eax, uint32_t ecx);

uint64_t ReadMSR(uint32_t);

typedef packed_struct {
  uint16_t limit;
  uint64_t base;
}
GDTR;

void ReadGDTR(GDTR*);

typedef packed_struct {
  // uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t eflags;
  uint64_t rsp;
  uint64_t ss;
}
InterruptInfo;

typedef packed_struct {
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
}
IDTGateDescriptor;

typedef packed_struct {
  uint16_t limit;
  IDTGateDescriptor* base;
}
IDTR;

void ReadIDTR(IDTR*);
void WriteIDTR(IDTR*);

void Int03(void);

uint16_t ReadCSSelector(void);

void AsmIntHandler03(void);
void AsmIntHandler0D(void);

void Disable8259PIC(void);

#define HPET_TIMER_CONFIG_REGISTER_BASE_OFS 0x100
#define HPET_TICK_PER_SEC (10UL * 1000 * 1000)

#define HPET_INT_TYPE_LEVEL_TRIGGERED (1UL << 1)
#define HPET_INT_ENABLE (1UL << 2)
#define HPET_MODE_PERIODIC (1UL << 3)
#define HPET_SET_VALUE (1UL << 6)

typedef packed_struct {
  uint64_t configuration_and_capability;
  uint64_t comparator_value;
  uint64_t fsb_interrupt_route;
  uint64_t reserved;
}
HPETTimerRegisterSet;

packed_struct HPET_REGISTER_SPACE {
  uint64_t general_capabilities_and_id;
  uint64_t reserved00;
  uint64_t general_configuration;
  uint64_t reserved01;
  uint64_t general_interrupt_status;
  uint64_t reserved02;
  uint64_t reserved03[24];
  uint64_t main_counter_value;
  uint64_t reserved04;
  HPETTimerRegisterSet timers[32];
};

// @static.c
extern const GUID EFI_ACPITableGUID;
extern const GUID EFI_GraphicsOutputProtocolGUID;
