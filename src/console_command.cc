#include "liumos.h"

namespace ConsoleCommand {

static const GUID kByteAdressablePersistentMemory = {
    0x66F0D379,
    0xB4F3,
    0x4074,
    {0xAC, 0x43, 0x0D, 0x33, 0x18, 0xB7, 0x8C, 0xDB}};

static void ShowNFIT_PrintMemoryMappingAttr(uint64_t attr) {
  PutString("  attr: ");
  PutHex64(attr);
  PutString(" =");
  if (attr & 0x10000)
    PutString(" MORE_RELIABLE");
  if (attr & 0x08000)
    PutString(" NV");
  if (attr & 0x04000)
    PutString(" XP");
  if (attr & 0x02000)
    PutString(" RP");
  if (attr & 0x01000)
    PutString(" WP");
  if (attr & 0x00010)
    PutString(" UCE");
  if (attr & 0x00008)
    PutString(" WB");
  if (attr & 0x00004)
    PutString(" WT");
  if (attr & 0x00002)
    PutString(" WC");
  if (attr & 0x00001)
    PutString(" UC");
  PutChar('\n');
}

static void ShowNFIT_PrintMemoryTypeGUID(ACPI::NFIT_SPARange* spa) {
  GUID* type_guid = reinterpret_cast<GUID*>(&spa->address_range_type_guid);
  PutString("  type:");
  if (IsEqualGUID(type_guid, &kByteAdressablePersistentMemory))
    PutString(" ByteAddressablePersistentMemory");
  else
    PutGUID(type_guid);
  PutChar('\n');
}

void ShowNFIT() {
  using namespace ACPI;
  if (!nfit) {
    PutString("NFIT not found\n");
    return;
  }
  PutString("NFIT found\n");
  PutStringAndHex("NFIT Size in bytes", nfit->length);
  for (int i = 0; i < (int)(nfit->length - offsetof(NFIT, entry)) / 2;
       i += nfit->entry[i + 1] / 2) {
    NFITStructureType type = static_cast<NFITStructureType>(nfit->entry[i]);
    if (type == NFITStructureType::kSystemPhysicalAddressRangeStructure) {
      NFIT_SPARange* spa_range =
          reinterpret_cast<NFIT_SPARange*>(&nfit->entry[i]);
      PutStringAndHex("SPARange #", spa_range->spa_range_structure_index);
      PutStringAndHex("  Base", spa_range->system_physical_address_range_base);
      PutStringAndHex("  Length",
                      spa_range->system_physical_address_range_length);
      ShowNFIT_PrintMemoryMappingAttr(
          spa_range->address_range_memory_mapping_attribute);
      ShowNFIT_PrintMemoryTypeGUID(spa_range);
    } else if (type == NFITStructureType::kNVDIMMRegionMappingStructure) {
      NFIT_RegionMapping* rmap =
          reinterpret_cast<NFIT_RegionMapping*>(&nfit->entry[i]);
      PutString("Region Mapping\n");
      PutStringAndHex("  NFIT Device Handle #", rmap->nfit_device_handle);
      PutStringAndHex("  NVDIMM phys ID", rmap->nvdimm_physical_id);
      PutStringAndHex("  NVDIMM region ID", rmap->nvdimm_region_id);
      PutStringAndHex("  SPARange struct index",
                      rmap->spa_range_structure_index);
      PutStringAndHex("  ControlRegion struct index",
                      rmap->nvdimm_control_region_struct_index);
      PutStringAndHex("  region size", rmap->nvdimm_region_size);
      PutStringAndHex("  region offset", rmap->region_offset);
      PutStringAndHex("  NVDIMM phys addr region base",
                      rmap->nvdimm_physical_address_region_base);
      PutStringAndHex("  NVDIMM interleave_structure_index",
                      rmap->interleave_structure_index);
      PutStringAndHex("  NVDIMM interleave ways", rmap->interleave_ways);
      PutStringAndHex("  NVDIMM state flags", rmap->nvdimm_state_flags);
    } else if (type == NFITStructureType::kNVDIMMControlRegionStructure) {
      NFIT_ControlRegionStruct* ctrl_region =
          reinterpret_cast<NFIT_ControlRegionStruct*>(&nfit->entry[i]);
      PutStringAndHex("Control Region Struct #",
                      ctrl_region->nvdimm_control_region_struct_index);
    } else {
      PutStringAndHex("Unknown entry. type", static_cast<int>(type));
    }
  }
  PutString("NFIT end\n");
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
