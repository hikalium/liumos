#include "lib/musl/include/elf.h"
#include "liumos.h"

struct ProcessMappingInfo {
  uint64_t file_paddr;
  uint64_t file_vaddr;
  uint64_t file_size;
  uint64_t stack_paddr;
  uint64_t stack_vaddr;
  uint64_t stack_size;
};

const Elf64_Ehdr* LoadELF(ProcessMappingInfo& info,
                          File& file,
                          IA_PML4& page_root) {
  const uint8_t* buf = file.GetBuf();
  // uint64_t buf_size = file.GetFileSize();
  PutString("Loading ELF...\n");
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
  /*
  PutString("sections:\n");
  const Elf64_Shdr* shstr = reinterpret_cast<const Elf64_Shdr*>(
      buf + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
  for (int i = 0; i < ehdr->e_shnum; i++) {
    const Elf64_Shdr* shdr = reinterpret_cast<const Elf64_Shdr*>(
        buf + ehdr->e_shoff + ehdr->e_shentsize * i);
    PutString(
        reinterpret_cast<const char*>(buf + shstr->sh_offset + shdr->sh_name));
    PutStringAndHex(" @+", shdr->sh_offset);
    PutStringAndHex("    @", shdr->sh_addr);
    PutStringAndHex("    size = ", shdr->sh_size);
    PutStringAndHex("    size = ", shdr->sh_flags);
  }
  */
  PutString("Program headers to be loaded:\n");
  for (int i = 0; i < ehdr->e_phnum; i++) {
    const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
        buf + ehdr->e_phoff + ehdr->e_phentsize * i);
    if (phdr->p_type != PT_LOAD)
      continue;
    PutString("Phdr #");
    PutHex64(i);
    PutString(" type:LOAD");
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
    const uint64_t map_base_file_ofs =
        phdr->p_offset & ~(phdr->p_align - 1) & ~kPageAddrMask;
    const uint64_t map_base_vaddr =
        phdr->p_vaddr & ~(phdr->p_align - 1) & ~kPageAddrMask;
    const uint64_t map_size =
        (phdr->p_memsz + (phdr->p_offset - map_base_file_ofs) + kPageAddrMask) &
        ~kPageAddrMask;
    PutString("  map_base_file_ofs:");
    PutHex64ZeroFilled(map_base_file_ofs);
    PutString(" map_base_vaddr:");
    PutHex64ZeroFilled(map_base_vaddr);
    PutString(" map_size:");
    PutHex64ZeroFilled(map_size);
    PutChar('\n');
    uint64_t page_attr = kPageAttrPresent;
    if (phdr->p_flags & PF_W)
      page_attr |= kPageAttrWritable;
    CreatePageMapping(*dram_allocator, page_root, map_base_vaddr,
                      reinterpret_cast<uint64_t>(buf) + map_base_file_ofs,
                      map_size, page_attr);
  }
  PutStringAndHex("Entry Point", ehdr->e_entry);

  const uint64_t file_size = file.GetFileSize();
  PutStringAndHex("File Size", file_size);
  uint8_t* body =
      dram_allocator->AllocPages<uint8_t*>(ByteSizeToPageSize(file_size));
  PutStringAndHex("Load Addr", body);
  memcpy(body, buf, file_size);

  info.file_paddr = reinterpret_cast<uint64_t>(body);
  info.file_vaddr = info.file_paddr;
  return ehdr;
}

ExecutionContext* LoadELFAndLaunchProcess(File& file) {
  ProcessMappingInfo info;
  const Elf64_Ehdr* ehdr = LoadELF(info, file, GetKernelPML4());
  if (!ehdr)
    return nullptr;

  uint8_t* entry_point =
      reinterpret_cast<uint8_t*>(info.file_paddr) + (ehdr->e_entry - 0x400000);
  PutStringAndHex("Entry address(physical)", entry_point);
  PutString("Data at entrypoint: ");
  for (int i = 0; i < 16; i++) {
    PutHex8ZeroFilled(entry_point[i]);
    PutChar(' ');
  }
  PutChar('\n');

  const int kNumOfStackPages = 3;
  info.stack_size = kNumOfStackPages << kPageSizeExponent;
  info.stack_paddr =
      dram_allocator->AllocPages<uint64_t>(ByteSizeToPageSize(info.stack_size));
  info.stack_vaddr = info.stack_paddr;
  void* sub_context_rsp =
      reinterpret_cast<void*>(info.stack_vaddr + info.stack_size);

  ExecutionContext* ctx = CreateExecutionContext(
      reinterpret_cast<void (*)(void)>(entry_point), GDT::kUserCSSelector,
      sub_context_rsp, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(CreatePageTable()));
  scheduler->RegisterExecutionContext(ctx);
  return ctx;
}

void LoadKernelELF(File& file) {
  ProcessMappingInfo info;
  const Elf64_Ehdr* ehdr = LoadELF(info, file, GetKernelPML4());
  if (!ehdr)
    Panic("Failed to load kernel ELF");

  uint8_t* entry_point = reinterpret_cast<uint8_t*>(ehdr->e_entry);
  PutStringAndHex("Entry address(physical)", entry_point);
  PutString("Data at entrypoint: ");
  for (int i = 0; i < 16; i++) {
    PutHex8ZeroFilled(entry_point[i]);
    PutChar(' ');
  }
  PutChar('\n');

  JumpToKernel(entry_point, liumos);

  for (;;) {
    StoreIntFlagAndHalt();
  }

  const int kNumOfStackPages = 3;
  info.stack_size = kNumOfStackPages << kPageSizeExponent;
  info.stack_paddr =
      dram_allocator->AllocPages<uint64_t>(ByteSizeToPageSize(info.stack_size));
  info.stack_vaddr = info.stack_paddr;
  void* sub_context_rsp =
      reinterpret_cast<void*>(info.stack_vaddr + info.stack_size);

  ExecutionContext* ctx = CreateExecutionContext(
      reinterpret_cast<void (*)(void)>(entry_point), GDT::kUserCSSelector,
      sub_context_rsp, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(CreatePageTable()));
  scheduler->RegisterExecutionContext(ctx);
}
