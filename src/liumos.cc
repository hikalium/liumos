#include "liumos.h"
#include "cpu_context.h"
#include "execution_context.h"
#include "hpet.h"

EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator* dram_allocator;
PhysicalPageAllocator* pmem_allocator;
int kMaxPhyAddr;
LocalAPIC bsp_local_apic;
CPUFeatureSet cpu_features;
SerialPort com1;
File hello_bin_file;
File liumos_elf_file;
HPET hpet;

PhysicalPageAllocator dram_allocator_;
PhysicalPageAllocator pmem_allocator_;

File logo_file;

void InitDRAMManagement(EFI::MemoryMap& map) {
  dram_allocator = &dram_allocator_;
  new (dram_allocator) PhysicalPageAllocator();
  int available_pages = 0;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    if (desc->type != EFI::MemoryType::kConventionalMemory)
      continue;
    available_pages += desc->number_of_pages;
    dram_allocator->FreePages(reinterpret_cast<void*>(desc->physical_start),
                              desc->number_of_pages);
  }
  PutStringAndHex("Available DRAM (KiB)", available_pages * 4);
}

void InitPMEMManagement() {
  using namespace ACPI;
  pmem_allocator = &pmem_allocator_;
  new (pmem_allocator) PhysicalPageAllocator();
  if (!nfit) {
    PutString("NFIT not found. There are no PMEMs on this system.\n");
    return;
  }
  uint64_t available_pmem_size = 0;

  for (auto& it : *nfit) {
    if (it.type != NFIT::Entry::kTypeSPARangeStructure)
      continue;
    NFIT::SPARange* spa_range = reinterpret_cast<NFIT::SPARange*>(&it);
    if (!IsEqualGUID(
            reinterpret_cast<GUID*>(&spa_range->address_range_type_guid),
            &NFIT::SPARange::kByteAdressablePersistentMemory))
      continue;
    PutStringAndHex("SPARange #", spa_range->spa_range_structure_index);
    PutStringAndHex("  Base", spa_range->system_physical_address_range_base);
    PutStringAndHex("  Length",
                    spa_range->system_physical_address_range_length);
    available_pmem_size += spa_range->system_physical_address_range_length;
    pmem_allocator->FreePages(
        reinterpret_cast<void*>(spa_range->system_physical_address_range_base),
        spa_range->system_physical_address_range_length >> kPageSizeExponent);
  }
  PutStringAndHex("Available PMEM (KiB)", available_pmem_size >> 10);
}

void InitMemoryManagement(EFI::MemoryMap& map) {
  InitDRAMManagement(map);
  InitPMEMManagement();
}

void SubTask() {
  constexpr int map_ysize_shift = 4;
  constexpr int map_xsize_shift = 5;
  constexpr int pixel_size = 8;

  constexpr int ysize_mask = (1 << map_ysize_shift) - 1;
  constexpr int xsize_mask = (1 << map_xsize_shift) - 1;
  constexpr int ysize = 1 << map_ysize_shift;
  constexpr int xsize = 1 << map_xsize_shift;
  static char map[ysize * xsize];
  constexpr int canvas_ysize = ysize * pixel_size;
  constexpr int canvas_xsize = xsize * pixel_size;

  map[(ysize / 2 - 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 - 1) * xsize + (xsize / 2 + 2)] = 1;

  map[(ysize / 2) * xsize + (xsize / 2 - 4)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 2)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 3)] = 1;

  map[(ysize / 2 + 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 + 1) * xsize + (xsize / 2 + 2)] = 1;

  while (1) {
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int count = 0;
        for (int p = -1; p <= 1; p++)
          for (int q = -1; q <= 1; q++)
            count +=
                map[((y + p) & ysize_mask) * xsize + ((x + q) & xsize_mask)] &
                1;
        count -= map[y * xsize + x] & 1;
        if ((map[y * xsize + x] && (count == 2 || count == 3)) ||
            (!map[y * xsize + x] && count == 3))
          map[y * xsize + x] |= 2;
      }
    }
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int p = map[y * xsize + x];
        int col = 0x000000;
        if (p & 1)
          col = 0xff0088 * (p & 2) + 0x00cc00 * (p & 1);
        map[y * xsize + x] >>= 1;
        screen_sheet->DrawRectWithoutFlush(
            screen_sheet->GetXSize() - canvas_xsize + x * pixel_size,
            y * pixel_size, pixel_size, pixel_size, col);
      }
    }
    screen_sheet->Flush(screen_sheet->GetXSize() - canvas_xsize, 0,
                        canvas_xsize, canvas_ysize);
    hpet.BusyWait(200);
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
        screen_sheet->DrawRect(screen_sheet->GetXSize() - width + x++, y + 128,
                               1, 1, rgb);
        if (x >= width) {
          x = 0;
          y++;
        }
      }
    }
    tmp = 0;
  }
}

constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_arch_prctl = 158;
constexpr uint64_t kArchSetGS = 0x1001;
constexpr uint64_t kArchSetFS = 0x1002;
constexpr uint64_t kArchGetFS = 0x1003;
constexpr uint64_t kArchGetGS = 0x1004;

extern "C" void SyscallHandler(uint64_t* args) {
  uint64_t idx = args[0];
  if (idx == kSyscallIndex_sys_write) {
    const uint64_t fildes = args[1];
    const uint8_t* buf = reinterpret_cast<uint8_t*>(args[2]);
    uint64_t nbyte = args[3];
    if (fildes != 1) {
      PutStringAndHex("fildes", fildes);
      Panic("Only stdout is supported for now.");
    }
    while (nbyte--) {
      PutChar(*(buf++));
    }
    return;
  } else if (idx == kSyscallIndex_sys_exit) {
    const uint64_t exit_code = args[1];
    PutStringAndHex("exit: exit_code", exit_code);
    scheduler->KillCurrentContext();
    for (;;) {
      StoreIntFlagAndHalt();
    };
  } else if (idx == kSyscallIndex_arch_prctl) {
    if (args[1] == kArchSetFS) {
      WriteMSR(MSRIndex::kFSBase, args[2]);
      return;
    }
    PutStringAndHex("arg1", args[1]);
    PutStringAndHex("arg2", args[2]);
    PutStringAndHex("arg3", args[3]);
    Panic("arch_prctl!");
  }
  PutStringAndHex("idx", idx);
  Panic("syscall handler!");
}

void EnableSyscall() {
  uint64_t star = GDT::kKernelCSSelector << 32;
  star |= GDT::kUserCSSelector << 48;
  WriteMSR(MSRIndex::kSTAR, star);

  uint64_t lstar = reinterpret_cast<uint64_t>(AsmSyscallHandler);
  WriteMSR(MSRIndex::kLSTAR, lstar);

  uint64_t efer = ReadMSR(MSRIndex::kEFER);
  efer |= 1;  // SCE
  WriteMSR(MSRIndex::kEFER, efer);
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
  PutString("\nliumOS version: " GIT_HASH "\n\n");
  ClearIntFlag();

  ACPI::DetectTables();

  InitMemoryManagement(efi_memory_map);

  InitDoubleBuffer();

  IdentifyCPU();
  gdt.Init();
  InitIDT();
  InitPaging();

  ExecutionContext* root_context =
      CreateExecutionContext(nullptr, 0, nullptr, 0, ReadCR3());
  Scheduler scheduler_(root_context);
  scheduler = &scheduler_;
  EnableSyscall();

  bsp_local_apic.Init();
  Disable8259PIC();

  InitIOAPIC(bsp_local_apic.GetID());

  hpet.Init(
      static_cast<HPET::RegisterSpace*>(ACPI::hpet->base_address.address));
  hpet.SetTimerMs(
      0, 10, HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);
  const int kNumOfStackPages = 3;
  void* sub_context_stack_base =
      dram_allocator->AllocPages<void*>(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("alloc addr", sub_context_stack_base);

  ExecutionContext* sub_context = CreateExecutionContext(
      SubTask, GDT::kUserCSSelector, sub_context_rsp, GDT::kUserDSSelector,
      reinterpret_cast<uint64_t>(CreatePageTable()));
  scheduler->RegisterExecutionContext(sub_context);

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
