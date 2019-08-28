#include "corefunc.h"
#include "efi_file_manager.h"
#include "execution_context.h"
#include "hpet.h"
#include "liumos.h"

LiumOS* liumos;
EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator* dram_allocator;
PhysicalPageAllocator* pmem_allocator;
CPUFeatureSet cpu_features;
SerialPort com1;
EFIFile hello_bin_file;
EFIFile pi_bin_file;
EFIFile liumos_elf_file;

LiumOS liumos_;
PhysicalPageAllocator dram_allocator_;
Console main_console_;
EFI efi_;

EFIFile logo_file;

void FreePages(PhysicalPageAllocator* allocator,
               void* phys_addr,
               uint64_t num_of_pages) {
  const uint32_t prox_domain =
      liumos->acpi.srat ? liumos->acpi.srat->GetProximityDomainForAddrRange(
                              reinterpret_cast<uint64_t>(phys_addr),
                              num_of_pages << kPageSizeExponent)
                        : 0;
  allocator->FreePagesWithProximityDomain(phys_addr, num_of_pages, prox_domain);
}

void InitDRAMManagement(EFI::MemoryMap& map) {
  dram_allocator = &dram_allocator_;
  new (dram_allocator) PhysicalPageAllocator();
  int available_pages = 0;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    if (desc->type != EFI::MemoryType::kConventionalMemory)
      continue;
    available_pages += desc->number_of_pages;
    FreePages(dram_allocator, reinterpret_cast<void*>(desc->physical_start),
              desc->number_of_pages);
  }
  PutStringAndHex("Available DRAM (KiB)", available_pages * 4);
}

void InitMemoryManagement(EFI::MemoryMap& map) {
  InitDRAMManagement(map);
  liumos->dram_allocator = &dram_allocator_;
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
        liumos->screen_sheet->DrawRect(
            liumos->screen_sheet->GetXSize() - width + x++, y + 128, 1, 1, rgb);
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
  CPUFeatureSet& f = cpu_features;

  ReadCPUID(&cpuid, 0, 0);
  f.max_cpuid = cpuid.eax;

  ReadCPUID(&cpuid, 0x8000'0000, 0);
  f.max_extended_cpuid = cpuid.eax;

  ReadCPUID(&cpuid, 1, 0);
  f.family = ((cpuid.eax >> 16) & 0xff0) | ((cpuid.eax >> 8) & 0xf);
  f.model = ((cpuid.eax >> 12) & 0xf0) | ((cpuid.eax >> 4) & 0xf);
  f.stepping = cpuid.eax & 0xf;
  if (!(cpuid.edx & kCPUID01H_EDXBitAPIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & kCPUID01H_EDXBitMSR))
    Panic("MSR not supported");
  f.x2apic = cpuid.ecx & kCPUID01H_ECXBitx2APIC;
  f.clfsh = cpuid.edx & (1 << 19);

  if (7 <= f.max_cpuid) {
    ReadCPUID(&cpuid, 7, 0);
    f.clflushopt = cpuid.ebx & (1 << 23);
  }

  if (0x8000'0004 <= f.max_extended_cpuid) {
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
  }

  if (CPUIDIndex::kMaxAddr <= f.max_extended_cpuid) {
    ReadCPUID(&cpuid, CPUIDIndex::kMaxAddr, 0);
    IA32_MaxPhyAddr maxaddr;
    maxaddr.data = cpuid.eax;
    f.max_phy_addr = maxaddr.bits.physical_address_bits;
  } else {
    PutString("CPUID function 80000008H not supported.\n");
    PutString("Assuming Physical address bits = 36\n");
    f.max_phy_addr = 36;
  }
  f.phy_addr_mask = (1ULL << f.max_phy_addr) - 1;
  f.kernel_phys_page_map_begin = ~((1ULL << (f.max_phy_addr - 1)) - 1);
  liumos->cpu_features = &f;
}

void CoreFunc::PutChar(char c) {
  main_console_.PutChar(c);
}

EFI& CoreFunc::GetEFI() {
  return efi_;
}

void Sleep() {
  // Fake
}

void MainForBootProcessor(EFI::Handle image_handle,
                          EFI::SystemTable* system_table) {
  liumos = &liumos_;
  efi_.Init(system_table);
  liumos_.loader_info.efi = &efi_;
  efi_.ClearScreen();

  InitGraphics();
  main_console_.SetSheet(liumos->screen_sheet);
  liumos->main_console = &main_console_;

  efi_.ListAllFiles();

  EFIFileManager::Load(logo_file, L"logo.ppm");
  liumos_.loader_info.files.hello_bin = &hello_bin_file;
  EFIFileManager::Load(pi_bin_file, L"pi.bin");
  liumos_.loader_info.files.pi_bin = &pi_bin_file;
  EFIFileManager::Load(liumos_elf_file, L"LIUMOS.ELF");
  liumos_.loader_info.files.liumos_elf = &liumos_elf_file;
  EFIFileManager::Load(hello_bin_file, L"hello.bin");
  liumos_.loader_info.files.hello_bin = &hello_bin_file;

  efi_.GetMemoryMapAndExitBootServices(image_handle, efi_memory_map);
  liumos->efi_memory_map = &efi_memory_map;

  com1.Init(kPortCOM1);
  main_console_.SetSerial(&com1);

  PrintLogoFile();
  PutString("\nliumOS version: ");
  PutString(GetVersionStr());
  PutString("\n\n");
  IdentifyCPU();
  ClearIntFlag();

  ACPI::DetectTables();

  InitMemoryManagement(efi_memory_map);

  InitDoubleBuffer();
  main_console_.SetSheet(liumos->screen_sheet);

  constexpr uint64_t kNumOfKernelStackPages = 2;
  uint64_t kernel_stack_base =
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t kernel_stack_pointer =
      kernel_stack_base + (kNumOfKernelStackPages << kPageSizeExponent);

  uint64_t ist1_base =
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t ist1_pointer =
      ist1_base + (kNumOfKernelStackPages << kPageSizeExponent);

  GDT gdt;
  gdt.Init(kernel_stack_pointer, ist1_pointer);
  IDT idt;
  idt.Init();
  InitPaging();

  LoadKernelELF(liumos_elf_file);
}
