#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "adlib.h"
#include "command_line_args.h"
#include "kernel.h"
#include "liumos.h"
#include "network.h"
#include "pci.h"
#include "pmem.h"
#include "virtio_net.h"
#include "xhci.h"

namespace ConsoleCommand {

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

static void ShowNFIT_PrintMemoryTypeGUID(ACPI::NFIT::SPARange* spa) {
  GUID* type_guid = reinterpret_cast<GUID*>(&spa->address_range_type_guid);
  PutString("  type:");
  if (IsEqualGUID(type_guid,
                  &ACPI::NFIT::SPARange::kByteAddressablePersistentMemory))
    PutString(" ByteAddressablePersistentMemory");
  else if (IsEqualGUID(type_guid, &ACPI::NFIT::SPARange::kNVDIMMControlRegion))
    PutString(" NVDIMMControlRegion");
  else
    PutGUID(type_guid);
  PutChar('\n');
}

void ShowXSDT() {
  using namespace ACPI;
  assert(liumos && liumos->acpi.rsdt && liumos->acpi.rsdt->xsdt);
  XSDT& xsdt = *liumos->acpi.rsdt->xsdt;
  const int num_of_xsdt_entries = (xsdt.length - kDescriptionHeaderSize) >> 3;

  PutString("Found 0x");
  PutHex64(num_of_xsdt_entries);
  PutString(" XSDT entries found:");
  for (int i = 0; i < num_of_xsdt_entries; i++) {
    const char* signature = static_cast<const char*>(xsdt.entry[i]);
    PutChar(' ');
    PutChar(signature[0]);
    PutChar(signature[1]);
    PutChar(signature[2]);
    PutChar(signature[3]);
  }
  PutString("\n");
}

void ShowNFIT() {
  using namespace ACPI;
  if (!liumos->acpi.nfit) {
    PutString("NFIT not found\n");
    return;
  }
  NFIT& nfit = *liumos->acpi.nfit;
  PutString("NFIT found\n");
  PutStringAndHex("NFIT Size in bytes", nfit.length);
  for (auto& it : nfit) {
    switch (it.type) {
      case NFIT::Entry::kTypeSPARangeStructure: {
        NFIT::SPARange* spa_range = reinterpret_cast<NFIT::SPARange*>(&it);
        PutStringAndHex("SPARange #", spa_range->spa_range_structure_index);
        PutStringAndHex("  Base",
                        spa_range->system_physical_address_range_base);
        PutStringAndHex("  Length",
                        spa_range->system_physical_address_range_length);
        ShowNFIT_PrintMemoryMappingAttr(
            spa_range->address_range_memory_mapping_attribute);
        ShowNFIT_PrintMemoryTypeGUID(spa_range);
      } break;
      case NFIT::Entry::kTypeNVDIMMRegionMappingStructure: {
        NFIT::RegionMapping* rmap = reinterpret_cast<NFIT::RegionMapping*>(&it);
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
      } break;
      case NFIT::Entry::kTypeInterleaveStructure: {
        NFIT::InterleaveStructure* interleave =
            reinterpret_cast<NFIT::InterleaveStructure*>(&it);
        PutStringAndHex("Interleave Struct #",
                        interleave->interleave_struct_index);
        PutStringAndHex("  Line size (in bytes)", interleave->line_size);
        PutString("  Lines = ");
        PutHex64(interleave->num_of_lines_described);
        PutString(" :");
        for (uint32_t line_index = 0;
             line_index < interleave->num_of_lines_described; line_index++) {
          PutString(" +");
          PutHex64(interleave->line_offsets[line_index]);
        }
        PutChar('\n');
      } break;
      case NFIT::Entry::kTypeNVDIMMControlRegionStructure: {
        NFIT::ControlRegionStruct* ctrl_region =
            reinterpret_cast<NFIT::ControlRegionStruct*>(&it);
        PutStringAndHex("Control Region Struct #",
                        ctrl_region->nvdimm_control_region_struct_index);
      } break;
      case NFIT::Entry::kTypeFlushHintAddressStructure: {
        NFIT::FlushHintAddressStructure* flush_hint =
            reinterpret_cast<NFIT::FlushHintAddressStructure*>(&it);
        PutStringAndHex("Flush Hint Addresses for NFIT Device handle",
                        flush_hint->nfit_device_handle);
        PutString("  Addrs = ");
        PutHex64(flush_hint->num_of_flush_hint_addresses);
        PutString(" :");
        for (uint32_t index = 0;
             index < flush_hint->num_of_flush_hint_addresses; index++) {
          PutString(" @");
          PutHex64(flush_hint->flush_hint_addresses[index]);
        }
        PutChar('\n');
      } break;
      case NFIT::Entry::kTypePlatformCapabilitiesStructure: {
        NFIT::PlatformCapabilities* caps =
            reinterpret_cast<NFIT::PlatformCapabilities*>(&it);
        const int cap_shift = 31 - caps->highest_valid_cap_bit;
        const uint32_t cap_bits =
            ((caps->capabilities << cap_shift) & 0xFFFF'FFFFUL) >> cap_shift;
        PutString("Platform Capabilities\n");
        PutStringAndBool("  Flush CPU Cache when power loss", cap_bits & 0b001);
        PutStringAndBool("  Flush Memory Controller when power loss",
                         cap_bits & 0b010);
        PutStringAndBool("  Hardware Mirroring Support", cap_bits & 0b100);
      } break;
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
  assert(liumos->acpi.madt);
  MADT& madt = *liumos->acpi.madt;
  for (int i = 0; i < (int)(madt.length - offsetof(MADT, entries));
       i += madt.entries[i + 1]) {
    uint8_t type = madt.entries[i];
    PutString("MADT(type=0x");
    PutHex64(type);
    PutString(") ");

    if (type == kProcessorLocalAPICInfo) {
      PutString("Processor 0x");
      PutHex64(madt.entries[i + 2]);
      PutString(" local_apic_id = 0x");
      PutHex64(madt.entries[i + 3]);
      PutString((madt.entries[i + 4] & 1) ? " Enabled" : "Disabled");
    } else if (type == kIOAPICInfo) {
      PutString("IOAPIC id=0x");
      PutHex64(madt.entries[i + 2]);
      PutString(" base=0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 4]);
      PutString(" gsi_base=0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 8]);
    } else if (type == kInterruptSourceOverrideInfo) {
      PutString("IRQ override: 0x");
      PutHex64(madt.entries[i + 3]);
      PutString(" -> 0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 4]);
      PutMPSINTIFlags(madt.entries[i + 8]);
    } else if (type == kLocalAPICNMIStruct) {
      PutString("LAPIC NMI uid=0x");
      PutHex64(madt.entries[i + 2]);
      PutMPSINTIFlags(madt.entries[i + 3]);
      PutString(" LINT#=0x");
      PutHex64(madt.entries[i + 5]);
    } else if (type == kProcessorLocalx2APICStruct) {
      PutString("x2APIC id=0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 4]);
      PutString((madt.entries[i + 8] & 1) ? " Enabled" : " Disabled");
      PutString(" acpi_processor_uid=0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 12]);
    } else if (type == kLocalx2APICNMIStruct) {
      PutString("x2APIC NMI");
      PutMPSINTIFlags(madt.entries[i + 2]);
      PutString(" acpi_processor_uid=0x");
      PutHex64(*(uint32_t*)&madt.entries[i + 4]);
      PutString(" LINT#=0x");
      PutHex64(madt.entries[i + 8]);
    }
    PutString("\n");
  }
}

void ShowSRAT() {
  using namespace ACPI;
  if (!liumos->acpi.srat) {
    PutString("SRAT not found.\n");
    return;
  }
  SRAT& srat = *liumos->acpi.srat;
  PutString("SRAT found.\n");
  for (auto& it : srat) {
    PutString("SRAT(type=0x");
    PutHex64(it.type);
    PutString(") ");

    if (it.type == SRAT::Entry::kTypeLAPICAffinity) {
      SRAT::LAPICAffinity* e = reinterpret_cast<SRAT::LAPICAffinity*>(&it);
      PutString("LAPIC Affinity id=0x");
      PutHex64(e->apic_id);
      const uint32_t proximity_domain = e->proximity_domain_low |
                                        (e->proximity_domain_high[0] << 8) |
                                        (e->proximity_domain_high[1] << 16) |
                                        (e->proximity_domain_high[2] << 24);
      PutString(" proximity_domain=0x");
      PutHex64(proximity_domain);
    } else if (it.type == SRAT::Entry::kTypeMemoryAffinity) {
      SRAT::MemoryAffinity* e = reinterpret_cast<SRAT::MemoryAffinity*>(&it);
      PutString("Memory Affinity [");
      PutHex64ZeroFilled(e->base_address);
      PutString(", ");
      PutHex64ZeroFilled(e->base_address + e->size);
      PutString(")\n  proximity_domain=0x");
      PutHex64(e->proximity_domain);
      if (e->flags & 1)
        PutString(" Enabled");
      if (e->flags & 2)
        PutString(" Hot-pluggable");
      if (e->flags & 4)
        PutString(" Non-volatile");
    } else if (it.type == SRAT::Entry::kTypeLx2APICAffinity) {
      SRAT::Lx2APICAffinity* e = reinterpret_cast<SRAT::Lx2APICAffinity*>(&it);
      PutString("Lx2APIC Affinity id=0x");
      PutHex64(e->x2apic_id);
      PutString(" proximity_domain=0x");
      PutHex64(e->proximity_domain);
    }
    PutString("\n");
  }
}

void ShowSLIT() {
  using namespace ACPI;
  if (!liumos->acpi.slit) {
    PutString("SLIT not found.\n");
    return;
  }
  SLIT& slit = *liumos->acpi.slit;
  PutString("SLIT found.\n");
  PutStringAndHex("num_of_system_localities", slit.num_of_system_localities);
  for (uint64_t y = 0; y < slit.num_of_system_localities; y++) {
    for (uint64_t x = 0; x < slit.num_of_system_localities; x++) {
      if (x)
        PutString(", ");
      PutHex64(slit.entry[y * slit.num_of_system_localities + x]);
    }
    PutString("\n");
  }
}

void ShowFADT() {
  using namespace ACPI;
  if (!liumos->acpi.fadt) {
    PutString("FADT not found.\n");
    return;
  }
  FADT& fadt = *liumos->acpi.fadt;
  PutString("FADT found.\n");
  PutStringAndHex("smi_cmd", fadt.smi_cmd);
  PutStringAndHex("acpi_enable", fadt.acpi_enable);
  PutStringAndHex("acpi_disable", fadt.acpi_disable);
  PutStringAndHex("reset_register", fadt.GetResetReg());
  PutStringAndHex("reset_value", fadt.GetResetValue());
}

void Reset() {
  using namespace ACPI;
  if (!liumos->acpi.fadt) {
    PutString("FADT not found.\n");
    return;
  }
  FADT& fadt = *liumos->acpi.fadt;
  WriteIOPort8(fadt.GetResetReg(), fadt.GetResetValue());
  for (;;) {
  }
}

void EnableACPI() {
  using namespace ACPI;
  if (!liumos->acpi.fadt) {
    PutString("FADT not found.\n");
    return;
  }
  FADT& fadt = *liumos->acpi.fadt;
  WriteIOPort8(fadt.smi_cmd, fadt.acpi_enable);
  PutString("ACPI enable requested\n");
}

void ShowEFIMemoryMap() {
  EFI::MemoryMap& map = *liumos->efi_memory_map;
  PutStringAndHex("Map entries", map.GetNumberOfEntries());
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    desc->Print();
    PutString("\n");
  }
}

void Free() {
  PutString("DRAM Free List:\n");
  GetSystemDRAMAllocator().Print();
}

void label(uint64_t i) {
  PutString("0x");
  PutHex64(i);
  PutString(",");
}

uint64_t get_seconds() {
  return HPET::GetInstance().ReadMainCounterValue();
}

void TestMem(
    PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>& allocator,
    uint32_t proximity_domain) {
  constexpr uint64_t kRangeMin = 1ULL << 10;
  constexpr uint64_t kRangeMax = 1ULL << 24;
  PutStringAndHex("Test memory on proximity_domain", proximity_domain);
  constexpr uint64_t array_size_in_pages =
      (sizeof(int) * kRangeMax + kPageSize - 1) >> kPageSizeExponent;
  int* array = allocator.AllocPagesInProximityDomain<int*>(array_size_in_pages,
                                                           proximity_domain);
  if (!array) {
    PutString("Alloc failed.");
    return;
  }
  PutString("Array range: ");
  PutAddressRange(array, array_size_in_pages << kPageSizeExponent);
  PutString("\n");

  int32_t* test_mem_map_addr =
      reinterpret_cast<int32_t*>(0xFFFF'FFFE'0000'0000ULL);

  CreatePageMapping(GetSystemDRAMAllocator(), GetKernelPML4(),
                    reinterpret_cast<uint64_t>(test_mem_map_addr),
                    reinterpret_cast<uint64_t>(array),
                    array_size_in_pages << kPageSizeExponent,
                    kPageAttrPresent | kPageAttrWritable | kPageAttrUser |
                        kPageAttrCacheDisable | kPageAttrWriteThrough);

  uint64_t nextstep, i, index;
  uint64_t csize, stride;
  uint64_t steps, tsteps;
  uint64_t t0, t1, tick_sum_overall, tick_sum_loop_only;
  uint64_t kDurationTick =
      (uint64_t)(0.1 * 1e15) / HPET::GetInstance().GetFemtosecondPerCount();

  PutString(" ,");
  for (stride = 1; stride <= kRangeMax / 2; stride = stride * 2)
    label(stride * sizeof(int));
  PutString("\n");

  ClearIntFlag();
  for (csize = kRangeMin; csize <= kRangeMax; csize = csize * 2) {
    label(csize * sizeof(int));
    for (stride = 1; stride <= csize / 2; stride = stride * 2) {
      for (index = 0; index < csize; index = index + stride) {
        test_mem_map_addr[index] = (int)(index + stride);
      }
      test_mem_map_addr[index - stride] = 0;

      // measure time spent on (reading data from memory) + (loop, instrs,
      // etc...)
      steps = 0;
      nextstep = 0;
      t0 = get_seconds();
      do {
        for (i = stride; i != 0; i = i - 1) {
          nextstep = 0;
          do
            nextstep = test_mem_map_addr[nextstep];
          while (nextstep != 0);
        }
        steps = steps + 1;
        t1 = get_seconds();
      } while ((t1 - t0) < kDurationTick);  // originary 20.0
      tick_sum_overall = t1 - t0;

      // measure time spent on (loop, instrs, etc...) only
      tsteps = 0;
      t0 = get_seconds();
      do {
        for (i = stride; i != 0; i = i - 1) {
          index = 0;
          do
            index = index + stride;
          while (index < csize);
        }
        tsteps = tsteps + 1;
        t1 = get_seconds();
      } while (tsteps < steps);
      tick_sum_loop_only = t1 - t0;

      // avoid negative value
      if (tick_sum_loop_only >= tick_sum_overall) {
        tick_sum_loop_only = tick_sum_overall;
      }

      const uint64_t tick_sum_of_mem_read =
          tick_sum_overall - tick_sum_loop_only;
      const uint64_t pico_second_per_mem_read =
          tick_sum_of_mem_read * HPET::GetInstance().GetFemtosecondPerCount() /
          (steps * csize) / 1000;
      PutString("0x");
      PutHex64(pico_second_per_mem_read > 0 ? pico_second_per_mem_read : 1);
      PutString(", ");
    };
    PutString("\n");
  };
  StoreIntFlag();
}

void TestMemWrite(
    PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy> allocator,
    uint32_t proximity_domain) {
  constexpr uint64_t kRangeMin = 1ULL << 10;
  constexpr uint64_t kRangeMax = 1ULL << 24;
  PutStringAndHex("Test memory on proximity_domain", proximity_domain);
  constexpr uint64_t array_size_in_pages =
      (sizeof(int) * kRangeMax + kPageSize - 1) >> kPageSizeExponent;
  int* array = allocator.AllocPagesInProximityDomain<int*>(array_size_in_pages,
                                                           proximity_domain);
  if (!array) {
    PutString("Alloc failed.");
    return;
  }
  PutString("Array range: ");
  PutAddressRange(array, array_size_in_pages << kPageSizeExponent);
  PutString("\n");

  volatile int32_t* test_mem_map_addr =
      reinterpret_cast<volatile int32_t*>(0xFFFF'FFFE'0000'0000ULL);

  CreatePageMapping(GetSystemDRAMAllocator(), GetKernelPML4(),
                    reinterpret_cast<uint64_t>(test_mem_map_addr),
                    reinterpret_cast<uint64_t>(array),
                    array_size_in_pages << kPageSizeExponent,
                    kPageAttrPresent | kPageAttrWritable | kPageAttrUser |
                        kPageAttrCacheDisable | kPageAttrWriteThrough);

  uint64_t nextstep, i, index;
  uint64_t csize, stride;
  uint64_t steps, tsteps;
  uint64_t t0, t1;
  uint64_t kDurationTick =
      (uint64_t)(0.1 * 1e15) / HPET::GetInstance().GetFemtosecondPerCount();

  PutString(" ,");
  for (stride = 1; stride <= kRangeMax / 2; stride = stride * 2)
    label(stride * sizeof(int));
  PutString("\n");

  ClearIntFlag();
  for (csize = kRangeMin; csize <= kRangeMax; csize = csize * 2) {
    label(csize * sizeof(int));
    for (stride = 1; stride <= csize / 2; stride = stride * 2) {
      for (index = 0; index < csize; index = index + stride) {
        test_mem_map_addr[index] = (int)(index + stride);
      }
      test_mem_map_addr[index - stride] = 0;

      // measure time spent on (memory access) + (loop, instrs, etc...)
      steps = 0;
      nextstep = 0;
      t0 = get_seconds();
      do {
        for (i = stride; i != 0; i = i - 1) {
          index = 0;
          do {
            test_mem_map_addr[nextstep] = (int32_t)index;  // HERE
            index = index + stride;
          } while (index < csize);
        }
        steps = steps + 1;
        t1 = get_seconds();
      } while ((t1 - t0) < kDurationTick);
      const uint64_t tick_sum_overall = t1 - t0;

      // measure time spent on (loop, instrs, etc...) only
      tsteps = 0;
      t0 = get_seconds();
      do {
        for (i = stride; i != 0; i = i - 1) {
          index = 0;
          do
            index = index + stride;
          while (index < csize);
        }
        tsteps = tsteps + 1;
        t1 = get_seconds();
      } while (tsteps < steps);
      const uint64_t tick_sum_loop_only = t1 - t0;

      // avoid negative value
      if (tick_sum_loop_only >= tick_sum_overall) {
        PutString(", ");
        continue;
      }

      const uint64_t tick_sum_access_only =
          tick_sum_overall - tick_sum_loop_only;
      const uint64_t pico_second_per_mem_read =
          tick_sum_access_only * HPET::GetInstance().GetFemtosecondPerCount() /
          (steps * csize) / 1000;
      if (pico_second_per_mem_read == 0) {
        PutString(", ");
        continue;
      }
      PutString("0x");
      PutHex64(pico_second_per_mem_read);
      PutString(", ");
    };
    PutString("\n");
  };
  StoreIntFlag();
}

void Time() {
  PutStringAndHex("HPET main counter",
                  HPET::GetInstance().ReadMainCounterValue());
}

void Version() {
  kprintf("liumOS version: %s\n", GetVersionStr());
}

static void ListPCIDevices() {
  PutString("lspci:\n");
  PCI::GetInstance().PrintDevices();
}

static void PlayMIDI(const char* file_name) {
  int idx = GetLoaderInfo().FindFile(file_name);
  if (idx == -1) {
    PutString("midi file not found.\n");
    return;
  }
  PlayMIDI(GetLoaderInfo().root_files[idx]);
}

uint8_t ReadCMOS(uint8_t reg_id) {
  WriteIOPort8(0x70, (1 << 7 /* NMI Disable */) | reg_id);
  return ReadIOPort8(0x71);
}

const char* tree_aa[10] = {
    "        X",     "        A",      "       / \\",     "      / u \\",
    "     / l   \\", "    / O  i  \\", "   /    S  m \\", "   -----------",
    "       | |",    "      |---|",
};

void Date() {
  uint8_t bcd_year = ReadCMOS(0x09);
  uint8_t bcd_month = ReadCMOS(0x08);
  uint8_t bcd_day_of_month = ReadCMOS(0x07);
  kprintf("%02X-%02X-%02X %02X:%02X:%02X\n", bcd_year, bcd_month,
          bcd_day_of_month, ReadCMOS(0x04), ReadCMOS(0x02), ReadCMOS(0x00));
  if (bcd_month == 0x12 && bcd_day_of_month == 0x25) {
    kprintf("\n");
    for (int i = 0; i < 10; i++) {
      kprintf(tree_aa[i]);
      kprintf("\n");
    }
    kprintf("\n>>>> Happy Holidays! <<<<\n\n");
  }
}

void Run(TextBox& tbox) {
  const char* raw = tbox.GetRecordedString();
  char line[TextBox::kSizeOfBuffer + 1];
  bool background = raw[strlen(raw) - 1] == '&';
  if (background) {
    memmove(line, raw, strlen(raw) - 1);
    line[strlen(raw) - 1] = '\0';
  } else {
    memmove(line, raw, strlen(raw));
    line[strlen(raw)] = '\0';
  }
  CommandLineArgs args;
  if (!args.Parse(line)) {
    PutString("Failed to parse command line\n");
    return;
  }
  if (IsEqualString(args.GetArg(0), "date")) {
    Date();
    return;
  }
  if (IsEqualString(args.GetArg(0), "gateway")) {
    if (args.GetNumOfArgs() != 3) {
      PutString("Usage: gateway <ip> <netmask>");
      return;
    }
    return;
  }
  if (IsEqualString(args.GetArg(0), "ip")) {
    Virtio::Net& net = Virtio::Net::GetInstance();
    auto ip_addr = net.GetSelfIPv4Addr();
    auto mac_addr = net.GetSelfEtherAddr();

    auto& network = Network::GetInstance();
    auto mask = network.GetIPv4NetMask();
    auto gateway_ip = network.GetIPv4DefaultGateway();

    StringBuffer<128> line;
    ip_addr.WriteString(line);
    line.WriteString(" eth ");
    mac_addr.WriteString(line);
    line.WriteString(" mask ");
    mask.WriteString(line);
    line.WriteString(" gateway ");
    gateway_ip.WriteString(line);
    line.WriteChar('\n');
    PutString(line.GetString());
    return;
  }
  if (IsEqualString(args.GetArg(0), "arp")) {
    if (args.GetNumOfArgs() == 2) {
      SendARPRequest(args.GetArg(1));
      return;
    }
    auto& net = Network::GetInstance();
    kprintf("%d entries found:\n", net.GetARPTable().size());
    for (const auto& e : net.GetARPTable()) {
      e.first.Print();
      PutString(" -> ");
      e.second.Print();
      PutString("\n");
    }
    return;
  }
  if (IsEqualString(args.GetArg(0), "dhcp")) {
    SendDHCPRequest();
    kprintf("DHCP request sent.\n");
    return;
  }
  if (IsEqualString(line, "hello")) {
    PutString("Hello, world!\n");
  } else if (IsEqualString(line, "reset")) {
    Reset();
  } else if (IsEqualString(line, "enable acpi")) {
    EnableACPI();
  } else if (IsEqualString(line, "show xsdt")) {
    ShowXSDT();
  } else if (IsEqualString(line, "show nfit")) {
    ShowNFIT();
  } else if (IsEqualString(line, "show madt")) {
    ShowMADT();
  } else if (IsEqualString(line, "show srat")) {
    ShowSRAT();
  } else if (IsEqualString(line, "show slit")) {
    ShowSLIT();
  } else if (IsEqualString(line, "show fadt")) {
    ShowFADT();
  } else if (IsEqualString(line, "show mmap")) {
    ShowEFIMemoryMap();
  } else if (IsEqualString(line, "show hpet")) {
    HPET::GetInstance().Print();
  } else if (IsEqualString(line, "show cpu")) {
    PutString("APIC Mode: ");
    PutString(liumos->bsp_local_apic->Isx2APIC() ? "x2APIC" : "xAPIC");
    PutString("\n");
    PutStringAndHex("BSP APIC ID", liumos->bsp_local_apic->GetID());
    if (liumos->acpi.srat)
      PutStringAndHex("  proximity_domain",
                      liumos->acpi.srat->GetProximityDomainForLocalAPIC(
                          *liumos->bsp_local_apic));
  } else if (IsEqualString(line, "pmem show")) {
    for (int i = 0; i < LiumOS::kNumOfPMEMManagers; i++) {
      if (!liumos->pmem[i])
        break;
      liumos->pmem[i]->Print();
    }
  } else if (IsEqualString(line, "pmem init")) {
    for (int i = 0; i < LiumOS::kNumOfPMEMManagers; i++) {
      if (!liumos->pmem[i])
        break;
      liumos->pmem[i]->Init();
    }
  } else if (IsEqualString(line, "pmem alloc")) {
    if (!liumos->pmem[0]) {
      PutString("PMEM not found\n");
      return;
    }
    uint64_t obj_addr = liumos->pmem[0]->AllocPages<uint64_t>(3);
    PutStringAndHex("Allocated object at", obj_addr);
  } else if (IsEqualString(line, "pmem ls")) {
    if (!liumos->pmem[0]) {
      PutString("PMEM not found\n");
      return;
    }
    PersistentProcessInfo* pp_info =
        liumos->pmem[0]->GetLastPersistentProcessInfo();
    pp_info->Print();
  } else if (IsEqualString(line, "pmem restore")) {
    if (!liumos->pmem[0]) {
      PutString("PMEM not found\n");
      return;
    }
    PersistentProcessInfo* pp_info =
        liumos->pmem[0]->GetLastPersistentProcessInfo();
    if (!pp_info) {
      PutString("No persistent process info found.\n");
      return;
    }
    PutString("Restoring process...\n");
    pp_info->Print();
    Process& proc =
        liumos->proc_ctrl->RestoreFromPersistentProcessInfo(*pp_info);
    liumos->scheduler->RegisterProcess(proc);
    proc.WaitUntilExit();
  } else if (IsEqualString(line, "pmem run pi.bin")) {
    assert(liumos->pmem[0]);
    int idx = GetLoaderInfo().FindFile("pi.bin");
    if (idx == -1) {
      PutString("file not found.");
      return;
    }
    EFIFile& pi_bin = GetLoaderInfo().root_files[idx];
    Process& proc = LoadELFAndCreatePersistentProcess(pi_bin, *liumos->pmem[0]);
    liumos->scheduler->LaunchAndWaitUntilExit(proc);
  } else if (IsEqualString(line, "pmem run hello.bin")) {
    assert(liumos->pmem[0]);
    int idx = GetLoaderInfo().FindFile("hello.bin");
    if (idx == -1) {
      PutString("file not found.");
      return;
    }
    EFIFile& hello_bin = GetLoaderInfo().root_files[idx];
    Process& proc =
        LoadELFAndCreatePersistentProcess(hello_bin, *liumos->pmem[0]);
    liumos->scheduler->LaunchAndWaitUntilExit(proc);
  } else if (strncmp(line, "test mem ", 9) == 0) {
    int proximity_domain = atoi(&line[9]);
    TestMem(GetSystemDRAMAllocator(), proximity_domain);
  } else if (strncmp(line, "test memw ", 10) == 0) {
    int proximity_domain = atoi(&line[10]);
    TestMemWrite(GetSystemDRAMAllocator(), proximity_domain);
  } else if (IsEqualString(line, "free")) {
    Free();
  } else if (IsEqualString(line, "time")) {
    Time();
  } else if (strncmp(line, "eval ", 5) == 0) {
    int idx = GetLoaderInfo().FindFile("pi.bin");
    if (idx == -1) {
      PutString("file not found.");
      return;
    }
    EFIFile& pi_bin = GetLoaderInfo().root_files[idx];
    int us = atoi(&line[5]);
    PutStringAndHex("Eval in time slice", us);
    ClearIntFlag();
    HPET::GetInstance().SetTimerNs(
        0, us,
        HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);

    StoreIntFlag();

    assert(liumos->pmem[0]);
    constexpr int kNumOfTestRun = 5;

    PutString("Ephemeral Process:\n");
    uint64_t ns_sum_ephemeral = 0;
    for (int i = 0; i < kNumOfTestRun; i++) {
      char* pname = new char[128 + 1];
      memmove(pname, line, strlen(line) + 1);
      Process& proc = LoadELFAndCreateEphemeralProcess(pi_bin, pname);
      ns_sum_ephemeral += liumos->scheduler->LaunchAndWaitUntilExit(proc);
    }

    PutString("Persistent Process:\n");
    uint64_t ns_sum_persistent = 0;
    for (int i = 0; i < kNumOfTestRun; i++) {
      Process& proc =
          LoadELFAndCreatePersistentProcess(pi_bin, *liumos->pmem[0]);
      ns_sum_persistent += liumos->scheduler->LaunchAndWaitUntilExit(proc);
    }
    PutString("timeslice(us), ephemeral avg(ns), persistent avg(ns)\n");
    PutString("0x");
    PutHex64(us);
    PutString(",");
    PutString("0x");
    PutHex64(ns_sum_ephemeral / kNumOfTestRun);
    PutString(",");
    PutString("0x");
    PutHex64(ns_sum_persistent / kNumOfTestRun);
    PutString("\n");
  } else if (IsEqualString(line, "cpuid")) {
    assert(liumos->cpu_features);
    CPUFeatureSet& f = *liumos->cpu_features;
    PutStringAndHex("Max CPUID", f.max_cpuid);
    PutStringAndHex("Max Extended CPUID", f.max_extended_cpuid);
    PutStringAndHex("family  ", f.family);
    PutStringAndHex("model   ", f.model);
    PutStringAndHex("stepping", f.stepping);
    PutString(f.brand_string);
    PutChar('\n');
    PutStringAndHex("MAX_PHY_ADDR", f.max_phy_addr);
    PutStringAndHex("phy_addr_mask", f.phy_addr_mask);
    PutStringAndBool("CLFLUSH supported", f.clfsh);
    PutStringAndBool("CLFLUSHOPT supported", f.clflushopt);
    for (int i = 0; i < CPUFeatureIndex::kSize; i++) {
      PutStringAndBool(CPUFeatureString[i], (f.features >> i) & 1);
    }
  } else if (IsEqualString(line, "lspci")) {
    ListPCIDevices();
  } else if (IsEqualString(line, "version")) {
    Version();
  } else if (IsEqualString(line, "help")) {
    PutString("hello: Nothing to say.\n");
    PutString("show xsdt: Print XSDT Entries\n");
    PutString("show nfit: Print NFIT Entries\n");
    PutString("show madt: Print MADT Entries\n");
    PutString("show srat: Print SRAT Entries\n");
    PutString("show slit: Print SLIT Entries\n");
    PutString("show mmap: Print UEFI MemoryMap\n");
    PutString("test mem: Test memory access \n");
    PutString("free: show memory free entries\n");
    PutString("time: show HPET main counter value\n");
  } else if (IsEqualString(line, "testscroll")) {
    uint64_t t0 = HPET::GetInstance().ReadMainCounterValue();
    uint64_t t1 = t0 + 3 * 1000'000'000'000'000 /
                           HPET::GetInstance().GetFemtosecondPerCount();
    for (int i = 0; HPET::GetInstance().ReadMainCounterValue() < t1; i++) {
      PutStringAndHex("Line", i + 1);
    }
  } else if (IsEqualString(line, "xhci init")) {
    XHCI::Controller::GetInstance().Init();
  } else if (IsEqualString(line, "xhci show portsc")) {
    XHCI::Controller::GetInstance().PrintPortSC();
  } else if (IsEqualString(line, "xhci show status")) {
    XHCI::Controller::GetInstance().PrintUSBSTS();
  } else if (IsEqualString(line, "lsusb")) {
    XHCI::Controller::GetInstance().PrintUSBDevices();
  } else if (IsEqualString(line, "teststl")) {
    char s[128];
    snprintf(s, sizeof(s), "123 = 0x%X\n", 123);
    PutString(s);

    int v = 123;
    std::function<void(void)> f = [&]() {
      snprintf(s, sizeof(s), "Hello from std::function!\nv = %d\n", v);
      PutString(s);
    };
    v = 456;
    f();

    std::vector<int> vec;
    snprintf(s, sizeof(s), "capacity = 0x%lX\n", vec.capacity());
    PutString(s);
    vec.push_back(3);
    vec.push_back(1);
    vec.push_back(4);
    vec.push_back(1);
    vec.push_back(5);
    vec.push_back(9);
    vec.push_back(2);
    vec.push_back(6);
    vec.push_back(5);
    for (auto& v : vec) {
      snprintf(s, sizeof(s), "%d ", v);
      PutString(s);
    }
    PutString("\n");
    snprintf(s, sizeof(s), "capacity = 0x%lX\n", vec.capacity());
    PutString(s);
    std::sort(vec.begin(), vec.end());
    PutString("sorted values:\n");
    for (auto& v : vec) {
      snprintf(s, sizeof(s), "value=%d, addr=%p\n", v, (void*)&v);
      PutString(s);
    }
    PutString("\n");
  } else if (IsEqualString(line, "fxsave")) {
    static uint8_t buf[512];
    _fxsave(buf);
    for (int i = 0; i < 32; i++) {
      PutHex8ZeroFilled(buf[i]);
      if ((i & 0xF) != 0xF)
        continue;
      PutString("\n");
    }
  } else if (IsEqualString(line, "ascii")) {
    for (int i = 0; i < 0x100; i++) {
      PutChar(i);
    }
    PutChar('\n');
  } else if (IsEqualString(line, "logo")) {
    int idx = GetLoaderInfo().FindFile("liumos.ppm");
    if (idx == -1) {
      PutString("file not found.");
      return;
    }
    EFIFile& liumos_ppm = GetLoaderInfo().root_files[idx];
    DrawPPMFile(liumos_ppm, 0, 0);
  } else if (IsEqualString(line, "ud2")) {
    __asm__ volatile("ud2;");
  } else if (IsEqualString(line, "adlib")) {
    TestAdlib();
  } else if (IsEqualString(args.GetArg(0), "midi")) {
    if (args.GetNumOfArgs() < 2) {
      PutString("midi <file>\n");
      return;
    }
    const char* file_name = args.GetArg(1);
    PlayMIDI(file_name);
  } else if (IsEqualString(line, "ls")) {
    for (int i = 0; i < GetLoaderInfo().root_files_used; i++) {
      PutString(GetLoaderInfo().root_files[i].GetFileName());
      PutChar('\n');
    }
  } else if (IsEqualString(line, "ps")) {
    using Status = Process::Status;
    kprintf("  PID CMD\n");
    for (int i = 0; i < liumos->scheduler->GetNumOfProcess(); i++) {
      Process* proc = liumos->scheduler->GetProcess(i);
      if (!proc || proc->GetStatus() == Status::kStopping ||
          proc->GetStatus() == Status::kStopped)
        continue;
      kprintf("%5lu %s\n", proc->GetID(), proc->GetName());
    }
  } else if (IsEqualString(args.GetArg(0), "kill")) {
    if (args.GetNumOfArgs() < 2) {
      kprintf("kill <pid>\n");
      return;
    }
    Process::PID pid = atoi(args.GetArg(1));
    liumos->scheduler->Kill(pid);
  } else {
    const char* arg0 = args.GetArg(0);
    EFIFile* file = nullptr;
    for (int i = 0; i < GetLoaderInfo().root_files_used; i++) {
      if (IsEqualString(GetLoaderInfo().root_files[i].GetFileName(), arg0)) {
        file = &GetLoaderInfo().root_files[i];
        break;
      }
    }
    if (!file) {
      PutString("Not a command or file: ");
      PutString(line);
      tbox.putc('\n');
      return;
    }
    int argc = args.GetNumOfArgs();
    constexpr int kMaxArgvSize = 8;
    uint64_t argv[kMaxArgvSize];
    if (argc > kMaxArgvSize) {
      PutString("Too many args");
      tbox.putc('\n');
      return;
    }
    char* pname = new char[strlen(line) + 1];
    memmove(pname, line, strlen(line) + 1);
    Process& proc = LoadELFAndCreateEphemeralProcess(*file, pname);
    for (int i = 0; i < argc; i++) {
      const char* arg = args.GetArg(i);
      proc.GetExecutionContext().PushDataToStack(arg, strlen(arg) + 1);
      argv[i] = proc.GetExecutionContext().GetRSP();
    }
    proc.GetExecutionContext().AlignStack(8);
    for (int i = argc - 1; i >= 0; i--) {
      proc.GetExecutionContext().PushDataToStack(&argv[i], sizeof(argv[0]));
    }
    uint64_t argc64 = argc;
    proc.GetExecutionContext().PushDataToStack(&argc64, sizeof(argc64));
    liumos->scheduler->RegisterProcess(proc);
    if (background) {
      kprintf("%lu\n", proc.GetID());
      return;
    }
    while (proc.GetStatus() != Process::Status::kStopped) {
      uint16_t keyid = liumos->main_console->GetCharWithoutBlocking();
      if (keyid == KeyID::kNoInput) {
        Sleep();
        continue;
      }
      if (KeyID::IsWithCtrl(keyid) && KeyID::IsChar(keyid, 'c')) {
        // Ctrl-C
        proc.Kill();
        proc.WaitUntilExit();
        PutString("\nkilled.\n");
        break;
      }
      if (!KeyID::IsBreak(keyid)) {
        proc.GetStdIn().Push(keyid);
      }
    }
  }
}

void WaitAndProcess(TextBox& tbox) {
  PutString("(liumos)$ ");
  tbox.StartRecording();
  while (1) {
    uint16_t keyid;
    while ((keyid = liumos->main_console->GetCharWithoutBlocking()) ==
           KeyID::kNoInput) {
      StoreIntFlagAndHalt();
    }
    if (keyid == '\n') {
      tbox.StopRecording();
      tbox.putc('\n');
      ConsoleCommand::Run(tbox);
      return;
    }
    tbox.putc(keyid);
  }
}

}  // namespace ConsoleCommand
