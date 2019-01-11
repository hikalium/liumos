#include "liumos.h"
#include "cpu_context.h"
#include "execution_context.h"
#include "hpet.h"

HPET hpet;
ACPI_NFIT* nfit;
ACPI_MADT* madt;
ACPI_HPET* hpet_table;
ACPI_RSDT* rsdt;
GDT global_desc_table;

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
    if (!(keyid & KeyID::kMaskBreak) && !(keyid & KeyID::kMaskExtended)) {
      if (is_recording_enabled_) {
        if (buf_used_ >= kSizeOfBuffer)
          return;
        buf_[buf_used_++] = (uint8_t)keyid;
        buf_[buf_used_] = 0;
      }
      PutChar(keyid);
    }
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

void DetectTablesOnXSDT() {
  assert(rsdt);
  ACPI_XSDT* xsdt = rsdt->xsdt;
  const int num_of_xsdt_entries =
      (xsdt->length - ACPI_DESCRIPTION_HEADER_SIZE) >> 3;
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
  if (!hpet_table)
    Panic("HPET table not found");
}

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

  rsdt = static_cast<ACPI_RSDT*>(
      EFIGetConfigurationTableByUUID(&EFI_ACPITableGUID));

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

  DetectTablesOnXSDT();

  new (&local_apic) LocalAPIC();
  Disable8259PIC();

  uint64_t local_apic_id = local_apic.GetID();
  PutStringAndHex("LOCAL APIC ID", local_apic_id);

  InitIOAPIC(local_apic_id);

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

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
