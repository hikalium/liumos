#include "liumos.h"
#include "cpu_context.h"
#include "execution_context.h"
#include "hpet.h"

EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator* page_allocator;
int kMaxPhyAddr;
LocalAPIC bsp_local_apic;
CPUFeatureSet cpu_features;
SerialPort com1;
File hello_bin_file;
File liumos_elf_file;

HPET hpet;

PhysicalPageAllocator page_allocator_;

File logo_file;

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
    screen_sheet->DrawRect(screen_sheet->GetXSize() - 20 - move_width, 10,
                           20 + move_width, 20, 0xffffff);
    screen_sheet->DrawRect(screen_sheet->GetXSize() - 20 - move_width + x, 10,
                           20, 20, col);
    x = (x + 4) & 127;
    StoreIntFlagAndHalt();
  }
}

uint16_t ParseKeyCode(uint8_t keycode);

void WaitAndProcessCommand(TextBox& tbox) {
  PutString("> ");
  tbox.StartRecording();
  while (1) {
    StoreIntFlagAndHalt();
    while (1) {
      uint16_t keyid;
      if (!keycode_buffer.IsEmpty()) {
        keyid = ParseKeyCode(keycode_buffer.Pop());
        if (!keyid && keyid & KeyID::kMaskBreak)
          continue;
        if (keyid == KeyID::kEnter) {
          keyid = '\n';
        }
      } else if (com1.IsReceived()) {
        keyid = com1.ReadCharReceived();
        if (keyid == '\n') {
          continue;
        }
        if (keyid == '\r') {
          keyid = '\n';
        }
      } else {
        break;
      }
      if (keyid == '\n') {
        tbox.StopRecording();
        tbox.putc('\n');
        ConsoleCommand::Process(tbox);
        return;
      }
      tbox.putc(keyid);
    }
  }
}

void PrintLogoFile() {
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
        screen_sheet->DrawRect(screen_sheet->GetXSize() - width + x++, y, 1, 1,
                               rgb);
        if (x >= width) {
          x = 0;
          y++;
        }
      }
    }
    tmp = 0;
  }
}

void IdentifyCPU() {
  CPUID cpuid;

  ReadCPUID(&cpuid, 0, 0);
  const uint32_t kMaxCPUID = cpuid.eax;
  PutStringAndHex("Max CPUID", kMaxCPUID);
  cpu_features.max_cpuid = kMaxCPUID;

  ReadCPUID(&cpuid, 0x8000'0000, 0);
  const uint32_t kMaxExtendedCPUID = cpuid.eax;
  PutStringAndHex("Max Extended CPUID", kMaxExtendedCPUID);
  cpu_features.max_extended_cpuid = kMaxExtendedCPUID;

  ReadCPUID(&cpuid, 1, 0);
  uint8_t cpu_family = ((cpuid.eax >> 16) & 0xff0) | ((cpuid.eax >> 8) & 0xf);
  uint8_t cpu_model = ((cpuid.eax >> 12) & 0xf0) | ((cpuid.eax >> 4) & 0xf);
  uint8_t cpu_stepping = cpuid.eax & 0xf;
  PutStringAndHex("CPU family  ", cpu_family);
  PutStringAndHex("CPU model   ", cpu_model);
  PutStringAndHex("CPU stepping", cpu_stepping);
  if (!(cpuid.edx & kCPUID01H_EDXBitAPIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & kCPUID01H_EDXBitMSR))
    Panic("MSR not supported");
  cpu_features.x2apic = cpuid.ecx & kCPUID01H_ECXBitx2APIC;

  if (0x8000'0004 <= kMaxExtendedCPUID) {
    for (int i = 0; i < 3; i++) {
      ReadCPUID(&cpuid, 0x8000'0002 + i, 0);
      *reinterpret_cast<uint32_t*>(&cpu_features.brand_string[i * 16 + 0]) =
          cpuid.eax;
      *reinterpret_cast<uint32_t*>(&cpu_features.brand_string[i * 16 + 4]) =
          cpuid.ebx;
      *reinterpret_cast<uint32_t*>(&cpu_features.brand_string[i * 16 + 8]) =
          cpuid.ecx;
      *reinterpret_cast<uint32_t*>(&cpu_features.brand_string[i * 16 + 12]) =
          cpuid.edx;
    }
    PutString(cpu_features.brand_string);
    PutChar('\n');
  }

  if (CPUIDIndex::kMaxAddr <= kMaxExtendedCPUID) {
    ReadCPUID(&cpuid, CPUIDIndex::kMaxAddr, 0);
    IA32_MaxPhyAddr maxaddr;
    maxaddr.data = cpuid.eax;
    kMaxPhyAddr = maxaddr.bits.physical_address_bits;
  } else {
    PutString("CPUID function 80000008H not supported.\n");
    PutString("Assuming Physical address bits = 36\n");
    kMaxPhyAddr = 36;
  }
  PutStringAndHex("kMaxPhyAddr", kMaxPhyAddr);
}

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table) {
  GDT gdt;

  EFI::Init(system_table);
  EFI::ConOut::ClearScreen();
  logo_file.LoadFromEFISimpleFS(L"logo.ppm");
  hello_bin_file.LoadFromEFISimpleFS(L"hello.bin");
  liumos_elf_file.LoadFromEFISimpleFS(L"LIUMOS.ELF");
  InitGraphics();
  EnableVideoModeForConsole();
  EFI::GetMemoryMapAndExitBootServices(image_handle, efi_memory_map);

  com1.Init(kPortCOM1);
  SetSerialForConsole(&com1);

  PrintLogoFile();
  PutString("\nliumOS is booting...\n\n");
  ClearIntFlag();

  ACPI::DetectTables();

  new (&page_allocator) PhysicalPageAllocator();
  InitMemoryManagement(efi_memory_map);

  InitDoubleBuffer();

  IdentifyCPU();
  gdt.Init();
  InitIDT();
  InitPaging();

  ExecutionContext root_context(1, NULL, 0, NULL, 0, ReadCR3());
  Scheduler scheduler_(&root_context);
  scheduler = &scheduler_;

  bsp_local_apic.Init();
  Disable8259PIC();

  InitIOAPIC(bsp_local_apic.GetID());

  hpet.Init(
      static_cast<HPET::RegisterSpace*>(ACPI::hpet->base_address.address));
  hpet.SetTimerMs(
      0, 10, HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);
  const int kNumOfStackPages = 3;
  void* sub_context_stack_base =
      page_allocator->AllocPages<void*>(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("alloc addr", sub_context_stack_base);

  ExecutionContext sub_context(2, SubTask, ReadCSSelector(), sub_context_rsp,
                               ReadSSSelector(),
                               reinterpret_cast<uint64_t>(CreatePageTable()));
  scheduler->RegisterExecutionContext(&sub_context);

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
