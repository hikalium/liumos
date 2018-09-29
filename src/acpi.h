#pragma once

#include "generic.h"
#include "guid.h"

#define packed_struct struct __attribute__((__packed__))
#define ACPI_DESCRIPTION_HEADER_SIZE 36

typedef struct ACPI_ROOT_SYSTEM_DESCRIPTION_POINTER ACPI_RSDP;
typedef struct ACPI_EXTENDED_SYSTEM_DESCRIPTION_TABLE ACPI_XSDT;
typedef struct ACPI_NVDIMM_FIRMWARE_INTERFACE_TABLE ACPI_NFIT;
typedef struct ACPI_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE
    ACPI_NFIT_SPARange;
typedef struct ACPI_GENERIC_ADDRESS_STRUCTURE ACPI_GAS;
typedef struct ACPI_HPET_DESCRIPTION_TABLE ACPI_HPET;
typedef struct HPET_REGISTER_SPACE HPETRegisterSpace;

packed_struct ACPI_ROOT_SYSTEM_DESCRIPTION_POINTER {
  char signature[8];
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  uint32_t length;
  ACPI_XSDT* xsdt;
  uint8_t extended_checksum;
  uint8_t reserved;
};

packed_struct ACPI_EXTENDED_SYSTEM_DESCRIPTION_TABLE {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
  void* entry[];
};

enum ACPI_NFITStructureType {
  kSystemPhysicalAddressRangeStructure,
  kMemoryDeviceToSystemAddressRangeMapStructure,
  kInterleaveStructure,
  kSMBIOSManagementInformationStructure,
  kNVDIMMControlRegionStructure,
  kNVDIMMBlockDataWindowRegionStructure,
  kFlushHintAddressStructure,
};

packed_struct ACPI_NVDIMM_FIRMWARE_INTERFACE_TABLE {
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
  uint16_t entry[];
};

packed_struct ACPI_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE {
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

packed_struct ACPI_GENERIC_ADDRESS_STRUCTURE {
  uint8_t address_space_id;
  uint8_t register_bit_width;
  uint8_t register_bit_offset;
  uint8_t reserved;
  void* address;
};

packed_struct ACPI_HPET_DESCRIPTION_TABLE {
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
  ACPI_GAS base_address;
  uint8_t hpet_number;
  uint16_t main_counter_minimum_clock_tick_in_periodic_mode;
  uint8_t page_protection_and_oem_attribute;
};
