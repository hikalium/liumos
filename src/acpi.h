#pragma once

#include "generic.h"
#include "guid.h"

namespace ACPI {

constexpr int kDescriptionHeaderSize = 36;

struct XSDT;

packed_struct RSDT {
  char signature[8];
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  uint32_t length;
  XSDT* xsdt;
  uint8_t extended_checksum;
  uint8_t reserved;
};

packed_struct XSDT {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
  void* entry[1];
};

enum class NFITStructureType : int {
  kSystemPhysicalAddressRangeStructure,
  kMemoryDeviceToSystemAddressRangeMapStructure,
  kInterleaveStructure,
  kSMBIOSManagementInformationStructure,
  kNVDIMMControlRegionStructure,
  kNVDIMMBlockDataWindowRegionStructure,
  kFlushHintAddressStructure,
};

packed_struct NFIT {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
  uint32_t reserved;
  uint16_t entry[1];
};

packed_struct NFIT_SPARange {
  // System Physical Address Range
  uint16_t type;
  uint16_t length;
  uint16_t spa_range_structure_index;
  uint16_t flags;
  uint32_t reserved;
  uint32_t proximity_domain;
  uint64_t address_range_type_guid[2];
  uint64_t system_physical_address_range_base;
  uint64_t system_physical_address_range_length;
  uint64_t address_range_memory_mapping_attribute;
};

packed_struct GAS {
  // Generic Address Structure
  uint8_t address_space_id;
  uint8_t register_bit_width;
  uint8_t register_bit_offset;
  uint8_t reserved;
  void* address;
};

packed_struct HPET {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;

  uint32_t event_timer_block_id;
  GAS base_address;
  uint8_t hpet_number;
  uint16_t main_counter_minimum_clock_tick_in_periodic_mode;
  uint8_t page_protection_and_oem_attribute;
};

enum MADTStructureType {
  kProcessorLocalAPICInfo = 0,
  kIOAPICInfo = 1,
  kInterruptSourceOverrideInfo = 2,
  kLocalAPICNMIStruct = 4,
};

packed_struct MADT {
  // Multiple APIC Description Table
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;

  uint32_t local_apic_address;
  uint32_t flags;
  uint8_t entries[1];
};

extern RSDT* rsdt;
extern MADT* madt;
extern HPET* hpet;
extern NFIT* nfit;

void DetectTables();
}  // namespace ACPI
