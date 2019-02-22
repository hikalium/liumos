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
  kNVDIMMRegionMappingStructure,
  kInterleaveStructure,
  kSMBIOSManagementInformationStructure,
  kNVDIMMControlRegionStructure,
  kNVDIMMBlockDataWindowRegionStructure,
  kFlushHintAddressStructure,
  kPlatformCapabilitiesStructure,
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
static_assert(offsetof(NFIT, entry) == 40);

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

packed_struct NFIT_RegionMapping {
  uint16_t type;
  uint16_t length;
  uint32_t nfit_device_handle;
  uint16_t nvdimm_physical_id;
  uint16_t nvdimm_region_id;
  uint16_t spa_range_structure_index;
  uint16_t nvdimm_control_region_struct_index;
  uint64_t nvdimm_region_size;
  uint64_t region_offset;
  uint64_t nvdimm_physical_address_region_base;
  uint16_t interleave_structure_index;
  uint16_t interleave_ways;
  uint16_t nvdimm_state_flags;
  uint16_t reserved;
};
static_assert(sizeof(NFIT_RegionMapping) == 48);

packed_struct NFIT_InterleaveStructure {
  uint16_t type;
  uint16_t length;
  uint16_t interleave_struct_index;
  uint16_t reserved;
  uint32_t num_of_lines_described;
  uint32_t line_size;
  uint32_t line_offsets[1];
};
static_assert(offsetof(NFIT_InterleaveStructure, line_offsets) == 16);

packed_struct NFIT_FlushHintAddressStructure {
  uint16_t type;
  uint16_t length;
  uint32_t nfit_device_handle;
  uint16_t num_of_flush_hint_addresses;
  uint16_t reserved[3];
  uint64_t flush_hint_addresses[1];
};
static_assert(offsetof(NFIT_FlushHintAddressStructure, flush_hint_addresses) ==
              16);

packed_struct NFIT_PlatformCapabilities {
  uint16_t type;
  uint16_t length;
  uint8_t highest_valid_cap_bit;
  uint8_t reserved0[3];
  uint32_t capabilities;
  uint32_t reserved1;
};
static_assert(sizeof(NFIT_PlatformCapabilities) == 16);

packed_struct NFIT_ControlRegionStruct {
  uint16_t type;
  uint16_t length;
  uint16_t nvdimm_control_region_struct_index;
};

packed_struct SRAT {
  static const uint8_t kEntryTypeLAPICAffinity = 0x00;
  packed_struct LAPICAffinity {
    uint8_t type;
    uint8_t length;
    uint8_t proximity_domain_low;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t local_sapic_eid;
    uint8_t proximity_domain_high[3];
    uint32_t clock_domain;
  };
  static_assert(sizeof(LAPICAffinity) == 16);

  static const uint8_t kEntryTypeMemoryAffinity = 0x01;
  packed_struct MemoryAffinity {
    uint8_t type;
    uint8_t length;
    uint32_t proximity_domain;
    uint16_t reserved0;
    uint64_t base_address;
    uint64_t size;
    uint32_t reserved1;
    uint32_t flags;
    uint64_t reserved2;
  };
  static_assert(sizeof(MemoryAffinity) == 40);

  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
  uint32_t reserved[3];
  uint8_t entry[1];
};
static_assert(offsetof(SRAT, entry) == 48);

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
  kProcessorLocalAPICInfo = 0x0,
  kIOAPICInfo = 0x1,
  kInterruptSourceOverrideInfo = 0x2,
  kLocalAPICNMIStruct = 0x4,
  kProcessorLocalx2APICStruct = 0x9,
  kLocalx2APICNMIStruct = 0xA,
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
extern SRAT* srat;

void DetectTables();
}  // namespace ACPI
