#pragma once

#include "acpi.h"
#include "apic.h"
#include "asm.h"
#include "console.h"
#include "efi.h"
#include "efi_file.h"
#include "elf.h"
#include "gdt.h"
#include "generic.h"
#include "githash.h"
#include "guid.h"
#include "hpet.h"
#include "interrupt.h"
#include "keyboard.h"
#include "keyid.h"
#include "libfunc.h"
#include "loader_info.h"
#include "paging.h"
#include "phys_page_allocator.h"
#include "process.h"
#include "serial.h"
#include "sheet.h"
#include "sheet_painter.h"
#include "sys_constant.h"
#include "text_box.h"

constexpr uint64_t kLAPICRegisterAreaPhysBase = 0x0000'0000'FEE0'0000ULL;
constexpr uint64_t kLAPICRegisterAreaVirtBase = 0xFFFF'FFFF'FEE0'0000ULL;
constexpr uint64_t kLAPICRegisterAreaByteSize = 0x0000'0000'0010'0000ULL;

constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000ULL;

constexpr uint64_t kKernelStackPagesForEachProcess = 64;

// @command.cc
namespace ConsoleCommand {
void ShowNFIT(void);
void ShowMADT(void);
void ShowSRAT(void);
void ShowSLIT(void);
void ShowEFIMemoryMap(void);
void Free(void);
void Time(void);
void Version();
void Run(TextBox& tbox);
void WaitAndProcess(TextBox& tbox);
}  // namespace ConsoleCommand

// @graphics.cc
void InitGraphics(void);
void InitDoubleBuffer(void);
void DrawPPMFile(EFIFile& file, int px, int py);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

// @liumos.c
class PersistentMemoryManager;
packed_struct LiumOS {
  struct {
    ACPI::RSDT* rsdt;
    ACPI::NFIT* nfit;
    ACPI::MADT* madt;
    ACPI::HPET* hpet;
    ACPI::SRAT* srat;
    ACPI::SLIT* slit;
    ACPI::FADT* fadt;
  } acpi;
  static constexpr int kNumOfPMEMManagers = 4;
  PersistentMemoryManager* pmem[kNumOfPMEMManagers];
  Sheet* vram_sheet;
  Sheet* screen_sheet;
  Console* main_console;
  KeyboardController* keyboard_ctrl;
  LocalAPIC* bsp_local_apic;
  CPUFeatureSet* cpu_features;
  KernelVirtualHeapAllocator* kernel_heap_allocator;
  EFI::MemoryMap* efi_memory_map;
  IA_PML4* kernel_pml4;
  uint64_t kernel_pml4_phys;
  Scheduler* scheduler;
  ProcessController* proc_ctrl;
  Process* root_process;
  uint64_t time_slice_count;
  bool is_multi_task_enabled;
  bool debug_mode_enabled;
  uint64_t direct_mapping_end_phys;
};
extern LiumOS* liumos;

template <typename T>
T GetKernelVirtAddrForPhysAddr(T paddr) {
  AssertAddressIsInLowerHalf(paddr);
  return reinterpret_cast<T>(reinterpret_cast<uint64_t>(paddr) +
                             liumos->cpu_features->kernel_phys_page_map_begin);
}

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table);

// @syscall.cc
void EnableSyscall();
