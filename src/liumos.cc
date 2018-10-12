#include "liumos.h"
#include "hpet.h"

uint8_t* vram;
int xsize;
int ysize;
int pixels_per_scan_line;

GDTR gdtr;
IDTR idtr;

void InitEFI(EFISystemTable* system_table) {
  _system_table = system_table;
  _system_table->boot_services->SetWatchdogTimer(0, 0, 0, nullptr);
  _system_table->boot_services->LocateProtocol(
      &EFI_GraphicsOutputProtocolGUID, nullptr,
      (void**)&efi_graphics_output_protocol);
}

void InitGraphics() {
  vram = static_cast<uint8_t*>(
      efi_graphics_output_protocol->mode->frame_buffer_base);
  xsize = efi_graphics_output_protocol->mode->info->horizontal_resolution;
  ysize = efi_graphics_output_protocol->mode->info->vertical_resolution;
  pixels_per_scan_line =
      efi_graphics_output_protocol->mode->info->pixels_per_scan_line;
}

[[noreturn]] void Panic(const char* s) {
  PutString("!!!! PANIC !!!!\n");
  PutString(s);
  for (;;) {
  }
}

uint32_t* GetLocalAPICRegisterAddr(uint64_t offset) {
  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  return (
      uint32_t*)(((local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) &
                  ~0xfffULL) +
                 offset);
}

void SendEndOfInterruptToLocalAPIC() {
  *GetLocalAPICRegisterAddr(0xB0) = 0;
}

extern "C" {

void IntHandler(uint64_t intcode, InterruptInfo* info) {
  if (intcode == 0x20) {
    // PutString(".");
    SendEndOfInterruptToLocalAPIC();
    return;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS", info->cs);

  if (intcode == 0x03) {
    Panic("Int3 Trap");
  }
  Panic("INTHandler not implemented");
}
}

void PrintIDTGateDescriptor(IDTGateDescriptor* desc) {
  PutStringAndHex("desc.ofs", ((uint64_t)desc->offset_high << 32) |
                                  ((uint64_t)desc->offset_mid << 16) |
                                  desc->offset_low);
  PutStringAndHex("desc.segmt", desc->segment_descriptor);
  PutStringAndHex("desc.IST", desc->interrupt_stack_table);
  PutStringAndHex("desc.type", desc->type);
  PutStringAndHex("desc.DPL", desc->descriptor_privilege_level);
  PutStringAndHex("desc.present", desc->present);
}

uint8_t GetLocalAPICID(uint64_t local_apic_base_addr) {
  return *(uint32_t*)(local_apic_base_addr + 0x20) >> 24;
}

uint32_t ReadIOAPICRegister(uint64_t io_apic_base_addr, uint8_t reg_index) {
  *(uint32_t volatile*)(io_apic_base_addr) = (uint32_t)reg_index;
  return *(uint32_t volatile*)(io_apic_base_addr + 0x10);
}

void WriteIOAPICRegister(uint64_t io_apic_base_addr,
                         uint8_t reg_index,
                         uint32_t value) {
  *(uint32_t volatile*)(io_apic_base_addr) = (uint32_t)reg_index;
  *(uint32_t volatile*)(io_apic_base_addr + 0x10) = value;
}

uint64_t ReadIOAPICRedirectTableRegister(uint64_t io_apic_base_addr,
                                         uint8_t irq_index) {
  return (uint64_t)ReadIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2) |
         ((uint64_t)ReadIOAPICRegister(io_apic_base_addr,
                                       0x10 + irq_index * 2 + 1)
          << 32);
}

void WriteIOAPICRedirectTableRegister(uint64_t io_apic_base_addr,
                                      uint8_t irq_index,
                                      uint64_t value) {
  WriteIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2, (uint32_t)value);
  WriteIOAPICRegister(io_apic_base_addr, 0x10 + irq_index * 2 + 1,
                      (uint32_t)(value >> 32));
}

void SetIntHandler(int index,
                   uint8_t segm_desc,
                   uint8_t ist,
                   IDTType type,
                   uint8_t dpl,
                   void (*handler)()) {
  IDTGateDescriptor* desc = &idtr.base[index];
  desc->segment_descriptor = segm_desc;
  desc->interrupt_stack_table = ist;
  desc->type = static_cast<int>(type);
  desc->descriptor_privilege_level = dpl;
  desc->present = 1;
  desc->offset_low = (uint64_t)handler & 0xffff;
  desc->offset_mid = ((uint64_t)handler >> 16) & 0xffff;
  desc->offset_high = ((uint64_t)handler >> 32) & 0xffffffff;
  desc->reserved0 = 0;
  desc->reserved1 = 0;
  desc->reserved2 = 0;
}

void InitGDT() {
  ReadGDTR(&gdtr);
}

