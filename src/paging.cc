#include "liumos.h"

IA_PML4* kernel_pml4;

IA_PDPT* direct_map_pdpt;

IA_PDPT* kernel_pdpt;
IA_PDT* kernel_pdt;
IA_PT* kernel_pt;

void IA_PDT::Print() {
  for (int i = 0; i < kNumOfPDE; i++) {
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

void IA_PDPT::Print() {
  for (int i = 0; i < kNumOfPDPTE; i++) {
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

void IA_PML4::Print() {
  for (int i = 0; i < kNumOfPML4E; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("PML4[");
    PutHex64(i);
    PutString("]:\n");
    IA_PDPT* pdpt = entries[i].GetTableAddr();
    pdpt->Print();
  }
}

IA_PML4* CreatePageTable() {
  IA_PML4* pml4 = reinterpret_cast<IA_PML4*>(page_allocator->AllocPages(1));
  pml4->ClearMapping();
  pml4->SetTableBaseForAddr(0, direct_map_pdpt,
                            kPageAttrPresent | kPageAttrWritable);
  return pml4;
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
  const EFI::MemoryDescriptor* loader_code_desc = nullptr;
  uint64_t direct_mapping_end = 0xffff'ffffULL;
  for (int i = 0; i < efi_memory_map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = efi_memory_map.GetDescriptor(i);
    uint64_t map_end_addr =
        desc->physical_start + (desc->number_of_pages << 12);
    if (map_end_addr > direct_mapping_end)
      direct_mapping_end = map_end_addr;
    if (desc->type != EFI::MemoryType::kLoaderCode)
      continue;
    assert(!loader_code_desc);
    loader_code_desc = desc;
  }
  loader_code_desc->Print();
  PutChar('\n');

  // for(;;);

  // Adjust direct_mapping_end here
  // since VRAM region is not appeared in EFIMemoryMap
  {
    PutStringAndHex("vram", vram);
    uint64_t map_end_addr =
        reinterpret_cast<uint64_t>(vram) + 4 * ysize * pixels_per_scan_line;
    if (map_end_addr > direct_mapping_end)
      direct_mapping_end = map_end_addr;
  }

  assert(loader_code_desc->number_of_pages < (1 << 9));
  PutStringAndHex("map_end_addr", direct_mapping_end);
  uint64_t direct_map_1gb_pages = (direct_mapping_end + (1ULL << 30) - 1) >> 30;
  PutStringAndHex("direct map 1gb pages", direct_map_1gb_pages);
  assert(direct_map_1gb_pages < (1 << 9));
  PutStringAndHex("InitPaging", reinterpret_cast<uint64_t>(InitPaging));

  kernel_pml4 = reinterpret_cast<IA_PML4*>(page_allocator->AllocPages(1));
  direct_map_pdpt = reinterpret_cast<IA_PDPT*>(page_allocator->AllocPages(1));
  kernel_pdpt = reinterpret_cast<IA_PDPT*>(page_allocator->AllocPages(1));
  kernel_pdt = reinterpret_cast<IA_PDT*>(page_allocator->AllocPages(1));
  kernel_pt = reinterpret_cast<IA_PT*>(page_allocator->AllocPages(1));

  kernel_pml4->ClearMapping();
  direct_map_pdpt->ClearMapping();
  kernel_pdpt->ClearMapping();
  kernel_pdt->ClearMapping();
  kernel_pt->ClearMapping();

  // mapping 1GB pages for real memory & memory mapped IOs
  kernel_pml4->SetTableBaseForAddr(0, direct_map_pdpt,
                                   kPageAttrPresent | kPageAttrWritable);
  for (uint64_t addr = 0; addr < direct_mapping_end;) {
    IA_PDT* pdt = reinterpret_cast<IA_PDT*>(page_allocator->AllocPages(1));
    pdt->ClearMapping();
    direct_map_pdpt->SetTableBaseForAddr(addr, pdt,
                                         kPageAttrPresent | kPageAttrWritable);
    for (int i = 0; i < IA_PDT::kNumOfPDE; i++) {
      if (addr >= direct_mapping_end)
        break;
      uint64_t attr = kPageAttrPresent | kPageAttrWritable;
      if (i == 3) {
        attr |= kPageAttrCacheDisable | kPageAttrWriteThrough;
      }
      pdt->SetPageBaseForAddr(addr, addr, attr);
      addr += 2 * 1024 * 1024;
    }
  }

  /*
  kernel_pml4->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdpt,
                                   kPageAttrPresent | kPageAttrWritable);
  kernel_pdpt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdt,
                                   kPageAttrPresent | kPageAttrWritable);
  kernel_pdt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pt,
                                  kPageAttrPresent | kPageAttrWritable);
  for (size_t i = 0; i < loader_code_desc->number_of_pages; i++) {
    uint8_t* page = reinterpret_cast<uint8_t*>(page_allocator->AllocPages(1));
    memcpy(page,
           reinterpret_cast<uint8_t*>(loader_code_desc->physical_start +
                                      i * (1 << 12)),
           (1 << 12));
    uint64_t page_flags = kPageAttrPresent | kPageAttrWritable;
    kernel_pt->SetPageBaseForAddr(kKernelBaseAddr + (1 << 12) * i,
                                  reinterpret_cast<uint64_t>(page), page_flags);
  }
  */
  PutStringAndHex("CR3", ReadCR3());
  WriteCR3(reinterpret_cast<uint64_t>(kernel_pml4));
}
