#include "liumos.h"

namespace ConsoleCommand {

void ShowNFIT() {
  if (!nfit) {
    PutString("NFIT not found\n");
    return;
  }
  PutString("NFIT found\n");
  PutStringAndHex("NFIT Size", nfit->length);
  PutStringAndHex("First NFIT Structure Type", nfit->entry[0]);
  PutStringAndHex("First NFIT Structure Size", nfit->entry[1]);
  if (static_cast<ACPI_NFITStructureType>(nfit->entry[0]) ==
      ACPI_NFITStructureType::kSystemPhysicalAddressRangeStructure) {
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
    PutStringAndHex("\nPointer in PMEM Region: ", p);
    PutStringAndHex("PMEM Previous value: ", *p);
    (*p)++;
    PutStringAndHex("PMEM value after write: ", *p);
  }
}

}  // namespace ConsoleCommand
