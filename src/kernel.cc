#include <immintrin.h>
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "corefunc.h"
#include "kernel.h"
#include "liumos.h"
#include "panic_printer.h"
#include "pci.h"
#include "ps2_mouse.h"
#include "rtl81xx.h"
#include "virtio_net.h"
#include "xhci.h"

// TODO(hikalium): Consider moving those vars into KernelEntry
// since the initializers will be called during the kernel initialization
LiumOS* liumos;
GDT gdt_;
KeyboardController keyboard_ctrl_;
LiumOS liumos_;
Sheet virtual_vram_;
Sheet virtual_screen_;
LocalAPIC bsp_local_apic_;
CPUFeatureSet cpu_features_;
SerialPort com1_;
SerialPort com2_;
LoaderInfo* loader_info_;

uint64_t GetKernelStraightMappingBase() {
  return liumos->cpu_features->kernel_phys_page_map_begin;
}

void kprintf(const char* fmt, ...) {
  constexpr int kSizeOfBuffer = 4096;
  static char buf[kSizeOfBuffer];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  if (len < 0 || kSizeOfBuffer <= len) {
    PutStringAndHex("kprintf: warning: vsnprintf returned", len);
    buf[kSizeOfBuffer - 1] = 0;
  }
  liumos->main_console->PutString(buf);
  va_end(args);
}

void kprintbuf(const char* desc,
               const volatile void* data,
               size_t start,
               size_t end) {
  kprintf("%s [ +%llu - +%llu ):\n", desc, start, end);
  int cnt = 0;
  for (size_t i = start; i < end; i++) {
    kprintf("%02X%c", reinterpret_cast<const volatile uint8_t*>(data)[i],
            (cnt & 0xF) == 0xF ? '\n' : ' ');
    cnt++;
  }
  kprintf("\n");
}

void InitPMEMManagement() {
  using namespace ACPI;
  if (!liumos->acpi.nfit) {
    PutString("NFIT not found. There are no PMEMs on this system.\n");
    return;
  }
  NFIT& nfit = *liumos->acpi.nfit;
  uint64_t available_pmem_size = 0;

  int pmem_manager_used = 0;

  for (auto& it : nfit) {
    if (it.type != NFIT::Entry::kTypeSPARangeStructure)
      continue;
    NFIT::SPARange* spa_range = reinterpret_cast<NFIT::SPARange*>(&it);
    if (!IsEqualGUID(
            reinterpret_cast<GUID*>(&spa_range->address_range_type_guid),
            &NFIT::SPARange::kByteAddressablePersistentMemory))
      continue;
    PutStringAndHex("SPARange #", spa_range->spa_range_structure_index);
    PutStringAndHex("  Base", spa_range->system_physical_address_range_base);
    PutStringAndHex("  Length",
                    spa_range->system_physical_address_range_length);
    available_pmem_size += spa_range->system_physical_address_range_length;
    assert(pmem_manager_used < LiumOS::kNumOfPMEMManagers);
    liumos->pmem[pmem_manager_used++] =
        reinterpret_cast<PersistentMemoryManager*>(
            spa_range->system_physical_address_range_base);
  }
  PutStringAndHex("Available PMEM (KiB)", available_pmem_size >> 10);
}

