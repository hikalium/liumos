#include "lib/musl/include/elf.h"
#include "liumos.h"
#include "pmem.h"

static void PrintProgramHeader(const Elf64_Phdr* phdr) {
  PutString(" flags:");
  if (phdr->p_flags & PF_R)
    PutChar('R');
  if (phdr->p_flags & PF_W)
    PutChar('W');
  if (phdr->p_flags & PF_X)
    PutChar('X');
  PutChar('\n');
  PutString("  offset:");
  PutHex64ZeroFilled(phdr->p_offset);
  PutString(" vaddr:");
  PutHex64ZeroFilled(phdr->p_vaddr);
  PutChar('\n');
  PutString("  filesz:");
  PutHex64ZeroFilled(phdr->p_filesz);
  PutString(" memsz:");
  PutHex64ZeroFilled(phdr->p_memsz);
  PutString(" align:");
  PutHex64ZeroFilled(phdr->p_align);
  PutChar('\n');
}

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

static const Elf64_Ehdr* EnsureLoadable(const uint8_t* buf,
                                        uint64_t file_size) {
  PutStringAndHex("File Size", file_size);
  if (strncmp(reinterpret_cast<const char*>(buf), ELFMAG, SELFMAG) != 0) {
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
  PutString("This is an ELF file\n");
  return ehdr;
}

static const Elf64_Ehdr* ParseProgramHeader(File& file,
                                            ProcessMappingInfo& proc_map_info,
                                            PhdrMappingInfo& phdr_map_info) {
  proc_map_info.Clear();
  phdr_map_info.Clear();
  const uint8_t* buf = file.GetBuf();
  assert(IsAlignedToPageSize(buf));
  PutString("Loading ELF...\n");
  const Elf64_Ehdr* ehdr = EnsureLoadable(buf, file.GetFileSize());
  if (!ehdr)
    return nullptr;

  PutString("Program headers to be loaded:\n");
  for (int i = 0; i < ehdr->e_phnum; i++) {
    const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
        buf + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD)
      continue;
    PutString("Phdr #");
    PutHex64(i);
    PrintProgramHeader(phdr);

    assert(IsAlignedToPageSize(phdr->p_align));

    SegmentMapping* seg_map = nullptr;
    PhdrInfo* phdr_info = nullptr;
    if (phdr->p_flags & PF_X) {
      seg_map = &proc_map_info.code;
      phdr_info = &phdr_map_info.code;
    }
    if (phdr->p_flags & PF_W) {
      seg_map = &proc_map_info.data;
      phdr_info = &phdr_map_info.data;
    }
    assert(seg_map);
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

static void MapSegment(IA_PML4& page_root,
                       SegmentMapping& seg_map,
                       uint64_t page_attr) {
  assert(seg_map.GetPhysAddr());
  CreatePageMapping(*liumos->dram_allocator, page_root, seg_map.GetVirtAddr(),
                    seg_map.GetPhysAddr(), seg_map.GetMapSize(), page_attr);
}

static void LoadAndMapSegment(IA_PML4& page_root,
                              SegmentMapping& seg_map,
                              PhdrInfo& phdr_info,
                              uint64_t page_attr) {
  assert(seg_map.GetVirtAddr() == phdr_info.vaddr);
  assert(seg_map.GetMapSize() == phdr_info.map_size);
  assert(seg_map.GetPhysAddr());
  uint8_t* phys_buf = reinterpret_cast<uint8_t*>(seg_map.GetPhysAddr());
  memcpy(phys_buf, phdr_info.data, phdr_info.copy_size);
  bzero(phys_buf + phdr_info.copy_size,
        phdr_info.map_size - phdr_info.copy_size);
  MapSegment(page_root, seg_map, page_attr);
}

static void LoadAndMap(IA_PML4& page_root,
                       ProcessMappingInfo& proc_map_info,
                       PhdrMappingInfo& phdr_map_info,
                       uint64_t base_attr) {
  uint64_t page_attr = kPageAttrPresent | base_attr;

  LoadAndMapSegment(page_root, proc_map_info.code, phdr_map_info.code,
                    page_attr);
  LoadAndMapSegment(page_root, proc_map_info.data, phdr_map_info.data,
                    page_attr | kPageAttrWritable);
  MapSegment(page_root, proc_map_info.stack, page_attr | kPageAttrWritable);
}

Process& LoadELFAndLaunchEphemeralProcess(File& file) {
  ProcessMappingInfo map_info;
  PhdrMappingInfo phdr_map_info;
  IA_PML4& user_page_table = CreatePageTable();

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);

  map_info.code.SetPhysAddr(liumos->dram_allocator->AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.code.GetMapSize())));
  map_info.data.SetPhysAddr(liumos->dram_allocator->AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.data.GetMapSize())));

  const int kNumOfStackPages = 32;
  map_info.stack.Set(
      0xBEEF'0000,
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfStackPages),
      kNumOfStackPages << kPageSizeExponent);

  map_info.Print();
  LoadAndMap(user_page_table, map_info, phdr_map_info, kPageAttrUser);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);

  void* stack_pointer =
      reinterpret_cast<void*>(map_info.stack.GetVirtEndAddr());

  ExecutionContext& ctx = liumos->exec_ctx_ctrl->Create(
      reinterpret_cast<void (*)(void)>(entry_point), GDT::kUserCS64Selector,
      stack_pointer, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(&user_page_table), kRFlagsInterruptEnable,
      liumos->kernel_heap_allocator->AllocPages<uint64_t>(
          kKernelStackPagesForEachProcess,
          kPageAttrPresent | kPageAttrWritable) +
          kPageSize * kKernelStackPagesForEachProcess);
  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsEphemeralProcess(ctx);
  liumos->scheduler->RegisterProcess(proc);
  return proc;
}

