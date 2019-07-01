#include <stdio.h>
#include <algorithm>
#include <functional>
#include <vector>
#include "corefunc.h"
#include "liumos.h"

LiumOS* liumos;

GDT gdt_;
IDT idt_;
KeyboardController keyboard_ctrl_;
LiumOS liumos_;
Sheet virtual_vram_;
Sheet virtual_screen_;
Console virtual_console_;
LocalAPIC bsp_local_apic_;
CPUFeatureSet cpu_features_;
SerialPort com1_;
SerialPort com2_;
HPET hpet_;

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
            &NFIT::SPARange::kByteAdressablePersistentMemory))
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
      *liumos->dram_allocator, GetKernelPML4(), kernel_virtual_vram_base,
      reinterpret_cast<uint64_t>(liumos->vram_sheet->GetBuf()),
      liumos->vram_sheet->GetBufSize(), kPageAttrPresent | kPageAttrWritable);
  virtual_vram_.Init(reinterpret_cast<uint8_t*>(kernel_virtual_vram_base),
                     xsize, ysize, ppsl);

  constexpr uint64_t kernel_virtual_screen_base = 0xFFFF'FFFF'8800'0000ULL;
  CreatePageMapping(
      *liumos->dram_allocator, GetKernelPML4(), kernel_virtual_screen_base,
      reinterpret_cast<uint64_t>(liumos->screen_sheet->GetBuf()),
      liumos->screen_sheet->GetBufSize(), kPageAttrPresent | kPageAttrWritable);
  virtual_screen_.Init(reinterpret_cast<uint8_t*>(kernel_virtual_screen_base),
                       xsize, ysize, ppsl);
  virtual_screen_.SetParent(&virtual_vram_);

  liumos->screen_sheet = &virtual_screen_;
  liumos->screen_sheet->DrawRect(0, 0, xsize, ysize, 0xc6c6c6);
}

void SubTask();  // @subtask.cc

void LaunchSubTask(KernelVirtualHeapAllocator& kernel_heap_allocator) {
  const int kNumOfStackPages = 3;
  void* sub_context_stack_base = kernel_heap_allocator.AllocPages<void*>(
      kNumOfStackPages, kPageAttrPresent | kPageAttrWritable);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      (kNumOfStackPages << kPageSizeExponent));
  PutStringAndHex("SubTask stack base", sub_context_stack_base);

  ExecutionContext& sub_context =
      *liumos->kernel_heap_allocator->Alloc<ExecutionContext>();
  sub_context.SetRegisters(
      SubTask, GDT::kKernelCSSelector, sub_context_rsp, GDT::kKernelDSSelector,
      reinterpret_cast<uint64_t>(&GetKernelPML4()), kRFlagsInterruptEnable, 0);

  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsEphemeralProcess(sub_context);
  liumos->scheduler->RegisterProcess(proc);
  liumos->sub_process = &proc;
}

