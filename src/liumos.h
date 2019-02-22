#pragma once

#include "acpi.h"
#include "apic.h"
#include "asm.h"
#include "efi.h"
#include "gdt.h"
#include "generic.h"
#include "guid.h"
#include "hpet.h"
#include "interrupt.h"
#include "keyid.h"
#include "paging.h"
#include "serial.h"
#include "sheet.h"
#include "text_box.h"

constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000;

// @console.c
void ResetCursorPosition();
void EnableVideoModeForConsole();
void SetSerialForConsole(SerialPort* p);
void PutChar(char c);
void PutString(const char* s);
void PutChars(const char* s, int n);
void PutHex64(uint64_t value);
void PutHex64ZeroFilled(uint64_t value);
void PutStringAndHex(const char* s, uint64_t value);
void PutStringAndHex(const char* s, void* value);
void PutStringAndBool(const char* s, bool cond);

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
void ParseELFFile(File& logo_file);

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

// @generic.h

[[noreturn]] void Panic(const char* s);
inline void* operator new(size_t, void* where) {
  return where;
}

// @graphics.cc
extern Sheet* screen_sheet;
void InitGraphics(void);
void InitDoubleBuffer(void);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

// @libfunc.cc
int strncmp(const char* s1, const char* s2, size_t n);
void* memcpy(void* dst, const void* src, size_t n);

// @liumos.c
constexpr uint64_t kPageSizeExponent = 12;
constexpr uint64_t kPageSize = 1 << kPageSizeExponent;
inline uint64_t ByteSizeToPageSize(uint64_t byte_size) {
  return (byte_size + kPageSize - 1) >> kPageSizeExponent;
}

class PhysicalPageAllocator;

extern EFI::MemoryMap efi_memory_map;
extern PhysicalPageAllocator* page_allocator;
extern int kMaxPhyAddr;
extern LocalAPIC bsp_local_apic;
extern CPUFeatureSet cpu_features;
extern SerialPort com1;
extern File hello_bin_file;
extern File liumos_elf_file;
extern HPET hpet;

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table);

// @phys_page_allocator.cc
class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_(nullptr) {}
  void FreePages(void* phys_addr, uint64_t num_of_pages);
  template <typename T>
  T AllocPages(int num_of_pages) {
    FreeInfo* info = head_;
    void* addr = nullptr;
    while (info) {
      addr = info->ProvidePages(num_of_pages);
      if (addr)
        return reinterpret_cast<T>(addr);
      info = info->GetNext();
    }
    Panic("Cannot allocate pages");
  }
  void Print();

 private:
  class FreeInfo {
   public:
    FreeInfo(uint64_t num_of_pages, FreeInfo* next)
        : num_of_pages_(num_of_pages), next_(next) {}
    FreeInfo* GetNext() const { return next_; }
    void* ProvidePages(int num_of_req_pages);
    void Print();

   private:
    bool CanProvidePages(uint64_t num_of_req_pages) const {
      return num_of_req_pages < num_of_pages_;
    }

    uint64_t num_of_pages_;
    FreeInfo* next_;
  };

  FreeInfo* head_;
};
