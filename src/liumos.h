#pragma once

#include "acpi.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"

// @asm.S
typedef packed_struct {
  uint16_t limit;
  uint64_t base;
} GDTR;
void ReadGDTR(GDTR *);

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
} IDTGateDescriptor;

typedef packed_struct {
  uint16_t limit;
  IDTGateDescriptor *base;
} IDTR;
void ReadIDTR(IDTR *);
void WriteIDTR(IDTR *);

void Int03(void);

void AsmIntHandler03(void);

// @static.c
extern const GUID EFI_ACPITableGUID;
extern const GUID EFI_GraphicsOutputProtocolGUID;
