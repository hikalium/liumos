#include "liumos.h"

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
                  &ACPI::NFIT::SPARange::kByteAdressablePersistentMemory))
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
  liumos->dram_allocator->Print();
  PutString("PMEM Free List:\n");
  liumos->pmem_allocator->Print();
}

bool IsEqualString(const char* a, const char* b) {
  while (*a == *b) {
    if (*a == 0)
      return true;
    a++;
    b++;
  }
  return false;
}

void label(uint64_t i) {
  PutString("0x");
  PutHex64(i);
  PutString(",");
}

uint64_t get_seconds() {
  return liumos->hpet->ReadMainCounterValue();
}

void TestMem(PhysicalPageAllocator* allocator, uint32_t proximity_domain) {
  constexpr uint64_t kRangeMin = 1ULL << 10;
  constexpr uint64_t kRangeMax = 1ULL << 24;
  PutStringAndHex("Test memory on proximity_domain", proximity_domain);
  constexpr uint64_t array_size_in_pages =
      (sizeof(int) * kRangeMax + kPageSize - 1) >> kPageSizeExponent;
  int* array = allocator->AllocPagesInProximityDomain<int*>(array_size_in_pages,
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

  CreatePageMapping(*liumos->dram_allocator, GetKernelPML4(),
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
      (uint64_t)(0.1 * 1e15) / liumos->hpet->GetFemtosecndPerCount();

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
          tick_sum_of_mem_read * liumos->hpet->GetFemtosecndPerCount() /
          (steps * csize) / 1000;
      PutString("0x");
      PutHex64(pico_second_per_mem_read > 0 ? pico_second_per_mem_read : 1);
      PutString(", ");
    };
    PutString("\n");
  };
  StoreIntFlag();
}

void TestMemWrite(PhysicalPageAllocator* allocator, uint32_t proximity_domain) {
  constexpr uint64_t kRangeMin = 1ULL << 10;
  constexpr uint64_t kRangeMax = 1ULL << 24;
  PutStringAndHex("Test memory on proximity_domain", proximity_domain);
  constexpr uint64_t array_size_in_pages =
      (sizeof(int) * kRangeMax + kPageSize - 1) >> kPageSizeExponent;
  int* array = allocator->AllocPagesInProximityDomain<int*>(array_size_in_pages,
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

  CreatePageMapping(*liumos->dram_allocator, GetKernelPML4(),
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
      (uint64_t)(0.1 * 1e15) / liumos->hpet->GetFemtosecndPerCount();

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
          tick_sum_access_only * liumos->hpet->GetFemtosecndPerCount() /
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
  PutStringAndHex("HPET main counter", liumos->hpet->ReadMainCounterValue());
}

void Process(TextBox& tbox) {
  const char* line = tbox.GetRecordedString();
  if (IsEqualString(line, "hello")) {
    PutString("Hello, world!\n");
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
  } else if (IsEqualString(line, "show mmap")) {
    ShowEFIMemoryMap();
  } else if (IsEqualString(line, "show hpet")) {
    liumos->hpet->Print();
  } else if (IsEqualString(line, "show cpu")) {
    PutString("APIC Mode: ");
    PutString(liumos->bsp_local_apic->Isx2APIC() ? "x2APIC" : "xAPIC");
    PutString("\n");
    PutStringAndHex("BSP APIC ID", liumos->bsp_local_apic->GetID());
    if (liumos->acpi.srat)
      PutStringAndHex("  proximity_domain",
                      liumos->acpi.srat->GetProximityDomainForLocalAPIC(
                          *liumos->bsp_local_apic));
  } else if (strncmp(line, "test mem ", 9) == 0) {
    int proximity_domain = atoi(&line[9]);
    TestMem(liumos->dram_allocator, proximity_domain);
  } else if (strncmp(line, "test pmem ", 10) == 0) {
    int proximity_domain = atoi(&line[10]);
    TestMem(liumos->pmem_allocator, proximity_domain);
  } else if (strncmp(line, "test pmemw ", 11) == 0) {
    int proximity_domain = atoi(&line[11]);
    TestMemWrite(liumos->pmem_allocator, proximity_domain);
  } else if (strncmp(line, "test memw ", 10) == 0) {
    int proximity_domain = atoi(&line[10]);
    TestMemWrite(liumos->dram_allocator, proximity_domain);
  } else if (IsEqualString(line, "free")) {
    Free();
  } else if (IsEqualString(line, "time")) {
    Time();
  } else if (IsEqualString(line, "hello.bin")) {
    ExecutionContext* ctx = LoadELFAndLaunchProcess(*liumos->hello_bin_file);
    ctx->WaitUntilExit();
  } else if (IsEqualString(line, "pi.bin")) {
    ExecutionContext* ctx = LoadELFAndLaunchProcess(*liumos->pi_bin_file);
    ctx->WaitUntilExit();
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
  } else {
    PutString("Command not found: ");
    PutString(tbox.GetRecordedString());
    tbox.putc('\n');
  }
}

void WaitAndProcess(TextBox& tbox) {
  PutString("> ");
  tbox.StartRecording();
  while (1) {
    StoreIntFlagAndHalt();
    while (1) {
      uint16_t keyid;
      if ((keyid = liumos->keyboard_ctrl->ReadKeyID())) {
        if (!keyid && keyid & KeyID::kMaskBreak)
          continue;
        if (keyid == KeyID::kEnter) {
          keyid = '\n';
        }
      } else if (liumos->com1->IsReceived()) {
        keyid = liumos->com1->ReadCharReceived();
        if (keyid == '\n') {
          continue;
        }
        if (keyid == '\r') {
          keyid = '\n';
        }
      } else {
        break;
      }
      if (keyid == '\n') {
        tbox.StopRecording();
        tbox.putc('\n');
        ConsoleCommand::Process(tbox);
        return;
      }
      tbox.putc(keyid);
    }
  }
}

}  // namespace ConsoleCommand
