#include "liumos.h"

void efi_main(void* ImageHandle, struct EFI_SYSTEM_TABLE* system_table) {
  EFIInit(system_table);
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

  ACPI_NFIT* nfit = NULL;

  for (int i = 0; i < num_of_xsdt_entries; i++) {
    if (!IsEqualStringWithSize(xsdt->entry[i], "NFIT", 4))
      continue;
    nfit = xsdt->entry[i];
    break;
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

  // EFIPrintMemoryMap();
  while (1) {
  };
}
