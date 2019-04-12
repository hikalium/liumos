#include "liumos.h"

template <>
void IA_PDT::Print() {
  for (int i = 0; i < kNumOfEntries; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("  PDT[");
    PutHex64(i);
    PutString("]:");
    if (entries[i].IsPage()) {
      PutString(" 2MB Page\n");
      continue;
    }
    PutStringAndHex(" addr", entries[i].GetTableAddr());
  }
}

template <>
void IA_PDPT::Print() {
  for (int i = 0; i < kNumOfEntries; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString(" PDPT[");
    PutHex64(i);
    PutString("]:");
    if (entries[i].IsPage()) {
      PutString("  1GB Page\n");
      continue;
    }
    PutString("\n");
    IA_PDT* pdt = entries[i].GetTableAddr();
    pdt->Print();
  }
}

template <>
void IA_PML4::Print() {
  for (int i = 0; i < kNumOfEntries; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString(" PML4[");
    PutHex64(i);
    PutString("]:");
    PutString("\n");
  }
}
template <>
void IA_PT::DebugPrintEntryForAddr(uint64_t vaddr) {
  IA_PTE& e = GetEntryForAddr(vaddr);
  PutString("PTE@");
  PutHex64ZeroFilled(this);
  PutString("[");
  PutHex64(addr2index(vaddr));
  PutString("]: ");
  PutHex64ZeroFilled(e.data);
  PutString("\n");
}
template <>
void IA_PDT::DebugPrintEntryForAddr(uint64_t vaddr) {
  IA_PDE& e = GetEntryForAddr(vaddr);
  PutString("PDTE@");
  PutHex64ZeroFilled(this);
  PutString("[");
  PutHex64(addr2index(vaddr));
  PutString("]: ");
  PutHex64ZeroFilled(e.data);
  PutString("\n");
  if (e.IsPage())
    return;
  e.GetTableAddr()->DebugPrintEntryForAddr(vaddr);
}

template <>
void IA_PDPT::DebugPrintEntryForAddr(uint64_t vaddr) {
  IA_PDPTE& e = GetEntryForAddr(vaddr);
  PutString("PDPT@");
  PutHex64ZeroFilled(this);
  PutString("[");
  PutHex64(addr2index(vaddr));
  PutString("]: ");
  PutHex64ZeroFilled(e.data);
  PutString("\n");
  if (e.IsPage())
    return;
  e.GetTableAddr()->DebugPrintEntryForAddr(vaddr);
}

template <>
void IA_PML4::DebugPrintEntryForAddr(uint64_t vaddr) {
  IA_PML4E& e = GetEntryForAddr(vaddr);
  PutString("PML4@");
  PutHex64ZeroFilled(this);
  PutString("[");
  PutHex64(addr2index(vaddr));
  PutString("]: ");
  PutHex64ZeroFilled(e.data);
  PutString("\n");
  if (e.IsPage())
    return;
  e.GetTableAddr()->DebugPrintEntryForAddr(vaddr);
}

void SetKernelPageEntries(IA_PML4& pml4) {
  for (int i = pml4.kNumOfEntries / 2; i < pml4.kNumOfEntries; i++) {
    pml4.entries[i] = liumos->kernel_pml4->entries[i];
  }
}

