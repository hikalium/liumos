#include <elf.h>

#include "liumos.h"
#include "pmem.h"

struct PhdrInfo {
  const uint8_t* data;
  uint64_t vaddr;
  size_t map_size;
  size_t copy_size;
  void Clear() {
    data = nullptr;
    vaddr = 0;
    map_size = 0;
    copy_size = 0;
  }
};

struct PhdrMappingInfo {
  PhdrInfo code;
  PhdrInfo data;
  void Clear() {
    code.Clear();
    data.Clear();
  }
};

static const Elf64_Ehdr* EnsureLoadable(const uint8_t* buf) {
  if (memcmp(buf, ELFMAG, SELFMAG) != 0) {
    PutString("Not an ELF file\n");
    return nullptr;
  }
  if (buf[EI_CLASS] != ELFCLASS64) {
    PutString("Not an ELF Class 64n");
    return nullptr;
  }
  if (buf[EI_DATA] != ELFDATA2LSB) {
    PutString("Not an ELF Data 2LSB");
    return nullptr;
  }
  if (buf[EI_OSABI] != ELFOSABI_SYSV) {
    PutString("Not a SYSV ABI");
    return nullptr;
  }
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(buf);
  if (ehdr->e_type != ET_EXEC) {
    PutString("Not an executable");
    return nullptr;
  }
  if (ehdr->e_machine != EM_X86_64) {
    PutString("Not for x86_64");
    return nullptr;
  }
  return ehdr;
}

const Elf64_Shdr* FindSectionHeader(EFIFile& file, const char* name) {
  const uint8_t* buf = file.GetBuf();
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(buf);
  if (!ehdr)
    return nullptr;

  const Elf64_Shdr* strtab_hdr = reinterpret_cast<const Elf64_Shdr*>(
      buf + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
  const char* strtab =
      reinterpret_cast<const char*>(buf + strtab_hdr->sh_offset);

  for (int i = 0; i < ehdr->e_shnum; i++) {
    const Elf64_Shdr* shdr = reinterpret_cast<const Elf64_Shdr*>(
        buf + ehdr->e_shoff + ehdr->e_shentsize * i);
    if (strcmp(&strtab[shdr->sh_name], name) == 0)
      return shdr;
  }
  return nullptr;
}

static const Elf64_Ehdr* ParseProgramHeader(EFIFile& file,
                                            ProcessMappingInfo& proc_map_info,
                                            PhdrMappingInfo& phdr_map_info) {
  proc_map_info.Clear();
  phdr_map_info.Clear();
  const uint8_t* buf = file.GetBuf();
  assert(IsAlignedToPageSize(buf));
  const Elf64_Ehdr* ehdr = EnsureLoadable(buf);
  if (!ehdr)
    return nullptr;

  for (int i = 0; i < ehdr->e_phnum; i++) {
    const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
        buf + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD)
      continue;
    PutStringAndHex("LOAD program header idx", i);

    assert(IsAlignedToPageSize(phdr->p_align));

    SegmentMapping* seg_map = &proc_map_info.data;
    PhdrInfo* phdr_info = &phdr_map_info.data;
    if (phdr->p_flags & PF_X) {
      seg_map = &proc_map_info.code;
      phdr_info = &phdr_map_info.code;
    }
    assert(!seg_map->GetMapSize());  // Avoid overwriting
    uint64_t vaddr = FloorToPageAlignment(phdr->p_vaddr);
    seg_map->Set(vaddr, 0,
                 CeilToPageAlignment(phdr->p_memsz + (phdr->p_vaddr - vaddr)));
    assert(phdr_info);
    phdr_info->data = buf + FloorToPageAlignment(phdr->p_offset);
    phdr_info->vaddr = seg_map->GetVirtAddr();
    phdr_info->map_size = seg_map->GetMapSize();
    phdr_info->copy_size =
        phdr->p_filesz + (phdr->p_vaddr - seg_map->GetVirtAddr());
  }
  return ehdr;
}

