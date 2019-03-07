#pragma once

#include "acpi.h"
#include "apic.h"
#include "asm.h"
#include "console.h"
#include "efi.h"
#include "gdt.h"
#include "generic.h"
#include "githash.h"
#include "guid.h"
#include "hpet.h"
#include "interrupt.h"
#include "keyboard.h"
#include "keyid.h"
#include "paging.h"
#include "phys_page_allocator.h"
#include "serial.h"
#include "sheet.h"
#include "sys_constant.h"
#include "text_box.h"

constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000;

// @console_command.cc
namespace ConsoleCommand {
void ShowNFIT(void);
void ShowMADT(void);
void ShowSRAT(void);
void ShowSLIT(void);
void ShowEFIMemoryMap(void);
void Free(void);
void Time(void);
void Process(TextBox& tbox);
}  // namespace ConsoleCommand

// @elf.cc
class File;
ExecutionContext* LoadELFAndLaunchProcess(File& file);
void LoadKernelELF(File& file);

// @file.cc
class File {
 public:
  void LoadFromEFISimpleFS(const wchar_t* file_name);
  const uint8_t* GetBuf() { return buf_pages_; }
  uint64_t GetFileSize() { return file_size_; }

 private:
  static constexpr int kFileNameSize = 16;
  char file_name_[kFileNameSize + 1];
  uint64_t file_size_;
  uint8_t* buf_pages_;
};

// @font.gen.c
extern uint8_t font[0x100][16];

// @graphics.cc
extern Sheet* screen_sheet;
void InitGraphics(void);
void InitDoubleBuffer(void);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

// @libfunc.cc
int strncmp(const char* s1, const char* s2, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int atoi(const char* str);

// @liumos.c
packed_struct LiumOS {
  struct {
    ACPI::RSDT* rsdt;
    ACPI::NFIT* nfit;
    ACPI::MADT* madt;
    ACPI::HPET* hpet;
    ACPI::SRAT* srat;
    ACPI::SLIT* slit;
  } acpi;
  Sheet* screen_sheet;
  Console* main_console;
  KeyboardController* keyboard_ctrl;
  SerialPort* com1;
  LocalAPIC* bsp_local_apic;
  CPUFeatureSet* cpu_features;
  PhysicalPageAllocator* dram_allocator;
  PhysicalPageAllocator* pmem_allocator;
  HPET* hpet;
  EFI::MemoryMap* efi_memory_map;
  File* hello_bin_file;
};
extern LiumOS* liumos;

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table);

// @syscall.c
extern "C" void SyscallHandler(uint64_t* args);
