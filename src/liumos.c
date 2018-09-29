#include "liumos.h"

void InitEFI(EFISystemTable* system_table) {
  _system_table = system_table;
  _system_table->boot_services->SetWatchdogTimer(0, 0, 0, NULL);
  _system_table->boot_services->LocateProtocol(
      &EFI_GraphicsOutputProtocolGUID, NULL,
      (void**)&efi_graphics_output_protocol);
}

void IntHandler03() {
  EFIPutString(L"Int03\r\n");
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

_Noreturn void Panic(const char* s) {
  EFIPutCString("!!!! PANIC !!!!\r\n");
  EFIPutCString(s);
  for (;;) {
  }
}

uint8_t GetLocalAPICID(uint64_t local_apic_base_addr) {
  return *(uint32_t*)(local_apic_base_addr + 0x20) >> 24;
}

uint32_t ReadIOAPICRegister(uint64_t io_apic_base_addr, uint8_t reg_index) {
  *(uint32_t volatile*)(io_apic_base_addr) = (uint32_t)reg_index;
  return *(uint32_t volatile*)(io_apic_base_addr + 0x10);
}

void efi_main(void* ImageHandle, struct EFI_SYSTEM_TABLE* system_table) {
  InitEFI(system_table);
  EFIClearScreen();
  EFIPutString(L"Hello, liumOS world!...\r\n");
  EFIGetMemoryMap();
  EFIPrintStringAndHex(L"Num of table entries",
                       system_table->number_of_table_entries);
  ACPI_RSDP* rsdp = EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID);
  ACPI_XSDT* xsdt = rsdp->xsdt;
  EFIPrintStringAndHex(L"ACPI Table address", (uint64_t)rsdp);
  EFIPutnCString(rsdp->signature, 8);
  EFIPrintStringAndHex(L"XSDT Table address", (uint64_t)xsdt);
  EFIPutnCString(xsdt->signature, 4);
  int num_of_xsdt_entries = (xsdt->length - ACPI_DESCRIPTION_HEADER_SIZE) >> 3;
  EFIPrintStringAndHex(L"XSDT Table entries", num_of_xsdt_entries);
  EFIPrintStringAndHex(L"XSDT Table entries", num_of_xsdt_entries);

  uint8_t* vram = efi_graphics_output_protocol->mode->frame_buffer_base;
  int xsize = efi_graphics_output_protocol->mode->info->horizontal_resolution;
  int ysize = efi_graphics_output_protocol->mode->info->vertical_resolution;

  for (int y = 0; y < ysize; y++) {
    for (int x = 0; x < xsize; x++) {
      vram[(xsize * y + x) * 4] = x;
      vram[(xsize * y + x) * 4 + 1] = y;
      vram[(xsize * y + x) * 4 + 2] = x + y;
    }
  }

  GDTR gdtr;
  ReadGDTR(&gdtr);

  IDTR idtr;
  ReadIDTR(&idtr);

  IDTGateDescriptor* sample_desc = NULL;
  for (int i = 0; i * sizeof(IDTGateDescriptor) < idtr.limit; i++) {
    if (!idtr.base[i].present)
      continue;
    sample_desc = &idtr.base[i];
    break;
  }

  idtr.base[0x03] = *sample_desc;

  idtr.base[0x03].type = 0xf;
  idtr.base[0x03].offset_low = (uint64_t)AsmIntHandler03 & 0xffff;
  idtr.base[0x03].offset_mid = ((uint64_t)AsmIntHandler03 >> 16) & 0xffff;
  idtr.base[0x03].offset_high = ((uint64_t)AsmIntHandler03 >> 32) & 0xffffffff;

  WriteIDTR(&idtr);
  Int03();

  Disable8259PIC();

  CPUID cpuid;
  ReadCPUID(&cpuid, 0, 0);
  EFIPrintStringAndHex(L"Max CPUID", cpuid.eax);

  ReadCPUID(&cpuid, 1, 0);
  if (!(cpuid.eax & CPUID_01_EDX_APIC))
    Panic("APIC not supported");
  if (!(cpuid.eax & CPUID_01_EDX_MSR))
    Panic("MSR not supported");

  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  EFIPrintStringAndHex(L"LOCAL APIC BASE MSR", local_apic_base_msr);
  uint64_t local_apic_base_addr =
      (local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) & ~0xfffULL;
  EFIPrintStringAndHex(L"LOCAL APIC BASE addr", local_apic_base_addr);
  EFIPrintStringAndHex(L"LOCAL APIC ID", GetLocalAPICID(local_apic_base_addr));
  EFIPrintStringAndHex(L"IO APIC ID",
                       ReadIOAPICRegister(IO_APIC_BASE_ADDR, 0) >> 24);
  EFIPrintStringAndHex(L"IO APIC VER",
                       ReadIOAPICRegister(IO_APIC_BASE_ADDR, 1));

  ACPI_NFIT* nfit = NULL;
  ACPI_HPET* hpet = NULL;

  for (int i = 0; i < num_of_xsdt_entries; i++) {
    if (IsEqualStringWithSize(xsdt->entry[i], "NFIT", 4))
      nfit = xsdt->entry[i];
    if (IsEqualStringWithSize(xsdt->entry[i], "HPET", 4))
      hpet = xsdt->entry[i];
  }

  if (nfit) {
    EFIPutCString("NFIT found.");
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

  if (hpet) {
    EFIPutCString("HPET found.");
    EFIPrintStringAndHex(L"base addr", (uint64_t)hpet->base_address.address);
    HPETRegisterSpace* hpet_registers = hpet->base_address.address;
    uint64_t general_config = hpet_registers->general_configuration;
    EFIPrintStringAndHex(L"geneal cap",
                         hpet_registers->general_capabilities_and_id);
    EFIPrintStringAndHex(L"geneal configuration", general_config);
    general_config |= 1;
    hpet_registers->general_configuration = general_config;

    EFIPrintStringAndHex(L"geneal configuration",
                         hpet_registers->general_configuration);
    EFIPrintStringAndHex(
        L"timer[0].config",
        hpet_registers->timers[0].configuration_and_capability);
    uint64_t config = hpet_registers->timers[0].configuration_and_capability;
    config |= (1 << 6);
    config |= (1 << 3);
    config |= (1 << 2);
    config |= (1 << 1);
    hpet_registers->timers[0].configuration_and_capability = config;
    hpet_registers->timers[0].comparator_value = HPET_TICK_PER_SEC * 10;
    EFIPrintStringAndHex(L"timer[0].comp",
                         hpet_registers->timers[0].comparator_value);
    EFIPrintStringAndHex(L"timer[0].fsb int",
                         hpet_registers->timers[0].fsb_interrupt_route);
    hpet_registers->main_counter_value = 0;
    while (1) {
      EFIPrintStringAndHex(L"counter", hpet_registers->main_counter_value);
      EFIPrintStringAndHex(L"int status",
                           hpet_registers->general_interrupt_status);
      uint64_t int_status = hpet_registers->general_interrupt_status;
      int_status |= 1ULL << 0;
      hpet_registers->general_interrupt_status = int_status;
      EFIPrintStringAndHex(L"timer[0].comp",
                           hpet_registers->timers[0].comparator_value);

      EFIGetChar();
    };
  }

  // EFIPrintMemoryMap();
  while (1) {
    wchar_t c = EFIGetChar();
    EFIPutChar(c);
  };
}
