#include "liumos.h"

IA_PML4* kernel_pml4;

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
  IA_PML4* pml4 = page_allocator->AllocPages<IA_PML4*>(1);
  pml4->ClearMapping();
  for (int i = 0; i < IA_PML4::kNumOfPML4E; i++) {
    pml4->entries[i] = kernel_pml4->entries[i];
  }
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

  // Adjust direct_mapping_end here
  // since VRAM region is not appeared in EFIMemoryMap
  {
    uint64_t map_end_addr = reinterpret_cast<uint64_t>(screen_sheet->GetBuf()) +
                            screen_sheet->GetBufSize();
    if (map_end_addr > direct_mapping_end)
      direct_mapping_end = map_end_addr;
  }

  assert(loader_code_desc->number_of_pages < (1 << 9));
  PutStringAndHex("map_end_addr", direct_mapping_end);
  uint64_t direct_map_1gb_pages = (direct_mapping_end + (1ULL << 30) - 1) >> 30;
  PutStringAndHex("direct map 1gb pages", direct_map_1gb_pages);
  PutStringAndHex("InitPaging", reinterpret_cast<uint64_t>(InitPaging));

  kernel_pml4 = page_allocator->AllocPages<IA_PML4*>(1);
  kernel_pml4->ClearMapping();

  // mapping pages for real memory & memory mapped IOs
  for (uint64_t addr = 0; addr < direct_mapping_end;) {
    if (addr >= direct_mapping_end)
      break;
    IA_PDPT* pdpt = page_allocator->AllocPages<IA_PDPT*>(1);
    pdpt->ClearMapping();
    kernel_pml4->SetTableBaseForAddr(addr, pdpt,
                                     kPageAttrPresent | kPageAttrWritable);
    for (int i = 0; i < IA_PDPT::kNumOfPDPTE; i++) {
      if (addr >= direct_mapping_end)
        break;
      IA_PDT* pdt = page_allocator->AllocPages<IA_PDT*>(1);
      pdt->ClearMapping();
      pdpt->SetTableBaseForAddr(addr, pdt,
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
  }

  /*
  kernel_pml4->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdpt,
                                   kPageAttrPresent | kPageAttrWritable);
  kernel_pdpt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdt,
                                   kPageAttrPresent | kPageAttrWritable);
  kernel_pdt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pt,
                                  kPageAttrPresent | kPageAttrWritable);
  for (size_t i = 0; i < loader_code_desc->number_of_pages; i++) {
    uint8_t* page = page_allocator->AllocPages<uint8_t*>(1);
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
