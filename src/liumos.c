#include "liumos.h"

GDTR gdtr;
IDTR idtr;
HPETRegisterSpace* hpet_registers;
uint64_t hpet_count_per_femtosecond;

uint8_t* vram;
int xsize;
int ysize;
int pixel_format;
int pixels_per_scan_line;

void InitEFI(EFISystemTable* system_table) {
  _system_table = system_table;
  _system_table->boot_services->SetWatchdogTimer(0, 0, 0, NULL);
  _system_table->boot_services->LocateProtocol(
      &EFI_GraphicsOutputProtocolGUID, NULL,
      (void**)&efi_graphics_output_protocol);
}

void InitGraphics() {
  vram = efi_graphics_output_protocol->mode->frame_buffer_base;
  xsize = efi_graphics_output_protocol->mode->info->horizontal_resolution;
  ysize = efi_graphics_output_protocol->mode->info->vertical_resolution;
  pixel_format = efi_graphics_output_protocol->mode->info->pixel_format;
  pixels_per_scan_line =
      efi_graphics_output_protocol->mode->info->pixels_per_scan_line;
}

_Noreturn void Panic(const char* s) {
  PutString("!!!! PANIC !!!!\r\n");
  PutString(s);
  for (;;) {
  }
}

uint32_t* GetLocalAPICRegisterAddr(uint64_t offset) {
  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  return (
      uint32_t*)(((local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) &
                  ~0xfffULL) +
                 offset);
}

void SendEndOfInterruptToLocalAPIC() {
  *GetLocalAPICRegisterAddr(0xB0) = 0;
}

int count = 0;
void IntHandler(uint64_t intcode, InterruptInfo* info) {
  if (intcode == 0x20) {
    PutString(".");
    count++;
    SendEndOfInterruptToLocalAPIC();
    return;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS", info->cs);

  if (intcode == 0x03) {
    PutString("!!!! Int3 Trap !!!!\n");
    return;
  }
  Panic("INTHandler not implemented");
}

void PrintIDTGateDescriptor(IDTGateDescriptor* desc) {
  PutStringAndHex("desc.ofs", ((uint64_t)desc->offset_high << 32) |
                                  ((uint64_t)desc->offset_mid << 16) |
                                  desc->offset_low);
  PutStringAndHex("desc.segmt", desc->segment_descriptor);
  PutStringAndHex("desc.IST", desc->interrupt_stack_table);
  PutStringAndHex("desc.type", desc->type);
  PutStringAndHex("desc.DPL", desc->descriptor_privilege_level);
  PutStringAndHex("desc.present", desc->present);
}

uint8_t GetLocalAPICID(uint64_t local_apic_base_addr) {
  return *(uint32_t*)(local_apic_base_addr + 0x20) >> 24;
}

uint32_t ReadIOAPICRegister(uint64_t io_apic_base_addr, uint8_t reg_index) {
  *(uint32_t volatile*)(io_apic_base_addr) = (uint32_t)reg_index;
  return *(uint32_t volatile*)(io_apic_base_addr + 0x10);
}

void WriteIOAPICRegister(uint64_t io_apic_base_addr,
                         uint8_t reg_index,
                         uint32_t value) {
  *(uint32_t volatile*)(io_apic_base_addr) = (uint32_t)reg_index;
  *(uint32_t volatile*)(io_apic_base_addr + 0x10) = value;
}

uint64_t ReadIOAPICRedirectTableRegister(uint64_t io_apic_base_addr,
                                         uint8_t irq_index) {
  return (uint64_t)ReadIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2) |
         ((uint64_t)ReadIOAPICRegister(io_apic_base_addr,
                                       0x10 + irq_index * 2 + 1)
          << 32);
}

void WriteIOAPICRedirectTableRegister(uint64_t io_apic_base_addr,
                                      uint8_t irq_index,
                                      uint64_t value) {
  WriteIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2, (uint32_t)value);
  WriteIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2 + 1,
                      (uint32_t)(value >> 32));
}