void InitIDT() {
  ReadIDTR(&idtr);

  ClearIntFlag();

  uint16_t cs = ReadCSSelector();
  SetIntHandler(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetIntHandler(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetIntHandler(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  WriteIDTR(&idtr);

  StoreIntFlag();
}

void InitIOAPIC(uint64_t local_apic_id) {
  uint64_t redirect_table =
      ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2);
  redirect_table &= 0x00fffffffffe0000UL;
  redirect_table |= (local_apic_id << 56) | 0x20;
  WriteIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, 2, redirect_table);
}

HPET hpet;

void __assert(const char* expr_str, const char* file, int line) {
  PutString("Assertion failed: ");
  PutString(expr_str);
  PutString(" at ");
  PutString(file);
  PutString(":");
  PutString("\n");
  Panic("halt...");
}

#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))

inline void* operator new(size_t, void* where) {
  return where;
}

class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_(nullptr) {}
  void FreePages(void* phys_addr, uint64_t num_of_pages);
  void* AllocPages(int num_of_pages);
  void Print();

 private:
  class FreeInfo {
   public:
    FreeInfo(uint64_t num_of_pages, FreeInfo* next)
        : num_of_pages_(num_of_pages), next_(next) {}
    FreeInfo* GetNext() const { return next_; }
    void* ProvidePages(int num_of_req_pages) {
      if (!CanProvidePages(num_of_req_pages))
        return nullptr;
      num_of_pages_ -= num_of_req_pages;
      return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this) +
                                     (num_of_pages_ << 12));
    }

    void Print() {
      uint64_t physical_start = reinterpret_cast<uint64_t>(this);
      PutString("[ 0x");
      PutHex64ZeroFilled(physical_start);
      PutString(" - 0x");
      PutHex64ZeroFilled(physical_start + (num_of_pages_ << 12));
      PutString(" ) = 0x");
      PutHex64(num_of_pages_);
      PutString(" pages\n");
    }

   private:
    bool CanProvidePages(int num_of_req_pages) const {
      return num_of_req_pages < num_of_pages_;
    }
    uint64_t num_of_pages_;
    FreeInfo* next_;
  };
  FreeInfo* head_;
};

void PhysicalPageAllocator::Print() {
  FreeInfo* info = head_;
  PutString("Free lists:\n");
  while (info) {
    info->Print();
    info = info->GetNext();
  }
  PutString("Free lists end\n");
}

void PhysicalPageAllocator::FreePages(void* phys_addr, uint64_t num_of_pages) {
  assert(phys_addr != nullptr);
  assert(num_of_pages > 0);
  assert((reinterpret_cast<uint64_t>(phys_addr) & 0xfff) == 0);

  FreeInfo* info = reinterpret_cast<FreeInfo*>(phys_addr);
  head_ = new (info) FreeInfo(num_of_pages, head_);
}

void* PhysicalPageAllocator::AllocPages(int num_of_pages) {
  FreeInfo* info = head_;
  void* addr = nullptr;
  while (info) {
    addr = info->ProvidePages(num_of_pages);
    if (addr)
      return addr;
  }
  Panic("Cannot allocate pages");
}

void InitMemoryManagement(EFIMemoryMap& map, PhysicalPageAllocator& allocator) {
  PutStringAndHex("Map entries", map.GetNumberOfEntries());
  int available_pages = 0;
  for (int i = 0; i < map.GetNumberOfEntries(); i++) {
    const EFIMemoryDescriptor* desc = map.GetDescriptor(i);
    if (desc->type != EFIMemoryType::kConventionalMemory)
      continue;
    desc->Print();
    PutString("\n");
    available_pages += desc->number_of_pages;
    allocator.FreePages(reinterpret_cast<void*>(desc->physical_start),
                        desc->number_of_pages);
  }
  PutStringAndHex("Available memory (KiB)", available_pages * 4);
}

void SubTask() {
  while (1) {
    StoreIntFlagAndHalt();
    PutString("BootProcessor\n");
  }
}

