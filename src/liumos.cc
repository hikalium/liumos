#include "liumos.h"
#include "cpu_context.h"
#include "execution_context.h"
#include "hpet.h"

EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator* page_allocator;

HPET hpet;
GDT global_desc_table;

PhysicalPageAllocator page_allocator_;

void InitMemoryManagement(EFI::MemoryMap& map) {
  page_allocator = &page_allocator_;
  new (page_allocator) PhysicalPageAllocator();
  int available_pages = 0;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    if (desc->type != EFI::MemoryType::kConventionalMemory)
      continue;
    available_pages += desc->number_of_pages;
    page_allocator->FreePages(reinterpret_cast<void*>(desc->physical_start),
                              desc->number_of_pages);
  }
  PutStringAndHex("Available memory (KiB)", available_pages * 4);
}

void SubTask() {
  int count = 0;
  while (1) {
    StoreIntFlagAndHalt();
    PutStringAndHex("SubContext!", count++);
  }
}

uint16_t ParseKeyCode(uint8_t keycode);

class TextBox {
 public:
  TextBox() : buf_used_(0), is_recording_enabled_(false) {}
  void putc(uint16_t keyid) {
    if (keyid & KeyID::kMaskBreak)
      return;
    if (keyid == KeyID::kBackspace) {
      if (is_recording_enabled_) {
        if (buf_used_ == 0)
          return;
        buf_[--buf_used_] = 0;
      }
      PutChar('\b');
      return;
    }
    if (keyid & KeyID::kMaskExtended)
      return;
    if (is_recording_enabled_) {
      if (buf_used_ >= kSizeOfBuffer)
        return;
      buf_[buf_used_++] = (uint8_t)keyid;
      buf_[buf_used_] = 0;
    }
    PutChar(keyid);
  }
  void StartRecording() {
    buf_used_ = 0;
    buf_[buf_used_] = 0;
    is_recording_enabled_ = true;
  }
  void StopRecording() { is_recording_enabled_ = false; }
  const char* GetRecordedString() { return buf_; }

 private:
  constexpr static int kSizeOfBuffer = 16;
  char buf_[kSizeOfBuffer + 1];
  int buf_used_;
  bool is_recording_enabled_;
};

bool IsEqualString(const char* a, const char* b) {
  while (*a == *b) {
    if (*a == 0)
      return true;
    a++;
    b++;
  }
  return false;
}

void WaitAndProcessCommand(TextBox& tbox) {
  PutString("> ");
  tbox.StartRecording();
  while (1) {
    StoreIntFlagAndHalt();
    ClearIntFlag();
    while (!keycode_buffer.IsEmpty()) {
      uint16_t keyid = ParseKeyCode(keycode_buffer.Pop());
      if (!keyid && keyid & KeyID::kMaskBreak)
        continue;
      if (keyid == KeyID::kEnter) {
        tbox.StopRecording();
        tbox.putc('\n');
        const char* line = tbox.GetRecordedString();
        if (IsEqualString(line, "hello")) {
          PutString("Hello, world!\n");
        } else if (IsEqualString(line, "show nfit")) {
          ConsoleCommand::ShowNFIT();
        } else if (IsEqualString(line, "show madt")) {
          ConsoleCommand::ShowMADT();
        } else if (IsEqualString(line, "show mmap")) {
          ConsoleCommand::ShowEFIMemoryMap();
        } else if (IsEqualString(line, "free")) {
          ConsoleCommand::Free();
        } else {
          PutString("Command not found: ");
          PutString(tbox.GetRecordedString());
          tbox.putc('\n');
        }
        return;
      }
      tbox.putc(keyid);
    }
  }
}

constexpr uint64_t kPageSizeExponent = 12;
constexpr uint64_t kPageSize = 1 << kPageSizeExponent;
inline uint64_t ByteSizeToPageSize(uint64_t byte_size) {
  return (byte_size + kPageSize - 1) >> kPageSizeExponent;
}

