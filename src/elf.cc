#include "lib/musl/include/elf.h"
#include "liumos.h"

struct SegmentMapping {
  uint64_t paddr;
  uint64_t vaddr;
  uint64_t size;
};

struct ProcessMappingInfo {
  SegmentMapping code;
  SegmentMapping data;
  SegmentMapping stack;
};

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

const Elf64_Ehdr* LoadELF(File& file,
                          IA_PML4& page_root,
                          ProcessMappingInfo& proc_map_info) {
  const uint8_t* buf = file.GetBuf();
  // uint64_t buf_size = file.GetFileSize();
  PutString("Loading ELF...\n");
  const uint64_t file_size = file.GetFileSize();
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

  PutString("Program headers to be loaded:\n");
  for (int i = 0; i < ehdr->e_phnum; i++) {
    const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
        buf + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD)
      continue;
    PutString("Phdr #");
    PutHex64(i);
    PrintProgramHeader(phdr);

    SegmentMapping* seg_map = nullptr;
    if (phdr->p_flags & PF_X)
      seg_map = &proc_map_info.code;
    if (phdr->p_flags & PF_W)
      seg_map = &proc_map_info.data;

    assert(IsAlignedToPageSize(phdr->p_align));

    const uint64_t map_base_file_ofs = FloorToPageAlignment(phdr->p_offset);
    const uint64_t map_base_vaddr = FloorToPageAlignment(phdr->p_vaddr);
    const uint64_t map_size =
        CeilToPageAlignment(phdr->p_memsz + (phdr->p_vaddr - map_base_vaddr));
    const size_t copy_size = phdr->p_filesz + (phdr->p_vaddr - map_base_vaddr);
    PutString("  map_base_file_ofs:");
    PutHex64ZeroFilled(map_base_file_ofs);
    PutString(" map_base_vaddr:");
    PutHex64ZeroFilled(map_base_vaddr);
    PutString(" map_size:");
    PutHex64ZeroFilled(map_size);
    PutString(" copy_size:");
    PutHex64ZeroFilled(copy_size);
    PutChar('\n');
    uint64_t page_attr = kPageAttrPresent | kPageAttrUser |
                         ((phdr->p_flags & PF_W) ? kPageAttrWritable : 0);
    uint8_t* phys_buf = liumos->dram_allocator->AllocPages<uint8_t*>(
        ByteSizeToPageSize(map_size));
    memcpy(phys_buf, buf + map_base_file_ofs, copy_size);
    bzero(phys_buf + copy_size, map_size - copy_size);
    CreatePageMapping(*liumos->dram_allocator, page_root, map_base_vaddr,
                      reinterpret_cast<uint64_t>(phys_buf), map_size,
                      page_attr);
  }
  PutStringAndHex("Entry Point", ehdr->e_entry);

  return ehdr;
}

ExecutionContext* LoadELFAndLaunchProcess(File& file) {
  ProcessMappingInfo info;
  IA_PML4& user_page_table = CreatePageTable();
  const Elf64_Ehdr* ehdr = LoadELF(file, user_page_table, info);
  if (!ehdr)
    return nullptr;

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);

  const int kNumOfStackPages = 32;
  info.stack.size = kNumOfStackPages << kPageSizeExponent;
  uint64_t stack_phys_base_addr = liumos->dram_allocator->AllocPages<uint64_t>(
      ByteSizeToPageSize(info.stack.size));
  constexpr uint64_t stack_virt_base_addr = 0xBEEF'0000;
  CreatePageMapping(*liumos->dram_allocator, user_page_table,
                    stack_virt_base_addr, stack_phys_base_addr, info.stack.size,
                    kPageAttrPresent | kPageAttrUser | kPageAttrWritable);
  void* stack_pointer =
      reinterpret_cast<void*>(stack_virt_base_addr + info.stack.size);

  ExecutionContext* ctx = liumos->exec_ctx_ctrl->Create(
      reinterpret_cast<void (*)(void)>(entry_point), GDT::kUserCS64Selector,
      stack_pointer, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(&user_page_table), kRFlagsInterruptEnable,
      liumos->kernel_heap_allocator->AllocPages<uint64_t>(
          kKernelStackPagesForEachProcess,
          kPageAttrPresent | kPageAttrWritable) +
          kPageSize * kKernelStackPagesForEachProcess);
  liumos->scheduler->RegisterExecutionContext(ctx);
  return ctx;
}

void LoadKernelELF(File& file) {
  ProcessMappingInfo info;
  const Elf64_Ehdr* ehdr = LoadELF(file, GetKernelPML4(), info);
  if (!ehdr)
    Panic("Failed to load kernel ELF");

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address: ", entry_point);
  PutString("Data at entrypoint: ");
  for (int i = 0; i < 16; i++) {
    PutHex8ZeroFilled(entry_point[i]);
    PutChar(' ');
  }
  PutChar('\n');

  constexpr uint64_t kNumOfKernelMainStackPages = 2;
  uint64_t kernel_main_stack_physical_base =
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelMainStackPages);
  uint64_t kernel_main_stack_virtual_base = 0xFFFF'FFFF'4000'0000ULL;
  CreatePageMapping(*liumos->dram_allocator, GetKernelPML4(),
                    kernel_main_stack_virtual_base,
                    kernel_main_stack_physical_base,
                    kNumOfKernelMainStackPages << kPageSizeExponent,
                    kPageAttrPresent | kPageAttrWritable);
  uint64_t kernel_main_stack_pointer =
      kernel_main_stack_virtual_base +
      (kNumOfKernelMainStackPages << kPageSizeExponent);

  JumpToKernel(entry_point, liumos, kernel_main_stack_pointer);
}
