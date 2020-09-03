#pragma once

#include "apic.h"
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

packed_struct NFIT {
  packed_struct Entry {
    static const uint16_t kTypeSPARangeStructure = 0x00;
    static const uint16_t kTypeNVDIMMRegionMappingStructure = 0x01;
    static const uint16_t kTypeInterleaveStructure = 0x02;
    static const uint16_t kTypeNVDIMMControlRegionStructure = 0x04;
    static const uint16_t kTypeFlushHintAddressStructure = 0x06;
    static const uint16_t kTypePlatformCapabilitiesStructure = 0x07;

    uint16_t type;
    uint16_t length;
  };

  class Iterator {
   public:
    Iterator(Entry* e) : current_(e){};
    void operator++() {
      current_ = reinterpret_cast<Entry*>(reinterpret_cast<uint64_t>(current_) +
                                          current_->length);
    }
    Entry& operator*() { return *current_; }
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
      return lhs.current_ != rhs.current_;
    }

   private:
    Entry* current_;
  };
  Iterator begin() { return Iterator(entry); }
  Iterator end() {
    return Iterator(
        reinterpret_cast<Entry*>(reinterpret_cast<uint64_t>(this) + length));
  }

  packed_struct SPARange {
    // System Physical Address Range
    static const GUID kByteAddressablePersistentMemory;
    static const GUID kNVDIMMControlRegion;

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

  packed_struct RegionMapping {
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
  static_assert(sizeof(RegionMapping) == 48);

  packed_struct InterleaveStructure {
    uint16_t type;
    uint16_t length;
    uint16_t interleave_struct_index;
    uint16_t reserved;
    uint32_t num_of_lines_described;
    uint32_t line_size;
    uint32_t line_offsets[1];
  };
  static_assert(offsetof(InterleaveStructure, line_offsets) == 16);

  packed_struct FlushHintAddressStructure {
    uint16_t type;
    uint16_t length;
    uint32_t nfit_device_handle;
    uint16_t num_of_flush_hint_addresses;
    uint16_t reserved[3];
    uint64_t flush_hint_addresses[1];
  };
  static_assert(offsetof(FlushHintAddressStructure, flush_hint_addresses) ==
                16);

  packed_struct PlatformCapabilities {
    uint16_t type;
    uint16_t length;
    uint8_t highest_valid_cap_bit;
    uint8_t reserved0[3];
    uint32_t capabilities;
    uint32_t reserved1;
  };
  static_assert(sizeof(PlatformCapabilities) == 16);

  packed_struct ControlRegionStruct {
    uint16_t type;
    uint16_t length;
    uint16_t nvdimm_control_region_struct_index;
  };

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
  Entry entry[1];
};
static_assert(offsetof(NFIT, entry) == 40);

packed_struct SRAT {
  packed_struct Entry {
    static const uint8_t kTypeLAPICAffinity = 0x00;
    static const uint8_t kTypeMemoryAffinity = 0x01;
    static const uint8_t kTypeLx2APICAffinity = 0x02;

    uint8_t type;
    uint8_t length;
  };

  class Iterator {
   public:
    Iterator(Entry* e) : current_(e){};
    void operator++() {
      current_ = reinterpret_cast<Entry*>(reinterpret_cast<uint64_t>(current_) +
                                          current_->length);
    }
    Entry& operator*() { return *current_; }
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
      return lhs.current_ != rhs.current_;
    }

   private:
    Entry* current_;
  };
  Iterator begin() { return Iterator(entry); }
  Iterator end() {
    return Iterator(
        reinterpret_cast<Entry*>(reinterpret_cast<uint64_t>(this) + length));
  }

  packed_struct LAPICAffinity {
    Entry entry;
    uint8_t proximity_domain_low;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t local_sapic_eid;
    uint8_t proximity_domain_high[3];
    uint32_t clock_domain;
  };
  static_assert(sizeof(LAPICAffinity) == 16);

  packed_struct MemoryAffinity {
    Entry entry;
    uint32_t proximity_domain;
    uint16_t reserved0;
    uint64_t base_address;
    uint64_t size;
    uint32_t reserved1;
    uint32_t flags;
    uint64_t reserved2;
  };
  static_assert(sizeof(MemoryAffinity) == 40);

  packed_struct Lx2APICAffinity {
    Entry entry;
    uint16_t reserved0;
    uint32_t proximity_domain;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t clock_domain;
    uint32_t reserved1;
  };
  static_assert(sizeof(Lx2APICAffinity) == 24);

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
  Entry entry[1];

  constexpr static uint32_t kUnknownProximityDomain = 0xffffffffULL;
  uint32_t GetProximityDomainForAddrRange(uint64_t paddr, uint64_t size) {
    for (auto& it : *this) {
      if (it.type != Entry::kTypeMemoryAffinity)
        continue;
      MemoryAffinity* e = reinterpret_cast<MemoryAffinity*>(&it);
      if (e->base_address <= paddr && paddr < e->base_address + e->size &&
          paddr + size <= e->base_address + e->size) {
        return e->proximity_domain;
      }
    }
    return kUnknownProximityDomain;
  }
  uint32_t GetProximityDomainForLocalAPIC(LocalAPIC & lapic) {
    for (auto& it : *this) {
      if (it.type != SRAT::Entry::kTypeLx2APICAffinity)
        continue;
      SRAT::Lx2APICAffinity* e = reinterpret_cast<SRAT::Lx2APICAffinity*>(&it);
      if (e->x2apic_id != lapic.GetID())
        continue;
      return e->proximity_domain;
    }
    for (auto& it : *this) {
      if (it.type != SRAT::Entry::kTypeLAPICAffinity)
        continue;
      SRAT::LAPICAffinity* e = reinterpret_cast<SRAT::LAPICAffinity*>(&it);
      if (e->apic_id != lapic.GetID())
        continue;
      return e->proximity_domain_low | (e->proximity_domain_high[0] << 8) |
             (e->proximity_domain_high[1] << 16) |
             (e->proximity_domain_high[2] << 24);
    }
    return kUnknownProximityDomain;
  }
};
static_assert(offsetof(SRAT, entry) == 48);

packed_struct SLIT {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
  uint64_t num_of_system_localities;
  uint8_t entry[1];
};
static_assert(offsetof(SLIT, entry) == 44);

packed_struct GAS {
  // Generic Address Structure
  uint8_t address_space_id;
  uint8_t register_bit_width;
  uint8_t register_bit_offset;
  uint8_t reserved;
  void* address;
};
static_assert(sizeof(GAS) == 12);

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

packed_struct FADT {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oem_id[6];
  uint64_t oem_table_id;
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;

  uint32_t firmware_ctrl;
  uint32_t dsdt;
  uint8_t reserved;
  uint8_t preferred_pm_profile;
  uint16_t sci_int;
  uint32_t smi_cmd;
  uint8_t acpi_enable;
  uint8_t acpi_disable;

  uint16_t GetResetReg() {
    return reinterpret_cast<uint64_t>(
        reinterpret_cast<GAS*>(reinterpret_cast<uint64_t>(this) + 116)
            ->address);
  }
  uint8_t GetResetValue() { return reinterpret_cast<uint8_t*>(this)[128]; }
};
static_assert(sizeof(FADT) == 54);

void DetectTables();
}  // namespace ACPI
