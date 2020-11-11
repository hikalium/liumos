#include "corefunc.h"
#include "efi_file_manager.h"
#include "execution_context.h"
#include "hpet.h"
#include "liumos.h"
#include "loader_info.h"
#include "panic_printer.h"
#include "util.h"

LiumOS* liumos;
EFI::MemoryMap efi_memory_map;
PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>* pmem_allocator;
CPUFeatureSet cpu_features;
SerialPort com2;

LiumOS liumos_;
Console main_console_;
EFI efi_;

uint8_t panic_printer_work[sizeof(PanicPrinter)];

LoaderInfo* loader_info_;
LoaderInfo& GetLoaderInfo() {
  assert(loader_info_);
  return *loader_info_;
}

uint64_t GetKernelStraightMappingBase() {
  Panic("GetKernelStraightMappingBase should not be called in loader");
}

void FreePages(
    PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>* allocator,
    uint64_t phys_addr,
    uint64_t num_of_pages) {
  const uint32_t prox_domain =
      liumos->acpi.srat ? liumos->acpi.srat->GetProximityDomainForAddrRange(
                              phys_addr, num_of_pages << kPageSizeExponent)
                        : 0;
  allocator->FreePagesWithProximityDomain(phys_addr, num_of_pages, prox_domain);
}

PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy> dram_allocator_;
void InitDRAMManagement(EFI::MemoryMap& map) {
  auto dram_allocator = &dram_allocator_;
  new (dram_allocator)
      PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>();
  int available_pages = 0;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFI::MemoryDescriptor* desc = map.GetDescriptor(i);
    if (desc->type != EFI::MemoryType::kConventionalMemory)
      continue;
    available_pages += desc->number_of_pages;
    FreePages(dram_allocator, desc->physical_start, desc->number_of_pages);
  }
  PutStringAndHex("Available DRAM (KiB)", available_pages * 4);
  GetLoaderInfo().dram_allocator = dram_allocator;
}

void IdentifyCPU() {
  CPUID cpuid;
  CPUFeatureSet& f = cpu_features;
  f.features = 0;

  ReadCPUID(&cpuid, 0, 0);
  f.max_cpuid = cpuid.eax;

  ReadCPUID(&cpuid, 0x8000'0000, 0);
  f.max_extended_cpuid = cpuid.eax;

  ReadCPUID(&cpuid, 1, 0);
  f.family = ((cpuid.eax >> 16) & 0xff0) | ((cpuid.eax >> 8) & 0xf);
  f.model = ((cpuid.eax >> 12) & 0xf0) | ((cpuid.eax >> 4) & 0xf);
  f.stepping = cpuid.eax & 0xf;
  f.x2apic = cpuid.ecx & kCPUID01H_ECXBitx2APIC;
  f.clfsh = cpuid.edx & (1 << 19);
  f.features |= ((cpuid.ecx >> 21) & 1) << CPUFeatureIndex::kX2APIC;
  f.features |= ((cpuid.ecx >> 26) & 1) << CPUFeatureIndex::kXSAVE;
  f.features |= ((cpuid.ecx >> 27) & 1) << CPUFeatureIndex::kOSXSAVE;
  f.features |= ((cpuid.edx >> 9) & 1) << CPUFeatureIndex::kAPIC;
  f.features |= ((cpuid.edx >> 24) & 1) << CPUFeatureIndex::kFXSR;
  if (!(cpuid.edx & kCPUID01H_EDXBitAPIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & kCPUID01H_EDXBitMSR))
    Panic("MSR not supported");
  if (!GetBit<CPUFeatureIndex::kFXSR>(f.features))
    Panic("FXSR not supported");

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
  LoaderInfo loader_info;
  loader_info_ = &loader_info;
  liumos = &liumos_;
  efi_.Init(image_handle, system_table);
  loader_info.efi = &efi_;
  efi_.ClearScreen();

  InitGraphics();
  main_console_.SetSheet(liumos->screen_sheet);
  liumos->main_console = &main_console_;

  loader_info.root_files_used =
      efi_.LoadRootFiles(loader_info.root_files, kNumOfRootFiles);

  efi_.GetMemoryMapAndExitBootServices(image_handle, efi_memory_map);
  liumos->efi_memory_map = &efi_memory_map;

  com2.Init(kPortCOM2);
  main_console_.SetSerial(&com2);

  PanicPrinter::Init(&panic_printer_work, *liumos->vram_sheet, com2);

  PutString("Loading liumOS...\n");
  PutString("\nliumOS version: ");
  PutString(GetVersionStr());
  PutString("\n\n");
  IdentifyCPU();
  ClearIntFlag();

  ACPI::DetectTables();

  InitDRAMManagement(efi_memory_map);

  InitDoubleBuffer();
  main_console_.SetSheet(liumos->screen_sheet);

  constexpr uint64_t kNumOfKernelStackPages = 256;
  uint64_t kernel_stack_base =
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t kernel_stack_pointer =
      kernel_stack_base + (kNumOfKernelStackPages << kPageSizeExponent);
  PutString("Kernel Stack: [ ");
  PutHex64ZeroFilled(kernel_stack_base);
  PutString(" - ");
  PutHex64ZeroFilled(kernel_stack_base + kNumOfKernelStackPages * kPageSize);
  PutString(" )\n");
  PutString("Initial RSP: ");
  PutHex64ZeroFilled(kernel_stack_pointer);
  PutString("\n");

  uint64_t ist1_base =
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t ist1_pointer =
      ist1_base + (kNumOfKernelStackPages << kPageSizeExponent);

  GDT gdt;
  gdt.Init(kernel_stack_pointer, ist1_pointer);
  IDT::Init();
  InitPaging();

  int idx = loader_info.FindFile("LIUMOSRS.ELF");
  if (idx == -1) {
    PutString("file not found.");
    return;
  }
  EFIFile& liumos_elf = loader_info.root_files[idx];

  LoadKernelELF(liumos_elf, loader_info);
}
