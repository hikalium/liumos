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
#include "kernel_virtual_heap_allocator.h"
#include "keyboard.h"
#include "keyid.h"
#include "paging.h"
#include "phys_page_allocator.h"
#include "process.h"
#include "serial.h"
#include "sheet.h"
#include "sys_constant.h"
#include "text_box.h"

constexpr uint64_t kLAPICRegisterAreaPhysBase = 0x0000'0000'FEE0'0000ULL;
constexpr uint64_t kLAPICRegisterAreaVirtBase = 0xFFFF'FFFF'FEE0'0000ULL;
constexpr uint64_t kLAPICRegisterAreaByteSize = 0x0000'0000'0010'0000ULL;

constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000ULL;

constexpr uint64_t kKernelStackPagesForEachProcess = 2;

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

// @font.gen.c
extern uint8_t font[0x100][16];

// @graphics.cc
void InitGraphics(void);
void InitDoubleBuffer(void);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

// @libfunc.cc
extern "C" {
int strncmp(const char* s1, const char* s2, size_t n);
int strcmp(const char* s1, const char* s2);
void* memcpy(void* dst, const void* src, size_t n);
void bzero(volatile void* s, size_t n);
int atoi(const char* str);
}
template <typename T>
inline T min(T a, T b) {
  return a < b ? a : b;
}

// @liumos.c
packed_struct LoaderInfo {
  struct {
    EFIFile* hello_bin;
    EFIFile* pi_bin;
    EFIFile* liumos_elf;
  } files;
  EFI* efi;
};
class PersistentMemoryManager;
packed_struct LiumOS {
  struct {
    ACPI::RSDT* rsdt;
    ACPI::NFIT* nfit;
    ACPI::MADT* madt;
    ACPI::HPET* hpet;
    ACPI::SRAT* srat;
    ACPI::SLIT* slit;
  } acpi;
  LoaderInfo loader_info;
  static constexpr int kNumOfPMEMManagers = 4;
  PersistentMemoryManager* pmem[kNumOfPMEMManagers];
  Sheet* vram_sheet;
  Sheet* screen_sheet;
  Console* main_console;
  KeyboardController* keyboard_ctrl;
  LocalAPIC* bsp_local_apic;
  CPUFeatureSet* cpu_features;
  PhysicalPageAllocator* dram_allocator;
  KernelVirtualHeapAllocator* kernel_heap_allocator;
  HPET* hpet;
  EFI::MemoryMap* efi_memory_map;
  IA_PML4* kernel_pml4;
  Scheduler* scheduler;
  ProcessController* proc_ctrl;
  IDT* idt;
  Process* root_process;
  Process* sub_process;
  uint64_t time_slice_count;
  bool is_multi_task_enabled;
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