void SwitchContext(InterruptInfo& int_info,
                   Process& from_proc,
                   Process& to_proc) {
  static uint64_t proc_last_time_count = 0;
  const uint64_t now_count = liumos->hpet->ReadMainCounterValue();
  if ((proc_last_time_count - now_count) < liumos->time_slice_count)
    return;

  from_proc.AddProcTimeFemtoSec(
      (liumos->hpet->ReadMainCounterValue() - proc_last_time_count) *
      liumos->hpet->GetFemtosecondPerCount());

  CPUContext& from = from_proc.GetExecutionContext().GetCPUContext();
  const uint64_t t0 = liumos->hpet->ReadMainCounterValue();
  from.cr3 = ReadCR3();
  from.greg = int_info.greg;
  from.int_ctx = int_info.int_ctx;
  from_proc.NotifyContextSaving();
  const uint64_t t1 = liumos->hpet->ReadMainCounterValue();
  from_proc.AddTimeConsumedInContextSavingFemtoSec(
      (t1 - t0) * liumos->hpet->GetFemtosecondPerCount());

  CPUContext& to = to_proc.GetExecutionContext().GetCPUContext();
  int_info.greg = to.greg;
  int_info.int_ctx = to.int_ctx;
  if (from.cr3 == to.cr3)
    return;
  WriteCR3(to.cr3);
  proc_last_time_count = liumos->hpet->ReadMainCounterValue();
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

EFI& CoreFunc::GetEFI() {
  assert(liumos->loader_info.efi);
  return *liumos->loader_info.efi;
}

extern "C" void KernelEntry(LiumOS* liumos_passed) {
  liumos_ = *liumos_passed;
  liumos = &liumos_;

  InitPMEMManagement();

  KernelVirtualHeapAllocator kernel_heap_allocator(GetKernelPML4(),
                                                   *liumos->dram_allocator);
  liumos->kernel_heap_allocator = &kernel_heap_allocator;

  Disable8259PIC();
  bsp_local_apic_.Init();

  InitIOAPIC(bsp_local_apic_.GetID());

  hpet_.Init(static_cast<HPET::RegisterSpace*>(
      liumos->acpi.hpet->base_address.address));
  liumos->hpet = &hpet_;
  hpet_.SetTimerNs(
      0, 1000,
      HPET::TimerConfig::kUsePeriodicMode | HPET::TimerConfig::kEnable);
  liumos->time_slice_count = 1e12 * 100 / hpet_.GetFemtosecondPerCount();

  cpu_features_ = *liumos->cpu_features;
  liumos->cpu_features = &cpu_features_;

  InitializeVRAMForKernel();

  new (&virtual_console_) Console();
  virtual_console_.SetSheet(liumos->screen_sheet);
  liumos->main_console = &virtual_console_;

  com1_.Init(kPortCOM1);
  com2_.Init(kPortCOM2);

  liumos->main_console->SetSerial(&com2_);

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
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelHeapPages),
      kNumOfKernelHeapPages << kPageSizeExponent);
  liumos->root_process = &liumos->proc_ctrl->Create();
  liumos->root_process->InitAsEphemeralProcess(root_context);
  Scheduler scheduler_(*liumos->root_process);
  liumos->scheduler = &scheduler_;

  PutString("Hello from kernel!\n");
  PutString("\nliumOS version: ");
  PutString(GetVersionStr());
  PutString("\n\n");

  char s[128];
  snprintf(s, sizeof(s), "123 = 0x%X\n", 123);
  PutString(s);

  int v = 123;
  std::function<void(void)> f = [&]() {
    snprintf(s, sizeof(s), "Hello from std::function!\nv = %d\n", v);
    PutString(s);
  };
  v = 456;
  f();

  std::vector<int> vec;
  snprintf(s, sizeof(s), "capacity = 0x%lX\n", vec.capacity());
  PutString(s);
  vec.push_back(3);
  vec.push_back(1);
  vec.push_back(4);
  vec.push_back(1);
  vec.push_back(5);
  vec.push_back(9);
  vec.push_back(2);
  vec.push_back(6);
  vec.push_back(5);
  for (auto& v : vec) {
    snprintf(s, sizeof(s), "%d ", v);
    PutString(s);
  }
  PutString("\n");
  snprintf(s, sizeof(s), "capacity = 0x%lX\n", vec.capacity());
  PutString(s);
  std::sort(vec.begin(), vec.end());
  PutString("sorted values:\n");
  for (auto& v : vec) {
    snprintf(s, sizeof(s), "value=%d, addr=%p\n", v, &v);
    PutString(s);
  }
  PutString("\n");

  ClearIntFlag();

  constexpr uint64_t kNumOfKernelStackPages = 128;
  uint64_t kernel_stack_physical_base =
      liumos->dram_allocator->AllocPages<uint64_t>(kNumOfKernelStackPages);
  uint64_t kernel_stack_virtual_base = 0xFFFF'FFFF'2000'0000ULL;
  CreatePageMapping(*liumos->dram_allocator, GetKernelPML4(),
                    kernel_stack_virtual_base, kernel_stack_physical_base,
                    kNumOfKernelStackPages << kPageSizeExponent,
                    kPageAttrPresent | kPageAttrWritable);
  uint64_t kernel_stack_pointer =
      kernel_stack_virtual_base + (kNumOfKernelStackPages << kPageSizeExponent);

  uint64_t ist1_virt_base = kernel_heap_allocator.AllocPages<uint64_t>(
      kNumOfKernelStackPages, kPageAttrPresent | kPageAttrWritable);

  gdt_.Init(kernel_stack_pointer,
            ist1_virt_base + (kNumOfKernelStackPages << kPageSizeExponent));
  idt_.Init();
  keyboard_ctrl_.Init();

  idt_.SetIntHandler(0x20, TimerHandler);

  StoreIntFlag();

  LaunchSubTask(kernel_heap_allocator);

  EnableSyscall();

  TextBox console_text_box;
  while (1) {
    ConsoleCommand::WaitAndProcess(console_text_box);
  }
}
