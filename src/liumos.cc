#include "liumos.h"
#include "hpet.h"

uint8_t* vram;
int xsize;
int ysize;
int pixels_per_scan_line;

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

void SendEndOfInterruptToLocalAPIC() {
  *(uint32_t*)(((ReadMSR(MSRIndex::kLocalAPICBase) &
                 ((1ULL << MAX_PHY_ADDR_BITS) - 1)) &
                ~0xfffULL) +
               0xB0) = 0;
}

packed_struct CPUContext {
  InterruptInfo int_info;
  // scratch registers
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t rax;
  uint64_t r8;
  uint64_t rdx;
  uint64_t rcx;
  // rest of general registers
  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
};

class ExecutionContext {
 public:
  enum class Status {
    kNotScheduled,
    kSleeping,
    kRunning,
  };
  ExecutionContext(uint64_t id,
                   void (*rip)(),
                   uint16_t cs,
                   void* rsp,
                   uint16_t ss)
      : id_(id), status_(Status::kNotScheduled) {
    cpu_context_.int_info.rip = reinterpret_cast<uint64_t>(rip);
    cpu_context_.int_info.cs = cs;
    cpu_context_.int_info.rsp = reinterpret_cast<uint64_t>(rsp);
    cpu_context_.int_info.ss = ss;
    cpu_context_.int_info.eflags = 0x202;
  }
  uint64_t GetID() { return id_; };
  int GetSchedulerIndex() const { return scheduler_index_; };
  void SetSchedulerIndex(int scheduler_index) {
    scheduler_index_ = scheduler_index;
  };
  CPUContext* GetCPUContext() { return &cpu_context_; };
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; };

 private:
  uint64_t id_;
  int scheduler_index_;
  Status status_;
  CPUContext cpu_context_;
};

class Scheduler {
 public:
  Scheduler(ExecutionContext* root_context) : number_of_contexts_(0) {
    RegisterExecutionContext(root_context);
    WriteMSR(MSRIndex::kKernelGSBase, reinterpret_cast<uint64_t>(root_context));
    SwapGS();
    root_context->SetStatus(ExecutionContext::Status::kSleeping);
  }
  void RegisterExecutionContext(ExecutionContext* context) {
    assert(number_of_contexts_ < kNumberOfContexts);
    assert(context->GetStatus() == ExecutionContext::Status::kNotScheduled);
    contexts_[number_of_contexts_] = context;
    context->SetSchedulerIndex(number_of_contexts_);
    number_of_contexts_++;

    context->SetStatus(ExecutionContext::Status::kSleeping);
  }
  ExecutionContext* SwitchContext(ExecutionContext* current_context) {
    const int base_index = current_context->GetSchedulerIndex();
    for (int i = 1; i < number_of_contexts_; i++) {
      ExecutionContext* context =
          contexts_[(base_index + i) % number_of_contexts_];
      if (context->GetStatus() == ExecutionContext::Status::kSleeping) {
        current_context->SetStatus(ExecutionContext::Status::kSleeping);
        context->SetStatus(ExecutionContext::Status::kRunning);
        return context;
      }
    }
    return nullptr;
  }

 private:
  const static int kNumberOfContexts = 16;
  ExecutionContext* contexts_[kNumberOfContexts];
  int number_of_contexts_;
};

Scheduler* scheduler;

packed_struct ContextSwitchRequest {
  CPUContext* from;
  CPUContext* to;
};

ContextSwitchRequest context_switch_request;