class File {
 public:
  File(const wchar_t* file_name) {
    for (int i = 0; i < kFileNameSize; i++) {
      file_name_[i] = (char)file_name[i];
      if (!file_name[i])
        break;
    }
    file_name_[kFileNameSize] = 0;
    EFI::FileProtocol* file = EFI::OpenFile(file_name);
    EFI::FileInfo info;
    EFI::ReadFileInfo(file, &info);
    EFI::UINTN buf_size = info.file_size;
    buf_pages_ = reinterpret_cast<uint8_t*>(
        EFI::AllocatePages(ByteSizeToPageSize(buf_size)));
    if (file->Read(file, &buf_size, buf_pages_) != EFI::Status::kSuccess) {
      PutString("Read failed\n");
      return;
    }
    assert(buf_size == info.file_size);
    file_size_ = info.file_size;
    return;
  }
  const uint8_t* GetBuf() { return buf_pages_; }
  uint64_t GetFileSize() { return file_size_; }

 private:
  static constexpr int kFileNameSize = 16;
  char file_name_[kFileNameSize + 1];
  uint64_t file_size_;
  uint8_t* buf_pages_;
};

void OpenAndPrintLogoFile() {
  File logo_file(L"logo.ppm");
  const uint8_t* buf = logo_file.GetBuf();
  uint64_t buf_size = logo_file.GetFileSize();
  if (buf[0] != 'P' || buf[1] != '3') {
    PutString("Not supported logo type (PPM 'P3' is supported)\n");
    return;
  }
  bool in_line_comment = false;
  bool is_num_read = false;
  int width = 0;
  int height = 0;
  int max_pixel_value = 0;
  uint32_t rgb = 0;
  uint32_t tmp = 0;
  int channel_count = 0;
  int x = 0, y = 0;
  for (int i = 3; i < (int)buf_size; i++) {
    uint8_t c = buf[i];
    if (in_line_comment) {
      if (c != '\n')
        continue;
      in_line_comment = false;
      continue;
    }
    if (c == '#') {
      in_line_comment = true;
      continue;
    }
    if ('0' <= c && c <= '9') {
      is_num_read = true;
      tmp *= 10;
      tmp += (uint32_t)c - '0';
      continue;
    }
    if (!is_num_read)
      continue;
    is_num_read = false;
    if (!width) {
      width = tmp;
    } else if (!height) {
      height = tmp;
    } else if (!max_pixel_value) {
      max_pixel_value = tmp;
      assert(max_pixel_value == 255);
    } else {
      rgb <<= 8;
      rgb |= tmp;
      channel_count++;
      if (channel_count == 3) {
        channel_count = 0;
        DrawRect(xsize - width + x++, y, 1, 1, rgb);
        if (x >= width) {
          x = 0;
          y++;
        }
      }
    }
    tmp = 0;
  }
}