void InitializeVRAMForKernel() {
  constexpr uint64_t kernel_virtual_vram_base = 0xFFFF'FFFF'8000'0000ULL;
  const int xsize = liumos->vram_sheet->GetXSize();
  const int ysize = liumos->vram_sheet->GetYSize();
  const int ppsl = liumos->vram_sheet->GetPixelsPerScanLine();
  CreatePageMapping(
      GetSystemDRAMAllocator(), GetKernelPML4(), kernel_virtual_vram_base,
      reinterpret_cast<uint64_t>(liumos->vram_sheet->GetBuf()),
      liumos->vram_sheet->GetBufSize(), kPageAttrPresent | kPageAttrWritable);
  virtual_vram_.Init(reinterpret_cast<uint32_t*>(kernel_virtual_vram_base),
                     xsize, ysize, ppsl);
  liumos->vram_sheet = &virtual_vram_;
  virtual_vram_.SetMap(AllocKernelMemory<Sheet**>(
      virtual_vram_.GetXSize() * virtual_vram_.GetYSize() * sizeof(Sheet*)));

  constexpr uint64_t kernel_virtual_screen_base = 0xFFFF'FFFF'8800'0000ULL;
  CreatePageMapping(
      GetSystemDRAMAllocator(), GetKernelPML4(), kernel_virtual_screen_base,
      reinterpret_cast<uint64_t>(liumos->screen_sheet->GetBuf()),
      liumos->screen_sheet->GetBufSize(), kPageAttrPresent | kPageAttrWritable);
  virtual_screen_.Init(reinterpret_cast<uint32_t*>(kernel_virtual_screen_base),
                       xsize, ysize, ppsl);
  virtual_screen_.SetParent(&virtual_vram_);

  assert(liumos->screen_sheet->GetBufSize() == virtual_screen_.GetBufSize());
  memcpy(virtual_screen_.GetBuf(), liumos->screen_sheet->GetBuf(),
         virtual_screen_.GetBufSize());
  liumos->screen_sheet = &virtual_screen_;
  liumos->screen_sheet->SetLocked(true);
}

void CreateAndLaunchKernelTask(void (*entry_point)(), const char* task_name) {
  const int kNumOfStackPages = 64;
  void* sub_context_stack_base =
      liumos->kernel_heap_allocator->AllocPages<void*>(kNumOfStackPages);
  kprintf("KernelTask stack: [%p - %p)\n", sub_context_stack_base,
          reinterpret_cast<uint8_t*>(sub_context_stack_base) +
              kNumOfStackPages * kPageSize);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      (kNumOfStackPages << kPageSizeExponent) - 8);
  kprintf("Initial RSP: %p\n", sub_context_rsp);
  // -8 here for alignment (which is usually used to store return pointer)

  ExecutionContext& sub_context =
      *liumos->kernel_heap_allocator->Alloc<ExecutionContext>();
  sub_context.SetRegisters(entry_point, GDT::kKernelCSSelector, sub_context_rsp,
                           GDT::kKernelDSSelector, ReadCR3(),
                           kRFlagsInterruptEnable, 0);

  Process& proc = liumos->proc_ctrl->Create(task_name);
  proc.InitAsEphemeralProcess(sub_context);
  liumos->scheduler->RegisterProcess(proc);
}

static void EnsureAddrIs16ByteAligned(Process& from,
                                      Process& to,
                                      const char* label,
                                      void* addr) {
  if ((reinterpret_cast<uint64_t>(addr) & 0xF) == 0)
    return;
  PutStringAndHex(label, addr);
  PutStringAndHex("From Process#", from.GetID());
  PutStringAndHex("To   Process#", to.GetID());
  Panic("Not 16-byte aligned\n");
}

void SwitchContext(InterruptInfo& int_info,
                   Process& from_proc,
                   Process& to_proc) {
  static uint64_t proc_last_time_count = 0;
  const uint64_t now_count = HPET::GetInstance().ReadMainCounterValue();
  if ((proc_last_time_count - now_count) < liumos->time_slice_count)
    return;

  from_proc.AddProcTimeFemtoSec(
      (HPET::GetInstance().ReadMainCounterValue() - proc_last_time_count) *
      HPET::GetInstance().GetFemtosecondPerCount());

  CPUContext& from = from_proc.GetExecutionContext().GetCPUContext();
  const uint64_t t0 = HPET::GetInstance().ReadMainCounterValue();

  EnsureAddrIs16ByteAligned(from_proc, to_proc, "int_info.fpu_context",
                            &int_info.fpu_context);

  from.cr3 = ReadCR3();
  from.greg = int_info.greg;
  from.int_ctx = int_info.int_ctx;
  from_proc.NotifyContextSaving();
  const uint64_t t1 = HPET::GetInstance().ReadMainCounterValue();
  from_proc.AddTimeConsumedInContextSavingFemtoSec(
      (t1 - t0) * HPET::GetInstance().GetFemtosecondPerCount());

  CPUContext& to = to_proc.GetExecutionContext().GetCPUContext();
  int_info.greg = to.greg;
  int_info.int_ctx = to.int_ctx;
  if (from.cr3 == to.cr3)
    return;
  WriteCR3(to.cr3);
  // TODO: Investigate why this line causes #GP on pi.bin
  // maybe ReadMainCounterValue accesses physical addr with
  // user pagetable?
  // proc_last_time_count = HPET::GetInstance().ReadMainCounterValue();
}

