#pragma once

#include "acpi.h"
#include "asm.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"
#include "interrupt.h"
#include "keyid.h"

/*
class SpinLock {
  public:
  SpinLock() : lock_(0){};
  void Lock(){}
  private:
  volatile uint64_t lock_;
};
*/

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
}  // namespace ConsoleCommand

// @font.gen.c
extern uint8_t font[0x100][16];

// @gdt.c
class GDT {
 public:
  GDT() {
    ReadGDTR(&gdtr_);
    PutStringAndHex("GDT base", gdtr_.base);
    PutStringAndHex("GDT limit", gdtr_.limit);
    Print();
  }
  void Print(void);

 private:
  GDTR gdtr_;
};

// @generic.h

[[noreturn]] void Panic(const char* s);
void __assert(const char* expr_str, const char* file, int line);
#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))
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
class PhysicalPageAllocator;

extern EFI::MemoryMap efi_memory_map;
extern PhysicalPageAllocator* page_allocator;

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