void SetIntHandler(int index,
                   uint8_t segm_desc,
                   uint8_t ist,
                   uint8_t type,
                   uint8_t dpl,
                   void (*handler)()) {
  IDTGateDescriptor* desc = &idtr.base[index];
  desc->segment_descriptor = segm_desc;
  desc->interrupt_stack_table = ist;
  desc->type = type;
  desc->descriptor_privilege_level = dpl;
  desc->present = 1;
  desc->offset_low = (uint64_t)handler & 0xffff;
  desc->offset_mid = ((uint64_t)handler >> 16) & 0xffff;
  desc->offset_high = ((uint64_t)handler >> 32) & 0xffffffff;
  desc->reserved0 = 0;
  desc->reserved1 = 0;
  desc->reserved2 = 0;
}

void SetHPETTimer(int timer_index, uint64_t count, uint64_t flags) {
  HPETTimerRegisterSet* entry = &hpet_registers->timers[timer_index];
  PutStringAndHex("timer.oldconfig", entry->configuration_and_capability);
  uint64_t config = entry->configuration_and_capability;
  uint64_t mask =
      HPET_MODE_PERIODIC | HPET_INT_ENABLE | HPET_INT_TYPE_LEVEL_TRIGGERED;
  config &= ~mask;
  config |= mask & flags;
  config |= HPET_SET_VALUE;
  entry->configuration_and_capability = config;
  entry->comparator_value = count;
  PutStringAndHex("timer.newconfig", entry->configuration_and_capability);
}

void InitGDT() {
  ReadGDTR(&gdtr);
}

void InitIDT() {
  ReadIDTR(&idtr);

  ClearIntFlag();

  SetIntHandler(0x03, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler03);
  SetIntHandler(0x0d, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler0D);
  SetIntHandler(0x20, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler20);
  WriteIDTR(&idtr);

  StoreIntFlag();
}

void InitIOAPIC(uint64_t local_apic_id) {
  uint64_t redirect_table =
      ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2);
  redirect_table &= 0x00fffffffffe0000UL;
  redirect_table |= (local_apic_id << 56) | 0x20;
  WriteIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2, redirect_table);
}

void InitHPET() {
  uint64_t general_config = hpet_registers->general_configuration;
  hpet_count_per_femtosecond =
      hpet_registers->general_capabilities_and_id >> 32;
  PutStringAndHex("count_per_femtosecond", hpet_count_per_femtosecond);
  PutStringAndHex("geneal cap", hpet_registers->general_capabilities_and_id);
  PutStringAndHex("geneal configuration", general_config);
  general_config |= 1;
  general_config |= (1ULL << 1);
  hpet_registers->general_configuration = general_config;
  PutStringAndHex("geneal configuration",
                  hpet_registers->general_configuration);
}

