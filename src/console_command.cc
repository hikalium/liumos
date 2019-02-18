#include "liumos.h"

namespace ConsoleCommand {

void ShowNFIT() {
  using namespace ACPI;
  if (!nfit) {
    PutString("NFIT not found\n");
    return;
  }
  PutString("NFIT found\n");
  PutStringAndHex("NFIT Size", nfit->length);
  PutStringAndHex("First NFIT Structure Type", nfit->entry[0]);
  PutStringAndHex("First NFIT Structure Size", nfit->entry[1]);
  if (static_cast<NFITStructureType>(nfit->entry[0]) ==
      NFITStructureType::kSystemPhysicalAddressRangeStructure) {
    NFIT_SPARange* spa_range = (NFIT_SPARange*)&nfit->entry[0];
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

void PutMPSINTIFlags(uint8_t flags) {
  PutString(" polarity=");
  uint8_t polarity = flags & 3;
  if (polarity == 0b00)
    PutString("same_as_bus_spec");
  if (polarity == 0b01)
    PutString("active-high");
  if (polarity == 0b10)
    PutString("reserved");
  if (polarity == 0b11)
    PutString("active-low");
  PutString(" trigger=");
  uint8_t trigger = (flags >> 2) & 3;
  if (trigger == 0b00)
    PutString("same_as_bus_spec");
  if (trigger == 0b01)
    PutString("edge");
  if (trigger == 0b10)
    PutString("reserved");
  if (trigger == 0b11)
    PutString("level");
}

void ShowMADT() {
  using namespace ACPI;
  for (int i = 0; i < (int)(madt->length - offsetof(MADT, entries));
       i += madt->entries[i + 1]) {
    uint8_t type = madt->entries[i];
    PutString("MADT(type=0x");
    PutHex64(type);
    PutString(") ");

    if (type == kProcessorLocalAPICInfo) {
      PutString("Processor 0x");
      PutHex64(madt->entries[i + 2]);
      PutString(" local_apic_id = 0x");
      PutHex64(madt->entries[i + 3]);
      PutString((madt->entries[i + 4] & 1) ? " Enabled" : "Disabled");
    } else if (type == kIOAPICInfo) {
      PutString("IOAPIC id=0x");
      PutHex64(madt->entries[i + 2]);
      PutString(" base=0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutString(" gsi_base=0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 8]);
    } else if (type == kInterruptSourceOverrideInfo) {
      PutString("IRQ override: 0x");
      PutHex64(madt->entries[i + 3]);
      PutString(" -> 0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutMPSINTIFlags(madt->entries[i + 8]);
    } else if (type == kLocalAPICNMIStruct) {
      PutString("LAPIC NMI uid=0x");
      PutHex64(madt->entries[i + 2]);
      PutMPSINTIFlags(madt->entries[i + 3]);
      PutString(" LINT#=0x");
      PutHex64(madt->entries[i + 5]);
    } else if (type == kProcessorLocalx2APICStruct) {
      PutString("x2APIC id=0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutString((madt->entries[i + 8] & 1) ? " Enabled" : "Disabled");
      PutString(" acpi_processor_uid=0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 12]);
    } else if (type == kLocalx2APICNMIStruct) {
      PutString("x2APIC NMI");
      PutMPSINTIFlags(madt->entries[i + 2]);
      PutString(" acpi_processor_uid=0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutString(" LINT#=0x");
      PutHex64(madt->entries[i + 8]);
    }
    PutString("\n");
  }
}

void ShowEFIMemoryMap() {
  PutStringAndHex("Map entries", efi_memory_map.GetNumberOfEntries());
  for (int i = 0; i < efi_memory_map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = efi_memory_map.GetDescriptor(i);
    desc->Print();
    PutString("\n");
  }
}

void Free() {
  page_allocator->Print();
}

}  // namespace ConsoleCommand
