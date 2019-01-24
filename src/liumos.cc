#include "liumos.h"
#include "cpu_context.h"
#include "execution_context.h"
#include "hpet.h"

EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator* page_allocator;
int kMaxPhyAddr;

HPET hpet;

PhysicalPageAllocator page_allocator_;

File hello_bin_file;

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
  StoreIntFlag();
  uint32_t col = 0xff0000;
  int x = 0;
  int move_width = 128;
  while (1) {
    DrawRect(xsize - 20 - move_width, 10, 20 + move_width, 20, 0xffffff);
    DrawRect(xsize - 20 - move_width + x, 10, 20, 20, col);
    x = (x + 4) & 127;
    StoreIntFlagAndHalt();
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
        } else if (IsEqualString(line, "hello.bin")) {
          ParseELFFile(hello_bin_file);
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

void OpenAndPrintLogoFile() {
  File logo_file;
  logo_file.LoadFromEFISimpleFS(L"logo.ppm");
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
  constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000;
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

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table) {
  LocalAPIC local_apic;
  GDT gdt;

  EFI::Init(system_table);
  EFI::ConOut::ClearScreen();
  InitGraphics();
  EnableVideoModeForConsole();
  OpenAndPrintLogoFile();
  hello_bin_file.LoadFromEFISimpleFS(L"hello.bin");
  EFI::GetMemoryMapAndExitBootServices(image_handle, efi_memory_map);

  PutString("\nliumOS is booting...\n\n");

  ClearIntFlag();

  gdt.Init();
  InitIDT();
  new (&page_allocator) PhysicalPageAllocator();
  InitMemoryManagement(efi_memory_map);
  InitPaging();
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

  const int kNumOfStackPages = 3;
  void* sub_context_stack_base = page_allocator->AllocPages(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("alloc addr", sub_context_stack_base);

  ExecutionContext sub_context(2, SubTask, ReadCSSelector(), sub_context_rsp,
                               ReadSSSelector());
  scheduler->RegisterExecutionContext(&sub_context);

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