template <class TAllocator>
static void LoadAndMapSegment(TAllocator& allocator,
                              IA_PML4& page_root,
                              SegmentMapping& seg_map,
                              PhdrInfo& phdr_info,
                              uint64_t page_attr,
                              bool should_clflush) {
  assert(seg_map.GetVirtAddr() == phdr_info.vaddr);
  assert(seg_map.GetMapSize() == phdr_info.map_size);
  assert(seg_map.GetPhysAddr());
  uint8_t* phys_buf = reinterpret_cast<uint8_t*>(seg_map.GetPhysAddr());
  memcpy(phys_buf, phdr_info.data, phdr_info.copy_size);
  bzero(phys_buf + phdr_info.copy_size,
        phdr_info.map_size - phdr_info.copy_size);
  seg_map.Map(allocator, page_root, page_attr, should_clflush);
}

template <class TAllocator>
static void LoadAndMap(TAllocator& allocator,
                       IA_PML4& page_root,
                       ProcessMappingInfo& proc_map_info,
                       PhdrMappingInfo& phdr_map_info,
                       uint64_t base_attr,
                       bool should_clflush) {
  uint64_t page_attr = kPageAttrPresent | base_attr;

  LoadAndMapSegment(allocator, page_root, proc_map_info.code,
                    phdr_map_info.code, page_attr, should_clflush);
  LoadAndMapSegment(allocator, page_root, proc_map_info.data,
                    phdr_map_info.data, page_attr | kPageAttrWritable,
                    should_clflush);
  proc_map_info.stack.Map(allocator, page_root, page_attr | kPageAttrWritable,
                          should_clflush);
  proc_map_info.heap.Map(allocator, page_root, page_attr | kPageAttrWritable,
                         should_clflush);
}

Process& LoadELFAndCreateEphemeralProcess(EFIFile& file, const char* const name) {
  ExecutionContext& ctx =
      *liumos->kernel_heap_allocator->Alloc<ExecutionContext>();
  ProcessMappingInfo& map_info = ctx.GetProcessMappingInfo();
  PhdrMappingInfo phdr_map_info;
  IA_PML4& user_page_table = AllocPageTable(GetSystemDRAMAllocator());
  SetKernelPageEntries(user_page_table);

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);

  map_info.code.SetPhysAddr(GetSystemDRAMAllocator().AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.code.GetMapSize())));
  map_info.data.SetPhysAddr(GetSystemDRAMAllocator().AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.data.GetMapSize())));

  const int kNumOfStackPages = 32;
  map_info.stack.Set(
      0xBEEF'0000,
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfStackPages),
      kNumOfStackPages << kPageSizeExponent);

  if (liumos->debug_mode_enabled) {
    map_info.Print();
  }
  LoadAndMap(GetSystemDRAMAllocator(), user_page_table, map_info, phdr_map_info,
             kPageAttrUser, false);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);

  void* stack_pointer =
      reinterpret_cast<void*>(map_info.stack.GetVirtEndAddr());

  uint64_t kernel_stack_pointer =
      liumos->kernel_heap_allocator->AllocPages<uint64_t>(
          kKernelStackPagesForEachProcess) +
      kPageSize * kKernelStackPagesForEachProcess;

  if (liumos->debug_mode_enabled) {
    PutStringAndHex("Kernel stack pointer", kernel_stack_pointer);
  }

  ctx.SetRegisters(reinterpret_cast<void (*)(void)>(entry_point),
                   GDT::kUserCS64Selector, stack_pointer, GDT::kUserDSSelector,
                   reinterpret_cast<uint64_t>(&user_page_table),
                   kRFlagsInterruptEnable, kernel_stack_pointer);
  Process& proc = liumos->proc_ctrl->Create(name);
  proc.InitAsEphemeralProcess(ctx);
  return proc;
}

