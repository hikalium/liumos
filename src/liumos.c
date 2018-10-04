#include "liumos.h"

GDTR gdtr;
IDTR idtr;
HPETRegisterSpace* hpet_registers;

uint8_t* vram;
int xsize;
int ysize;

void InitEFI(EFISystemTable* system_table) {
  _system_table = system_table;
  _system_table->boot_services->SetWatchdogTimer(0, 0, 0, NULL);
  _system_table->boot_services->LocateProtocol(
      &EFI_GraphicsOutputProtocolGUID, NULL,
      (void**)&efi_graphics_output_protocol);
  vram = efi_graphics_output_protocol->mode->frame_buffer_base;
  xsize = efi_graphics_output_protocol->mode->info->horizontal_resolution;
  ysize = efi_graphics_output_protocol->mode->info->vertical_resolution;
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
    PutStringAndHex("Int20", count);
    count++;
    SendEndOfInterruptToLocalAPIC();
    return;
  }
  EFIPrintStringAndHex(L"Int#", intcode);
  EFIPrintStringAndHex(L"RIP", info->rip);
  EFIPrintStringAndHex(L"CS", info->cs);

  if (intcode == 0x03) {
    EFIPutCString("!!!! Trap !!!!\r\n");
    return;
  }
  Panic("INTHandler not implemented");
}

void PrintIDTGateDescriptor(IDTGateDescriptor* desc) {
  EFIPrintStringAndHex(L"desc.ofs", ((uint64_t)desc->offset_high << 32) |
                                        ((uint64_t)desc->offset_mid << 16) |
                                        desc->offset_low);
  EFIPrintStringAndHex(L"desc.segmt", desc->segment_descriptor);
  EFIPrintStringAndHex(L"desc.IST", desc->interrupt_stack_table);
  EFIPrintStringAndHex(L"desc.type", desc->type);
  EFIPrintStringAndHex(L"desc.DPL", desc->descriptor_privilege_level);
  EFIPrintStringAndHex(L"desc.present", desc->present);
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
  EFIPrintStringAndHex(L"timer.oldconfig", entry->configuration_and_capability);
  uint64_t config = entry->configuration_and_capability;
  uint64_t mask =
      HPET_MODE_PERIODIC | HPET_INT_ENABLE | HPET_INT_TYPE_LEVEL_TRIGGERED;
  config &= ~mask;
  config |= mask & flags;
  config |= HPET_SET_VALUE;
  entry->configuration_and_capability = config;
  entry->comparator_value = count;
  EFIPrintStringAndHex(L"timer.newconfig", entry->configuration_and_capability);
}