extern "C" ContextSwitchRequest* IntHandler(uint64_t intcode,
                                            uint64_t error_code,
                                            InterruptInfo* info) {
  (void)error_code;
  if (intcode == 0x20) {
    SendEndOfInterruptToLocalAPIC();
    uint64_t kernel_gs_base = ReadMSR(MSRIndex::kKernelGSBase);
    ExecutionContext* current_context =
        reinterpret_cast<ExecutionContext*>(kernel_gs_base);
    ExecutionContext* next_context = scheduler->SwitchContext(current_context);
    if (!next_context) {
      // no need to switching context.
      return nullptr;
    }
    context_switch_request.from = current_context->GetCPUContext();
    context_switch_request.to = next_context->GetCPUContext();
    WriteMSR(MSRIndex::kKernelGSBase, reinterpret_cast<uint64_t>(next_context));
    return &context_switch_request;
  }
  if (intcode == 0x21) {
    SendEndOfInterruptToLocalAPIC();
    uint8_t data = ReadIOPort8(0x0060);
    PutStringAndHex("INT #0x21: ", data);
    return NULL;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS", info->cs);

  if (intcode == 0x03) {
    Panic("Int3 Trap");
  }
  Panic("INTHandler not implemented");
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

void InitIDT() {
  ReadIDTR(&idtr);

  uint16_t cs = ReadCSSelector();
  SetIntHandler(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetIntHandler(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetIntHandler(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  SetIntHandler(0x21, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler21);
  WriteIDTR(&idtr);
}

void SetInterruptRedirection(uint64_t local_apic_id,
                             int from_irq_num,
                             int to_vector_index) {
  uint64_t redirect_table =
      ReadIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, from_irq_num);
  redirect_table &= 0x00fffffffffe0000UL;
  redirect_table |= (local_apic_id << 56) | to_vector_index;
  WriteIOAPICRedirectTableRegister(IO_APIC_BASE_ADDR, from_irq_num,
                                   redirect_table);
}

void InitIOAPIC(uint64_t local_apic_id) {
  SetInterruptRedirection(local_apic_id, 2, 0x20);
  SetInterruptRedirection(local_apic_id, 1, 0x21);
}

HPET hpet;

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

class LocalAPIC {
 public:
  LocalAPIC() {
    uint64_t base_msr = ReadMSR(MSRIndex::kLocalAPICBase);
    base_addr_ = (base_msr & ((1ULL << MAX_PHY_ADDR_BITS) - 1)) & ~0xfffULL;
    id_ = *GetRegisterAddr(0x20) >> 24;
  }
  uint8_t GetID() { return id_; }

 private:
  uint32_t* GetRegisterAddr(uint64_t offset) {
    return (uint32_t*)(base_addr_ + offset);
  }

  uint64_t base_addr_;
  uint8_t id_;
};

void SubTask() {
  int count = 0;
  while (1) {
    StoreIntFlagAndHalt();
    PutStringAndHex("SubContext!", count++);
  }
}

GDT global_desc_table;

void MainForBootProcessor(void* image_handle, EFISystemTable* system_table) {
  LocalAPIC local_apic;
  EFIMemoryMap memory_map;
  PhysicalPageAllocator page_allocator;

  InitEFI(system_table);
  EFIClearScreen();
  InitGraphics();
  EnableVideoModeForConsole();
  EFIGetMemoryMapAndExitBootServices(image_handle, memory_map);

  PutString("liumOS is booting...\n");

  ClearIntFlag();

  new (&global_desc_table) GDT();
  InitIDT();

  ExecutionContext root_context(1, NULL, 0, NULL, 0);
  Scheduler scheduler_(&root_context);
  scheduler = &scheduler_;

  ACPI_RSDT* rsdt = static_cast<ACPI_RSDT*>(
      EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID));
  ACPI_XSDT* xsdt = rsdt->xsdt;

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

  new (&local_apic) LocalAPIC();

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

  for (int i = 0; i < (int)(madt->length - offsetof(ACPI_MADT, entries));
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

  uint64_t local_apic_id = local_apic.GetID();
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
  const int kNumOfStackPages = 3;
  void* sub_context_stack_base = page_allocator.AllocPages(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("alloc addr", sub_context_stack_base);

  ExecutionContext sub_context(2, SubTask, ReadCSSelector(), sub_context_rsp,
                               ReadSSSelector());
  // scheduler->RegisterExecutionContext(&sub_context);

  if (nfit) {
    PutString("NFIT found\n");
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
      PutStringAndHex("\nPointer in PMEM Region: ", p);
      PutStringAndHex("PMEM Previous value: ", *p);
      (*p)++;
      PutStringAndHex("PMEM value after write: ", *p);

      uint64_t* q = reinterpret_cast<uint64_t*>(page_allocator.AllocPages(1));
      PutStringAndHex("\nPointer in DRAM Region: ", q);
      PutStringAndHex("DRAM Previous value: ", *q);
      (*q)++;
      PutStringAndHex("DRAM value after write: ", *q);
    }
  }

  while (1) {
    StoreIntFlagAndHalt();
    ClearIntFlag();
    // PutStringAndHex("RootContext", count += 2);
  }
}