Process& LoadELFAndCreatePersistentProcess(EFIFile& file,
                                           PersistentMemoryManager& pmem) {
  constexpr uint64_t kUserStackBaseAddr = 0xBEEF'0000;
  const int kNumOfStackPages = 32;
  PersistentProcessInfo& pp_info = *pmem.AllocPersistentProcessInfo();
  pp_info.Init();
  ExecutionContext& ctx = pp_info.GetContext(0);
  pp_info.SetValidContextIndex(0);
  ProcessMappingInfo& map_info = ctx.GetProcessMappingInfo();
  PhdrMappingInfo phdr_map_info;
  IA_PML4& user_page_table = AllocPageTable(pmem);

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);
  map_info.stack.Set(kUserStackBaseAddr, 0,
                     kNumOfStackPages << kPageSizeExponent);

  map_info.code.AllocSegmentFromPersistentMemory(pmem);
  map_info.data.AllocSegmentFromPersistentMemory(pmem);
  map_info.stack.AllocSegmentFromPersistentMemory(pmem);

  map_info.Print();
  LoadAndMap(pmem, user_page_table, map_info, phdr_map_info, kPageAttrUser,
             true);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);

  void* stack_pointer =
      reinterpret_cast<void*>(map_info.stack.GetVirtEndAddr());

  ctx.SetRegisters(reinterpret_cast<void (*)(void)>(entry_point),
                   GDT::kUserCS64Selector, stack_pointer, GDT::kUserDSSelector,
                   reinterpret_cast<uint64_t>(&user_page_table),
                   kRFlagsInterruptEnable, 0);

  ExecutionContext& working_ctx = pp_info.GetContext(1);
  working_ctx = ctx;
  ProcessMappingInfo& working_ctx_map = working_ctx.GetProcessMappingInfo();
  working_ctx_map.data.AllocSegmentFromPersistentMemory(pmem);
  working_ctx_map.stack.AllocSegmentFromPersistentMemory(pmem);

  IA_PML4& pt = AllocPageTable(pmem);
  working_ctx_map.code.Map(pmem, pt, kPageAttrUser, true);
  working_ctx_map.data.Map(pmem, pt, kPageAttrUser | kPageAttrWritable, true);
  working_ctx_map.stack.Map(pmem, pt, kPageAttrUser | kPageAttrWritable, true);
  working_ctx.SetCR3(pt);

  return liumos->proc_ctrl->RestoreFromPersistentProcessInfo(pp_info);
}

void LoadKernelELF(EFIFile& file, LoaderInfo& loader_info) {
  ProcessMappingInfo map_info;
  PhdrMappingInfo phdr_map_info;

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);

  map_info.code.SetPhysAddr(GetSystemDRAMAllocator().AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.code.GetMapSize())));
  map_info.data.SetPhysAddr(GetSystemDRAMAllocator().AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.data.GetMapSize())));

  constexpr uint64_t kNumOfKernelMainStackPages = 4;
  uint64_t kernel_main_stack_virtual_base = 0xFFFF'FFFF'4000'0000ULL;

  map_info.stack.Set(
      kernel_main_stack_virtual_base,
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelMainStackPages),
      kNumOfKernelMainStackPages << kPageSizeExponent);

  constexpr uint64_t kNumOfKernelHeapPages = 4;
  uint64_t kernel_heap_virtual_base = 0xFFFF'FFFF'5000'0000ULL;

  map_info.heap.Set(
      kernel_heap_virtual_base,
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelHeapPages),
      kNumOfKernelHeapPages << kPageSizeExponent);

  LoadAndMap(GetSystemDRAMAllocator(), GetKernelPML4(), map_info, phdr_map_info,
             0, false);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);

  uint64_t kernel_main_stack_pointer =
      kernel_main_stack_virtual_base +
      (kNumOfKernelMainStackPages << kPageSizeExponent);

  JumpToKernel(entry_point, liumos, kernel_main_stack_pointer, loader_info);
}