void efi_main(void* ImageHandle, struct EFI_SYSTEM_TABLE* system_table) {
  InitEFI(system_table);
  EFIClearScreen();
  EFIGetMemoryMap();

  PutString("liumOS is booting...\n");

  ACPI_RSDP* rsdp = EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID);
  ACPI_XSDT* xsdt = rsdp->xsdt;

  PutStringAndHex("ACPI Table address", (uint64_t)rsdp);
  PutChars(rsdp->signature, 8);
  PutStringAndHex("XSDT Table address", (uint64_t)xsdt);
  PutChars(xsdt->signature, 4);

  int num_of_xsdt_entries = (xsdt->length - ACPI_DESCRIPTION_HEADER_SIZE) >> 3;
  EFIPrintStringAndHex(L"XSDT Table entries", num_of_xsdt_entries);

  ReadGDTR(&gdtr);
  ReadIDTR(&idtr);

  SetIntHandler(0x03, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler03);
  SetIntHandler(0x0d, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler0D);
  SetIntHandler(0x20, ReadCSSelector(), 0, 0xf, 0, AsmIntHandler20);
  WriteIDTR(&idtr);
  Int03();

  CPUID cpuid;
  ReadCPUID(&cpuid, 0, 0);
  EFIPrintStringAndHex(L"Max CPUID", cpuid.eax);

  ReadCPUID(&cpuid, 1, 0);
  if (!(cpuid.eax & CPUID_01_EDX_APIC))
    Panic("APIC not supported");
  if (!(cpuid.eax & CPUID_01_EDX_MSR))
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

  uint8_t boot_cpu_apic_id = 0xff;

  for (int i = 0; i < madt->length - offsetof(ACPI_MADT, entries);
       i += madt->entries[i + 1]) {
    uint8_t type = madt->entries[i];
    if (type == 0) {
      // EFIPrintStringAndHex(L"APIC Processor ID", madt->entries[i + 2]);
      boot_cpu_apic_id = madt->entries[i + 2];
      // EFIPrintStringAndHex(L"  Local APIC ID", madt->entries[i + 3]);
      // EFIPrintStringAndHex(L"  flags", *(uint32_t*)&madt->entries[i + 4]);
    } else if (type == 1) {
      /*
      EFIPrintStringAndHex(L"IO APIC ID", madt->entries[i + 2]);
      EFIPrintStringAndHex(L"  Base addr", *(uint32_t*)&madt->entries[i + 4]);
      EFIPrintStringAndHex(L"  Global system int base",
                           *(uint32_t*)&madt->entries[i + 8]);
                           */
    } else if (type == 2) {
      // EFIPrintStringAndHex(L"Bus source", madt->entries[i + 2]);
      // EFIPrintStringAndHex(L"IRQ source", madt->entries[i + 3]);
      // EFIPrintStringAndHex(L"  global system int",
      //                    *(uint32_t*)&madt->entries[i + 4]);
      /*
      EFIPrintStringAndHex(L"  flags",
                           *(uint16_t*)&madt->entries[i + 8]);
                           */
      if (madt->entries[i + 3] == 0x00) {
        EFIPrintStringAndHex(L"  flags",
                             0x02 & *(uint16_t*)&madt->entries[i + 8]);
        EFIPrintStringAndHex(L"  flags",
                             0x08 & *(uint16_t*)&madt->entries[i + 8]);
      }
    }
  }

  if (boot_cpu_apic_id == 0xff)
    Panic("Boot CPU APIC ID cannot be retrieved");

  Disable8259PIC();

  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  uint64_t local_apic_base_addr =
      (local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) & ~0xfffULL;
  EFIPrintStringAndHex(L"LOCAL APIC BASE addr", local_apic_base_addr);
  EFIPrintStringAndHex(L"LOCAL APIC ID", GetLocalAPICID(local_apic_base_addr));
  EFIPrintStringAndHex(L"ID", *(uint32_t*)(local_apic_base_addr + 0x20));
  EFIPrintStringAndHex(L"VER", *(uint32_t*)(local_apic_base_addr + 0x30));
  EFIPrintStringAndHex(L"IO APIC ID",
                       ReadIOAPICRegister(IO_APIC_BASE_ADDR, 0) >> 24);
  EFIPrintStringAndHex(L"IO APIC VER",
                       ReadIOAPICRegister(IO_APIC_BASE_ADDR, 1));
  EFIPrintStringAndHex(L"IOREDTBL[2]",
                       ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2));

  WriteIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2,
                                   ((uint64_t)boot_cpu_apic_id << 56) | 0x20);
  EFIPrintStringAndHex(L"IOREDTBL[2]",
                       ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2));
  if (!hpet)
    Panic("HPET table not found");

  hpet_registers = hpet->base_address.address;
  uint64_t general_config = hpet_registers->general_configuration;
  EFIPrintStringAndHex(L"geneal cap",
                       hpet_registers->general_capabilities_and_id);
  EFIPrintStringAndHex(L"geneal configuration", general_config);
  general_config |= 1;
  general_config |= (1ULL << 1);
  hpet_registers->general_configuration = general_config;
  EFIPrintStringAndHex(L"geneal configuration",
                       hpet_registers->general_configuration);

  count = 0;
  SetHPETTimer(0, HPET_TICK_PER_SEC * 1, HPET_MODE_PERIODIC | HPET_INT_ENABLE);

  SetHPETTimer(1, HPET_TICK_PER_SEC * 100, HPET_MODE_PERIODIC);

  hpet_registers->main_counter_value = 0;

  while (1)
    ;

  while (1) {
    EFIPrintStringAndHex(L"counter", hpet_registers->main_counter_value);
    EFIPrintStringAndHex(L"int status",
                         hpet_registers->general_interrupt_status);
    uint64_t int_status = hpet_registers->general_interrupt_status;
    int_status |= 1ULL << 0;
    hpet_registers->general_interrupt_status = int_status;
    EFIPrintStringAndHex(L"timer[0].comp",
                         hpet_registers->timers[0].comparator_value);
    EFIPrintStringAndHex(L"IOREDTBL[2]",
                         ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2));
    EFIPrintStringAndHex(L"ISR", *(uint32_t*)(local_apic_base_addr + 0x0100));
    EFIPrintStringAndHex(L"IRR", *(uint32_t*)(local_apic_base_addr + 0x0200));
    EFIPrintStringAndHex(L"ESR", *(uint32_t*)(local_apic_base_addr + 0x0280));

    // EFIGetChar();
  };

  if (nfit) {
    EFIPutCString("NFIT found");
    EFIPrintStringAndHex(L"NFIT Size", nfit->length);
    EFIPrintStringAndHex(L"First NFIT Structure Type", nfit->entry[0]);
    EFIPrintStringAndHex(L"First NFIT Structure Size", nfit->entry[1]);
    if (nfit->entry[0] == kSystemPhysicalAddressRangeStructure) {
      ACPI_NFIT_SPARange* spa_range = (ACPI_NFIT_SPARange*)&nfit->entry[0];
      EFIPrintStringAndHex(L"SPARange Base",
                           spa_range->system_physical_address_range_base);
      EFIPrintStringAndHex(L"SPARange Length",
                           spa_range->system_physical_address_range_length);
      EFIPrintStringAndHex(L"SPARange Attribute",
                           spa_range->address_range_memory_mapping_attribute);
      EFIPrintStringAndHex(L"SPARange TypeGUID[0]",
                           spa_range->address_range_type_guid[0]);
      EFIPrintStringAndHex(L"SPARange TypeGUID[1]",
                           spa_range->address_range_type_guid[1]);
      uint64_t* p = (uint64_t*)spa_range->system_physical_address_range_base;
      EFIPrintStringAndHex(L"PMEM Previous value: ", *p);
      *p = *p + 3;
      EFIPrintStringAndHex(L"PMEM value after write: ", *p);
    }
  }

  // EFIPrintMemoryMap();
  while (1) {
    wchar_t c = EFIGetChar();
    EFIPutChar(c);
  };
}
