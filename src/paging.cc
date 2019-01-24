#include "liumos.h"

void IA_PDT::Print() {
  for (int i = 0; i < kNumOfPDE; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("PDT[");
    PutHex64(i);
    PutString("]:\n");
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
    PutString("PDPT[");
    PutHex64(i);
    PutString("]:\n");
    if (entries[i].IsPage()) {
      PutString(" 1GB Page\n");
      continue;
    }
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

void InitPaging() {
  IA32_EFER efer;
  efer.data = ReadMSR(MSRIndex::kEFER);
  if (!efer.bits.LME)
    Panic("IA32_EFER.LME not enabled.");
  PutString("4-level paging enabled.\n");

  IA32_MaxPhyAddr max_phy_addr_msr;
  max_phy_addr_msr.data = ReadMSR(MSRIndex::kMaxPhyAddr);
  kMaxPhyAddr = max_phy_addr_msr.bits.physical_address_bits;
  if (!max_phy_addr_msr.bits.physical_address_bits) {
    PutString("CPUID function 80000008H not supported.\n");
    PutString("Assuming Physical address bits = 36\n");
    kMaxPhyAddr = 36;
  }
  PutStringAndHex("kMaxPhyAddr", kMaxPhyAddr);

  const EFI::MemoryDescriptor* loader_code_desc = nullptr;
  uint64_t direct_mapping_end = 0;
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
  assert(loader_code_desc->number_of_pages < (1 << 9));
  PutChar('\n');
  PutStringAndHex("map_end_addr", direct_mapping_end);
  uint64_t direct_map_1gb_pages = (direct_mapping_end + (1ULL << 30) - 1) >> 30;
  PutStringAndHex("direct map 1gb pages", direct_map_1gb_pages);
  assert(direct_map_1gb_pages < (1 << 9));
  PutStringAndHex("InitPaging", reinterpret_cast<uint64_t>(InitPaging));
  IA_PML4* kernel_pml4 =
      reinterpret_cast<IA_PML4*>(page_allocator->AllocPages(1));
  kernel_pml4->ClearMapping();
  IA_PDPT* direct_map_pdpt =
      reinterpret_cast<IA_PDPT*>(page_allocator->AllocPages(1));
  direct_map_pdpt->ClearMapping();
  kernel_pml4->SetTableBaseForAddr(0, direct_map_pdpt,
                                   kPageAttrPresent | kPageAttrWritable);
  for (size_t i = 0; i < direct_map_1gb_pages; i++) {
    direct_map_pdpt->SetPageBaseForAddr((1 << 30) * i, (1 << 30) * i,
                                        kPageAttrPresent | kPageAttrWritable);
  }
  IA_PDPT* kernel_pdpt =
      reinterpret_cast<IA_PDPT*>(page_allocator->AllocPages(1));
  kernel_pdpt->ClearMapping();
  kernel_pml4->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdpt,
                                   kPageAttrPresent | kPageAttrWritable);
  IA_PDT* kernel_pdt = reinterpret_cast<IA_PDT*>(page_allocator->AllocPages(1));
  kernel_pdt->ClearMapping();
  kernel_pdpt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pdt,
                                   kPageAttrPresent | kPageAttrWritable);
  IA_PT* kernel_pt = reinterpret_cast<IA_PT*>(page_allocator->AllocPages(1));
  kernel_pt->ClearMapping();
  kernel_pdt->SetTableBaseForAddr(kKernelBaseAddr, kernel_pt,
                                  kPageAttrPresent | kPageAttrWritable);
  for (size_t i = 0; i < loader_code_desc->number_of_pages; i++) {
    kernel_pt->SetPageBaseForAddr(
        kKernelBaseAddr + (1 << 12) * i,
        reinterpret_cast<uint64_t>(page_allocator->AllocPages(1)),
        kPageAttrPresent | kPageAttrWritable);
  }
  WriteCR3(reinterpret_cast<uint64_t>(kernel_pml4));
  *reinterpret_cast<uint8_t*>(kKernelBaseAddr) = 1;
}