void InitPaging() {
  IA32_EFER efer;
  efer.data = ReadMSR(MSRIndex::kEFER);
  if (!efer.bits.LME)
    Panic("IA32_EFER.LME not enabled.");
  PutString("4-level paging enabled.\n");
  // Even if 4-level paging is supported,
  // whether 1GB pages are supported or not is determined by
  // CPUID.80000001H:EDX.Page1GB [bit 26] = 1.
  uint64_t direct_mapping_end = 0xffff'ffffULL;
  EFI::MemoryMap& map = *liumos->efi_memory_map;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    uint64_t map_end_addr =
        desc->physical_start + (desc->number_of_pages << 12);
    if (map_end_addr > direct_mapping_end)
      direct_mapping_end = map_end_addr;
  }

  // Adjust direct_mapping_end here
  // since VRAM region is not appeared in EFIMemoryMap
  {
    uint64_t map_end_addr =
        reinterpret_cast<uint64_t>(liumos->screen_sheet->GetBuf()) +
        liumos->screen_sheet->GetBufSize();
    if (map_end_addr > direct_mapping_end)
      direct_mapping_end = map_end_addr;
  }

  // PMEM address area may not be shown in UEFI memory map. (ex. QEMU)
  // So we should check NFIT to determine direct_mapping_end.
  if (liumos->acpi.nfit) {
    ACPI::NFIT& nfit = *liumos->acpi.nfit;
    for (auto& it : nfit) {
      using namespace ACPI;
      if (it.type != NFIT::Entry::kTypeSPARangeStructure)
        continue;
      NFIT::SPARange* spa_range = reinterpret_cast<NFIT::SPARange*>(&it);
      uint64_t map_end_addr = spa_range->system_physical_address_range_base +
                              spa_range->system_physical_address_range_length;
      if (map_end_addr > direct_mapping_end)
        direct_mapping_end = map_end_addr;
    }
  }

  PutStringAndHex("map_end_addr", direct_mapping_end);
  uint64_t direct_map_1gb_pages = (direct_mapping_end + (1ULL << 30) - 1) >> 30;
  PutStringAndHex("direct map 1gb pages", direct_map_1gb_pages);

  IA_PML4* kernel_pml4 = liumos->dram_allocator->AllocPages<IA_PML4*>(1);
  kernel_pml4->ClearMapping();

  // mapping pages for real memory & memory mapped IOs
  CreatePageMapping(*liumos->dram_allocator, *kernel_pml4, 0, 0,
                    direct_mapping_end, kPageAttrPresent | kPageAttrWritable);
  CreatePageMapping(*liumos->dram_allocator, *kernel_pml4,
                    liumos->cpu_features->kernel_phys_page_map_begin, 0,
                    direct_mapping_end, kPageAttrPresent | kPageAttrWritable);

  CreatePageMapping(*liumos->dram_allocator, *kernel_pml4,
                    kLAPICRegisterAreaVirtBase, kLAPICRegisterAreaPhysBase,
                    kLAPICRegisterAreaByteSize,
                    kPageAttrPresent | kPageAttrWritable |
                        kPageAttrWriteThrough | kPageAttrCacheDisable);

  WriteCR3(reinterpret_cast<uint64_t>(kernel_pml4));
  PutStringAndHex("Paging enabled. Kernel CR3", ReadCR3());
  liumos->kernel_pml4 = kernel_pml4;
}

IA_PML4& GetKernelPML4(void) {
  return *liumos->kernel_pml4;
}

void FlushDirtyPages(IA_PML4& pml4_phys,
                     uint64_t vaddr,
                     uint64_t byte_size,
                     uint64_t& num_of_clflush_issued) {
  assert((vaddr & kPageAddrMask) == 0);
  IA_PML4& pml4 = *GetKernelVirtAddrForPhysAddr(&pml4_phys);
  uint64_t num_of_4k_pages = ByteSizeToPageSize(byte_size);
  for (int pml4_idx = IA_PML4::addr2index(vaddr);
       pml4_idx < IA_PML4::kNumOfEntries; pml4_idx++) {
    auto& pml4e = pml4.GetEntryForAddr(vaddr);
    if (!pml4e.IsPresent()) {
      Panic("Not mapped");
    }
    auto* pdpt = GetKernelVirtAddrForPhysAddr(pml4e.GetTableAddr());
    for (int pdpt_idx = IA_PDPT::addr2index(vaddr);
         num_of_4k_pages && pdpt_idx < IA_PDPT::kNumOfEntries; pdpt_idx++) {
      auto& pdpte = pdpt->GetEntryForAddr(vaddr);
      if (!pdpte.IsPresent()) {
        Panic("Not mapped");
      }
      if (pdpte.IsPage())
        Panic("Page overwrapping at pdpte");
      auto* pdt = GetKernelVirtAddrForPhysAddr(pdpte.GetTableAddr());
      for (int pdt_idx = IA_PDT::addr2index(vaddr);
           num_of_4k_pages && pdt_idx < IA_PDT::kNumOfEntries; pdt_idx++) {
        auto& pdte = pdt->GetEntryForAddr(vaddr);
        if (!pdte.IsPresent()) {
          Panic("Not mapped");
        }
        if (pdte.IsPage())
          Panic("Page overwrapping at pdte");
        auto* pt = GetKernelVirtAddrForPhysAddr(pdte.GetTableAddr());
        for (int pt_idx = IA_PT::addr2index(vaddr);
             num_of_4k_pages && pt_idx < IA_PT::kNumOfEntries; pt_idx++) {
          auto& pte = pt->GetEntryForAddr(vaddr);
          if (pte.IsDirty()) {
            CLFlush(reinterpret_cast<void*>(
                        GetKernelVirtAddrForPhysAddr(pte.GetPageBaseAddr())),
                    pte.kChunkSize, num_of_clflush_issued);
            pte.ClearDirtyBit();
          }
          vaddr += (1 << 12);
          num_of_4k_pages--;
        }
      }
    }
  }
}