Process& LoadELFAndLaunchPersistentProcess(File& file,
                                           PersistentMemoryManager& pmem) {
  PersistentProcessInfo& pp_info = *pmem.AllocPersistentProcessInfo();
  pp_info.Init();
  ProcessMappingInfo& map_info = pp_info.pmap[0];
  PhdrMappingInfo phdr_map_info;
  IA_PML4& user_page_table = CreatePageTable();

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);

  map_info.code.SetPhysAddr(pmem.AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.code.GetMapSize())));
  map_info.data.SetPhysAddr(pmem.AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.data.GetMapSize())));

  const int kNumOfStackPages = 32;
  map_info.stack.Set(0xBEEF'0000, pmem.AllocPages<uint64_t>(kNumOfStackPages),
                     kNumOfStackPages << kPageSizeExponent);

  map_info.Print();
  LoadAndMap(user_page_table, map_info, phdr_map_info, kPageAttrUser);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);

  void* stack_pointer =
      reinterpret_cast<void*>(map_info.stack.GetVirtEndAddr());

  ExecutionContext& ctx = liumos->exec_ctx_ctrl->Create(
      reinterpret_cast<void (*)(void)>(entry_point), GDT::kUserCS64Selector,
      stack_pointer, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(&user_page_table), kRFlagsInterruptEnable,
      liumos->kernel_heap_allocator->AllocPages<uint64_t>(
          kKernelStackPagesForEachProcess,
          kPageAttrPresent | kPageAttrWritable) +
          kPageSize * kKernelStackPagesForEachProcess);
  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsEphemeralProcess(ctx);
  liumos->scheduler->RegisterProcess(proc);
  return proc;
}

void LoadKernelELF(File& file) {
  ProcessMappingInfo map_info;
  PhdrMappingInfo phdr_map_info;

  const Elf64_Ehdr* ehdr = ParseProgramHeader(file, map_info, phdr_map_info);
  assert(ehdr);

  map_info.code.SetPhysAddr(liumos->dram_allocator->AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.code.GetMapSize())));
  map_info.data.SetPhysAddr(liumos->dram_allocator->AllocPages<uint64_t>(
      ByteSizeToPageSize(map_info.data.GetMapSize())));

  constexpr uint64_t kNumOfKernelMainStackPages = 2;
  uint64_t kernel_main_stack_virtual_base = 0xFFFF'FFFF'4000'0000ULL;

  map_info.stack.Set(
      kernel_main_stack_virtual_base,
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelMainStackPages),
      kNumOfKernelMainStackPages << kPageSizeExponent);

  LoadAndMap(GetKernelPML4(), map_info, phdr_map_info, 0);

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);

  uint64_t kernel_main_stack_pointer =
      kernel_main_stack_virtual_base +
      (kNumOfKernelMainStackPages << kPageSizeExponent);

  JumpToKernel(entry_point, liumos, kernel_main_stack_pointer);
}