void MainForBootProcessor(void* image_handle, EFISystemTable* system_table) {
  EFIMemoryMap memory_map;
  PhysicalPageAllocator page_allocator;

  InitEFI(system_table);
  EFIClearScreen();
  InitGraphics();
  EnableVideoModeForConsole();
  EFIGetMemoryMapAndExitBootServices(image_handle, memory_map);

  PutString("liumOS is booting...\n");

  InitGDT();
  InitIDT();

  ACPI_RSDP* rsdp = static_cast<ACPI_RSDP*>(
      EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID));
  ACPI_XSDT* xsdt = rsdp->xsdt;

  int num_of_xsdt_entries = (xsdt->length - ACPI_DESCRIPTION_HEADER_SIZE) >> 3;

  CPUID cpuid;
  ReadCPUID(&cpuid, 0, 0);
  PutStringAndHex("Max CPUID", cpuid.eax);

  DrawRect(300, 100, 200, 200, 0xffffff);
  DrawRect(350, 150, 200, 200, 0xff0000);
  DrawRect(400, 200, 200, 200, 0x00ff00);
  DrawRect(450, 250, 200, 200, 0x0000ff);

  ReadCPUID(&cpuid, 1, 0);
  if (!(cpuid.edx & CPUID_01_EDX_APIC))
    Panic("APIC not supported");
  if (!(cpuid.edx & CPUID_01_EDX_MSR))
    Panic("MSR not supported");

  ACPI_NFIT* nfit = nullptr;
  ACPI_HPET* hpet_table = nullptr;
  ACPI_MADT* madt = nullptr;

  for (int i = 0; i < num_of_xsdt_entries; i++) {
    const char* signature = static_cast<const char*>(xsdt->entry[i]);
    if (IsEqualStringWithSize(signature, "NFIT", 4))
      nfit = static_cast<ACPI_NFIT*>(xsdt->entry[i]);
    if (IsEqualStringWithSize(signature, "HPET", 4))
      hpet_table = static_cast<ACPI_HPET*>(xsdt->entry[i]);
    if (IsEqualStringWithSize(signature, "APIC", 4))
      madt = static_cast<ACPI_MADT*>(xsdt->entry[i]);
  }

  if (!madt)
    Panic("MADT not found");

  for (int i = 0; i < madt->length - offsetof(ACPI_MADT, entries);
       i += madt->entries[i + 1]) {
    uint8_t type = madt->entries[i];
    if (type == kProcessorLocalAPICInfo) {
      PutString("Processor 0x");
      PutHex64(madt->entries[i + 2]);
      PutString(" local_apic_id = 0x");
      PutHex64(madt->entries[i + 3]);
      PutString(" flags = 0x");
      PutHex64(madt->entries[i + 4]);
      PutString("\n");
    } else if (type == kInterruptSourceOverrideInfo) {
      PutString("IRQ override: 0x");
      PutHex64(madt->entries[i + 3]);
      PutString(" -> 0x");
      PutHex64(*(uint32_t*)&madt->entries[i + 4]);
      PutString("\n");
    }
  }

  Disable8259PIC();

  uint64_t local_apic_base_msr = ReadMSR(0x1b);
  uint64_t local_apic_base_addr =
      (local_apic_base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) & ~0xfffULL;
  uint64_t local_apic_id = GetLocalAPICID(local_apic_base_addr);
  PutStringAndHex("LOCAL APIC ID", local_apic_id);

  InitIOAPIC(local_apic_id);

  if (!hpet_table)
    Panic("HPET table not found");

  hpet.Init(
      static_cast<HPET::RegisterSpace*>(hpet_table->base_address.address));

  hpet.SetTimerMs(
      0, 100, HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);

  new (&page_allocator) PhysicalPageAllocator();
  InitMemoryManagement(memory_map, page_allocator);
  page_allocator.Print();
  void* addr = page_allocator.AllocPages(3);
  PutStringAndHex("addr", reinterpret_cast<uint64_t>(addr));
  page_allocator.Print();

  Int03();

  while (1) {
    StoreIntFlagAndHalt();
    PutString("BootProcessor\n");
  }

  if (nfit) {
    PutString("NFIT found");
    PutStringAndHex("NFIT Size", nfit->length);
    PutStringAndHex("First NFIT Structure Type", nfit->entry[0]);
    PutStringAndHex("First NFIT Structure Size", nfit->entry[1]);
    if (static_cast<ACPI_NFITStructureType>(nfit->entry[0]) ==
        ACPI_NFITStructureType::kSystemPhysicalAddressRangeStructure) {
      ACPI_NFIT_SPARange* spa_range = (ACPI_NFIT_SPARange*)&nfit->entry[0];
      PutStringAndHex("SPARange Base",
                      spa_range->system_physical_address_range_base);
      PutStringAndHex("SPARange Length",
                      spa_range->system_physical_address_range_length);
      PutStringAndHex("SPARange Attribute",
                      spa_range->address_range_memory_mapping_attribute);
      PutStringAndHex("SPARange TypeGUID[0]",
                      spa_range->address_range_type_guid[0]);
      PutStringAndHex("SPARange TypeGUID[1]",
                      spa_range->address_range_type_guid[1]);
      uint64_t* p = (uint64_t*)spa_range->system_physical_address_range_base;
      PutStringAndHex("PMEM Previous value: ", *p);
      *p = *p + 3;
      PutStringAndHex("PMEM value after write: ", *p);
    }
  }

  while (1) {
    wchar_t c = EFIGetChar();
    EFIPutChar(c);
  };
}
