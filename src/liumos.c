#include "liumos.h"

void InitEFI(EFISystemTable* system_table) {
  _system_table = system_table;
  _system_table->boot_services->SetWatchdogTimer(0, 0, 0, NULL);
  _system_table->boot_services->LocateProtocol(
      &EFI_GraphicsOutputProtocolGUID, NULL,
      (void**)&efi_graphics_output_protocol);
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
    while (1) {
      EFIPrintStringAndHex(L"counter", hpet_registers->main_counter_value);

      EFIGetChar();
    };
  }

  // EFIPrintMemoryMap();
  while (1) {
    wchar_t c = EFIGetChar();
    EFIPutChar(c);
  };
}