__attribute__((ms_abi)) extern "C" void SleepHandler(uint64_t,
                                                     InterruptInfo* info) {
  Process& proc = liumos->scheduler->GetCurrentProcess();
  Process* next_proc = liumos->scheduler->SwitchProcess();
  if (!next_proc)
    return;  // no need to switching context.
  assert(info);
  SwitchContext(*info, proc, *next_proc);
}

void TimerHandler(uint64_t, InterruptInfo* info) {
  liumos->bsp_local_apic->SendEndOfInterrupt();
  SleepHandler(0, info);
}

void CoreFunc::PutChar(char c) {
  liumos->main_console->PutChar(c);
}
void CoreFunc::PutString(const char* s) {
  liumos->main_console->PutString(s);
}

EFI& CoreFunc::GetEFI() {
  assert(loader_info_);
  assert(loader_info_->efi);
  return *loader_info_->efi;
}

LoaderInfo& GetLoaderInfo() {
  assert(loader_info_);
  return *loader_info_;
}

KernelPhysPageAllocator& GetKernelPhysPageAllocator() {
  return *reinterpret_cast<KernelPhysPageAllocator*>(
      reinterpret_cast<uint64_t>(&GetSystemDRAMAllocator()) +
      GetKernelStraightMappingBase());
}

void SubTask();     // @subtask.cc
void USBManager();  // @usb_manager.cc