void efi_main(void* ImageHandle, struct EFI_SYSTEM_TABLE* system_table) {
  InitEFI(system_table);
  EFIClearScreen();
  EFIGetMemoryMap();

  InitGraphics();
  EnableVideoModeForConsole();

  PutString("liumOS is booting...\n");

  InitGDT();
  InitIDT();

  ACPI_RSDP* rsdp = EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID);
  ACPI_XSDT* xsdt = rsdp->xsdt;

  int num_of_xsdt_entries = (xsdt->length - ACPI_DESCRIPTION_HEADER_SIZE) >> 3;

  CPUID cpuid;
  ReadCPUID(&cpuid, 0, 0);
  PutStringAndHex("Max CPUID", cpuid.eax);

  DrawRect(300, 100, 200, 200, 0xffffff);
  DrawRect(350, 150, 200, 200, 0xff0000);
  DrawRect(400, 200, 200, 200, 0x00ff00);
  DrawRect(450, 250, 200, 200, 0x0000ff);

  ReadCPUID(&cpuid, 1, 0);
  if (!(cpuid.edx & CPUID_01_EDX_APIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & CPUID_01_EDX_MSR))
    Panic("MSR not supported");

  ACPI_NFIT* nfit = NULL;
  ACPI_HPET* hpet = NULL;
  ACPI_MADT* madt = NULL;

  for (int i = 0; i < num_of_xsdt_entries; i++) {
    if (IsEqualStringWithSize(xsdt->entry[i], "NFIT", 4))
      nfit = xsdt->entry[i];
    if (IsEqualStringWithSize(xsdt->entry[i], "HPET", 4))
      hpet = xsdt->entry[i];
    if (IsEqualStringWithSize(xsdt->entry[i], "APIC", 4))
      madt = xsdt->entry[i];
  }

  if (!madt)
    Panic("MADT not found");

  for (int i = 0; i < madt->length - offsetof(ACPI_MADT, entries);
       i += madt->entries[i + 1]) {
    uint8_t type = madt->entries[i];
    if (type == kProcessorLocalAPICInfo) {
      PutString("Processor 0x");
      PutHex64(madt->entries[i + 2]);
      PutString(" local_apic_id = 0x");
      PutHex64(madt->entries[i + 3]);
      PutString(" flags = 0x");
      PutHex64(madt->entries[i + 4]);
      PutString("\n");
    } else if (type == kInterruptSourceOverrideInfo) {
      PutString("IRQ override: 0x");
      PutHex64(madt->entries[i + 3]);
      PutString(" -> 0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutString("\n");
    }
  }

  Disable8259PIC();

  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  uint64_t local_apic_base_addr =
      (local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) & ~0xfffULL;
  uint64_t local_apic_id = GetLocalAPICID(local_apic_base_addr);
  PutStringAndHex("LOCAL APIC ID", local_apic_id);

  InitIOAPIC(local_apic_id);

  if (!hpet)
    Panic("HPET table not found");

  hpet_registers = hpet->base_address.address;
  InitHPET();

  count = 0;
  SetHPETTimer(0, 1e15 / hpet_count_per_femtosecond * 0.1,
               HPET_MODE_PERIODIC | HPET_INT_ENABLE);

  SetHPETTimer(1, HPET_TICK_PER_SEC * 100, HPET_MODE_PERIODIC);

  hpet_registers->main_counter_value = 0;
  PutStringAndHex("main_counter_value", hpet_registers->main_counter_value);
  PutStringAndHex("main_counter_value", hpet_registers->main_counter_value);

  while (1) {
    StoreIntFlagAndHalt();
  }

  while (1) {
    PutStringAndHex("counter", hpet_registers->main_counter_value);
    PutStringAndHex("int status", hpet_registers->general_interrupt_status);
    uint64_t int_status = hpet_registers->general_interrupt_status;
    int_status |= 1ULL << 0;
    hpet_registers->general_interrupt_status = int_status;
    PutStringAndHex("timer[0].comp",
                    hpet_registers->timers[0].comparator_value);
    PutStringAndHex("IOREDTBL[2]",
                    ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2));
    PutStringAndHex("ISR", *(uint32_t*)(local_apic_base_addr + 0x0100));
    PutStringAndHex("IRR", *(uint32_t*)(local_apic_base_addr + 0x0200));
    PutStringAndHex("ESR", *(uint32_t*)(local_apic_base_addr + 0x0280));
  };

  if (nfit) {
    PutString("NFIT found");
    PutStringAndHex("NFIT Size", nfit->length);
    PutStringAndHex("First NFIT Structure Type", nfit->entry[0]);
    PutStringAndHex("First NFIT Structure Size", nfit->entry[1]);
    if (nfit->entry[0] == kSystemPhysicalAddressRangeStructure) {
      ACPI_NFIT_SPARange* spa_range = (ACPI_NFIT_SPARange*)&nfit->entry[0];
      PutStringAndHex("SPARange Base",
                      spa_range->system_physical_address_range_base);
      PutStringAndHex("SPARange Length",
                      spa_range->system_physical_address_range_length);
      PutStringAndHex("SPARange Attribute",
                      spa_range->address_range_memory_mapping_attribute);
      PutStringAndHex("SPARange TypeGUID[0]",
                      spa_range->address_range_type_guid[0]);
      PutStringAndHex("SPARange TypeGUID[1]",
                      spa_range->address_range_type_guid[1]);
      uint64_t* p = (uint64_t*)spa_range->system_physical_address_range_base;
      PutStringAndHex("PMEM Previous value: ", *p);
      *p = *p + 3;
      PutStringAndHex("PMEM value after write: ", *p);
    }
  }

  while (1) {
    wchar_t c = EFIGetChar();
    EFIPutChar(c);
  };
}
