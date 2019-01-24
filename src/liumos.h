#pragma once

#include "acpi.h"
#include "asm.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"
#include "interrupt.h"
#include "keyid.h"
#include "paging.h"

constexpr uint64_t kKernelBaseAddr = 0xFFFF'FFFF'0000'0000;

// @apic.cc
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

void SendEndOfInterruptToLocalAPIC();
void InitIOAPIC(uint64_t local_apic_id);

// @console.c
void ResetCursorPosition();
void EnableVideoModeForConsole();
void PutChar(char c);
void PutString(const char* s);
void PutChars(const char* s, int n);
void PutHex64(uint64_t value);
void PutHex64ZeroFilled(uint64_t value);
void PutStringAndHex(const char* s, uint64_t value);
void PutStringAndHex(const char* s, void* value);

// @console_command.cc
namespace ConsoleCommand {
void ShowNFIT(void);
void ShowMADT(void);
void ShowEFIMemoryMap(void);
void Free(void);
void CheckWrite(void);
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

// @gdt.c
class GDT {
 public:
  static constexpr uint64_t kDescBitTypeData = 0b10ULL << 43;
  static constexpr uint64_t kDescBitTypeCode = 0b11ULL << 43;

  static constexpr uint64_t kDescBitForSystemSoftware = 1ULL << 52;
  static constexpr uint64_t kDescBitPresent = 1ULL << 47;
  static constexpr uint64_t kDescBitOfsDPL = 45;
  static constexpr uint64_t kDescBitMaskDPL = 0b11ULL << kDescBitOfsDPL;
  static constexpr uint64_t kDescBitAccessed = 1ULL << 40;

  static constexpr uint64_t kCSDescBitLongMode = 1ULL << 53;
  static constexpr uint64_t kCSDescBitReadable = 1ULL << 41;

  static constexpr uint64_t kDSDescBitWritable = 1ULL << 41;

  static constexpr uint64_t kKernelCSIndex = 1;
  static constexpr uint64_t kKernelDSIndex = 2;

  static constexpr uint64_t kKernelCSSelector = kKernelCSIndex << 3;
  static constexpr uint64_t kKernelDSSelector = kKernelDSIndex << 3;

  static constexpr uint64_t kNumOfDescriptors = 3;

  void Init(void);
  void Print(void);

 private:
  GDTR gdtr_;
  uint64_t descriptors_[kNumOfDescriptors];
};

// @generic.h

[[noreturn]] void Panic(const char* s);
inline void* operator new(size_t, void* where) {
  return where;
}

// @graphics.cc
extern int xsize;
void InitGraphics();
void DrawCharacter(char c, int px, int py);
void DrawRect(int px, int py, int w, int h, uint32_t col);
void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

// @libfunc.cc
int strncmp(const char* s1, const char* s2, size_t n);

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

void MainForBootProcessor(void* image_handle, EFI::SystemTable* system_table);

// @phys_page_allocator.cc
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