extern "C" void KernelEntry(LiumOS* liumos_passed, LoaderInfo& loader_info) {
  // These local vars will not being freed until the OS stops.
  Console virtual_console_;

  loader_info_ = &loader_info;
  liumos_ = *liumos_passed;
  liumos = &liumos_;

  auto& kernel_phys_page_allocator = GetKernelPhysPageAllocator();
  InitPMEMManagement();

  KernelVirtualHeapAllocator kernel_heap_allocator(GetKernelPML4(),
                                                   kernel_phys_page_allocator);
  liumos->kernel_heap_allocator = &kernel_heap_allocator;

  Disable8259PIC();
  bsp_local_apic_.Init();

  InitIOAPIC(bsp_local_apic_.GetID());

  HPET& hpet = HPET::GetInstance();
  hpet.Init(static_cast<HPET::RegisterSpace*>(
      liumos->acpi.hpet->base_address.address));
  hpet.SetTimerNs(
      0, 1000,
      HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);
  liumos->time_slice_count = 1e12 * 100 / hpet.GetFemtosecondPerCount();

  cpu_features_ = *liumos->cpu_features;
  liumos->cpu_features = &cpu_features_;

  InitializeVRAMForKernel();

  new (&virtual_console_) Console();
  virtual_console_.SetSheet(liumos->screen_sheet);
  PutStringAndHex("screen_sheet", liumos->screen_sheet);
  auto [cursor_x, cursor_y] = liumos->main_console->GetCursorPosition();
  virtual_console_.SetCursorPosition(cursor_x, cursor_y);
  liumos->main_console = &virtual_console_;

  com1_.Init(kPortCOM1);
  com2_.Init(kPortCOM2);

  liumos->main_console->SetSerial(&com2_);

  PanicPrinter::Init(liumos->kernel_heap_allocator->Alloc<PanicPrinter>(),
                     virtual_vram_, com2_);

  bsp_local_apic_.Init();

  ProcessController proc_ctrl_(kernel_heap_allocator);
  liumos->proc_ctrl = &proc_ctrl_;

  ExecutionContext& root_context =
      *liumos->kernel_heap_allocator->Alloc<ExecutionContext>();
  root_context.SetRegisters(nullptr, 0, nullptr, 0, ReadCR3(), 0, 0);
  ProcessMappingInfo& map_info = root_context.GetProcessMappingInfo();
  constexpr uint64_t kNumOfKernelHeapPages = 4;
  uint64_t kernel_heap_virtual_base = 0xFFFF'FFFF'5000'0000ULL;
  map_info.heap.Set(
      kernel_heap_virtual_base,
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelHeapPages),
      kNumOfKernelHeapPages << kPageSizeExponent);
  liumos->root_process = &liumos->proc_ctrl->Create("root process");
  liumos->root_process->InitAsEphemeralProcess(root_context);
  Scheduler scheduler_(*liumos->root_process);
  liumos->scheduler = &scheduler_;
  liumos->is_multi_task_enabled = true;

  int idx = GetLoaderInfo().FindFile("LIUMOS.ELF");
  if (idx == -1) {
    PutString("file not found.");
    return;
  }
  EFIFile& liumos_elf = GetLoaderInfo().root_files[idx];

  const Elf64_Shdr* sh_ctor = FindSectionHeader(liumos_elf, ".ctors");
  if (sh_ctor) {
    void (*const* ctors)() =
        reinterpret_cast<void (*const*)()>(sh_ctor->sh_addr);
    auto num = sh_ctor->sh_size / sizeof(ctors);
    for (decltype(num) i = 0; i < num; i++) {
      ctors[i]();
    }
  }

  const Elf64_Shdr* sh_init_array =
      FindSectionHeader(liumos_elf, ".init_array");
  if (sh_init_array) {
    void (*const* ctors)() =
        reinterpret_cast<void (*const*)()>(sh_init_array->sh_addr);
    auto num = sh_init_array->sh_size / sizeof(ctors);
    for (decltype(num) i = 0; i < num; i++) {
      ctors[i]();
    }
  }

  // At this point, kprintf is available.
  kprintf("Hello from kernel!\n");
  ConsoleCommand::Version();

  ClearIntFlag();

  kprintf("SystemDRAMAllocator is at: %p\n", &GetSystemDRAMAllocator());
  kprintf("KernelPhysPageAllocator is at: %p\n", &kernel_phys_page_allocator);
  kprintf("KernelPhysPageAllocator alloc 1: %p\n",
          kernel_phys_page_allocator.AllocPages<void*>(1));

  constexpr uint64_t kNumOfKernelStackPages = 128;
  uint64_t kernel_stack_physical_base =
      GetSystemDRAMAllocator().AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t kernel_stack_virtual_base = 0xFFFF'FFFF'2000'0000ULL;
  CreatePageMapping(GetSystemDRAMAllocator(), GetKernelPML4(),
                    kernel_stack_virtual_base, kernel_stack_physical_base,
                    kNumOfKernelStackPages << kPageSizeExponent,
                    kPageAttrPresent | kPageAttrWritable);
  uint64_t kernel_stack_pointer =
      kernel_stack_virtual_base + (kNumOfKernelStackPages << kPageSizeExponent);

  uint64_t ist1_virt_base =
      kernel_heap_allocator.AllocPages<uint64_t>(kNumOfKernelStackPages);

  gdt_.Init(kernel_stack_pointer,
            ist1_virt_base + (kNumOfKernelStackPages << kPageSizeExponent));
  IDT::Init();
  keyboard_ctrl_.Init();

  PS2MouseController& mouse_ctrl = PS2MouseController::GetInstance();
  mouse_ctrl.Init();

  IDT::GetInstance().SetIntHandler(0x20, TimerHandler);

  PCI& pci = PCI::GetInstance();
  pci.DetectDevices();

  // CreateAndLaunchKernelTask(SubTask);
  CreateAndLaunchKernelTask(NetworkManager, "network manager");
  CreateAndLaunchKernelTask(MouseManager, "mouse manager");
  // CreateAndLaunchKernelTask(USBManager);

  EnableSyscall();
  Virtio::Net::GetInstance().Init();
  // RTL81::GetInstance().Init();

  StoreIntFlag();

  TextBox console_text_box;
  while (1) {
    ConsoleCommand::WaitAndProcess(console_text_box);
  }
}