#include "lib/musl/include/elf.h"
void OpenAndPrintELFFile() {
  File logo_file(L"hello.bin");
  const uint8_t* buf = logo_file.GetBuf();
  uint64_t buf_size = logo_file.GetFileSize();
  PutString("Loading ELF...\n");
  if (strncmp(reinterpret_cast<const char*>(buf), ELFMAG, SELFMAG) != 0) {
    PutString("Not an ELF file\n");
    return;
  }
  if (buf[EI_CLASS] != ELFCLASS64) {
    PutString("Not an ELF Class 64n");
    return;
  }
  if (buf[EI_DATA] != ELFDATA2LSB) {
    PutString("Not an ELF Data 2LSB");
    return;
  }
  if (buf[EI_OSABI] != ELFOSABI_SYSV) {
    PutString("Not a SYSV ABI");
    return;
  }
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(buf);
  if (ehdr->e_type != ET_EXEC) {
    PutString("Not an executable");
    return;
  }
  if (ehdr->e_machine != EM_X86_64) {
    PutString("Not for x86_64");
    return;
  }
  PutString("This is an ELF file\n");
  PutString("sections:\n");
  const Elf64_Shdr* shstr = reinterpret_cast<const Elf64_Shdr*>(
      buf + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
  for (int i = 0; i < ehdr->e_shnum; i++) {
    const Elf64_Shdr* shdr = reinterpret_cast<const Elf64_Shdr*>(
        buf + ehdr->e_shoff + ehdr->e_shentsize * i);
    PutString(
        reinterpret_cast<const char*>(buf + shstr->sh_offset + shdr->sh_name));
    PutChar('\n');
  }
  PutString("Program headers:\n");
  for (int i = 0; i < ehdr->e_phnum; i++) {
    const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
        buf + ehdr->e_phoff + ehdr->e_phentsize * i);
    PutString("Phdr #");
    PutHex64(i);
    PutString(" type:");
    if (phdr->p_type == PT_NULL)
      PutString("NULL");
    else if (phdr->p_type == PT_LOAD)
      PutString("LOAD");
    else if (phdr->p_type == PT_GNU_STACK)
      PutString("GNU_STACK");
    else if (phdr->p_type == PT_GNU_RELRO)
      PutString("GNU_RELRO");
    else
      PutHex64(phdr->p_type);
    PutString(" flags:");
    if (phdr->p_flags & PF_R)
      PutChar('R');
    if (phdr->p_flags & PF_W)
      PutChar('W');
    if (phdr->p_flags & PF_X)
      PutChar('X');
    PutChar('\n');
    PutStringAndHex("Phdr offset", phdr->p_offset);
    PutString("ofsset:");
    PutHex64ZeroFilled(phdr->p_offset);
    PutString(" vaddr:");
    PutHex64ZeroFilled(phdr->p_vaddr);
    PutString(" paddr:");
    PutHex64ZeroFilled(phdr->p_paddr);
    PutChar('\n');
    PutString("filesz:");
    PutHex64ZeroFilled(phdr->p_filesz);
    PutString(" memsz:");
    PutHex64ZeroFilled(phdr->p_memsz);
    PutString(" align:");
    PutHex64ZeroFilled(phdr->p_align);
    PutChar('\n');
  }
  PutStringAndHex("Entry Point", ehdr->e_entry);
}

void ReadFilesFromEFISimpleFileSystem() {
  OpenAndPrintLogoFile();
  OpenAndPrintELFFile();
}

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table) {
  LocalAPIC local_apic;

  EFI::Init(system_table);
  EFI::ConOut::ClearScreen();
  InitGraphics();
  EnableVideoModeForConsole();
  ReadFilesFromEFISimpleFileSystem();
  EFI::GetMemoryMapAndExitBootServices(image_handle, efi_memory_map);

  PutString("\nliumOS is booting...\n\n");

  ClearIntFlag();

  new (&global_desc_table) GDT();
  InitIDT();
  ExecutionContext root_context(1, NULL, 0, NULL, 0);
  Scheduler scheduler_(&root_context);
  scheduler = &scheduler_;

  CPUID cpuid;
  ReadCPUID(&cpuid, 0, 0);
  PutStringAndHex("Max CPUID", cpuid.eax);

  ReadCPUID(&cpuid, 1, 0);
  if (!(cpuid.edx & CPUID_01_EDX_APIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & CPUID_01_EDX_MSR))
    Panic("MSR not supported");

  ACPI::DetectTables();

  new (&local_apic) LocalAPIC();
  Disable8259PIC();

  uint64_t local_apic_id = local_apic.GetID();
  PutStringAndHex("LOCAL APIC ID", local_apic_id);

  InitIOAPIC(local_apic_id);

  hpet.Init(
      static_cast<HPET::RegisterSpace*>(ACPI::hpet->base_address.address));

  hpet.SetTimerMs(
      0, 100, HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);

  new (&page_allocator) PhysicalPageAllocator();
  InitMemoryManagement(efi_memory_map);
  const int kNumOfStackPages = 3;
  void* sub_context_stack_base = page_allocator->AllocPages(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("alloc addr", sub_context_stack_base);

  ExecutionContext sub_context(2, SubTask, ReadCSSelector(), sub_context_rsp,
                               ReadSSSelector());
  // scheduler->RegisterExecutionContext(&sub_context);

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
