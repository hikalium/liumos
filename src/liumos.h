#pragma once

#include "acpi.h"
#include "asm.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"
#include "interrupt.h"

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
void InitGraphics();
void DrawCharacter(char c, int px, int py);
void DrawRect(int px, int py, int w, int h, uint32_t col);
void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);

// @keyboard.cc
constexpr uint16_t kIOPortKeyboardData = 0x0060;

namespace KeyID {
constexpr uint16_t kMaskBreak = 0x80;
constexpr uint16_t kMaskExtended = 0x8000;
constexpr uint16_t kEsc = KeyID::kMaskExtended | 0x0000;
constexpr uint16_t kF1 = KeyID::kMaskExtended | 0x0001;
constexpr uint16_t kF2 = KeyID::kMaskExtended | 0x0002;
constexpr uint16_t kF3 = KeyID::kMaskExtended | 0x0003;
constexpr uint16_t kF4 = KeyID::kMaskExtended | 0x0004;
constexpr uint16_t kF5 = KeyID::kMaskExtended | 0x0005;
constexpr uint16_t kF6 = KeyID::kMaskExtended | 0x0006;
constexpr uint16_t kF7 = KeyID::kMaskExtended | 0x0007;
constexpr uint16_t kF8 = KeyID::kMaskExtended | 0x0008;
constexpr uint16_t kF9 = KeyID::kMaskExtended | 0x0009;
constexpr uint16_t kF10 = KeyID::kMaskExtended | 0x000a;
constexpr uint16_t kF11 = KeyID::kMaskExtended | 0x000b;
constexpr uint16_t kF12 = KeyID::kMaskExtended | 0x000c;
constexpr uint16_t kNumLock = KeyID::kMaskExtended | 0x000d;
constexpr uint16_t kScrollLock = KeyID::kMaskExtended | 0x000e;
constexpr uint16_t kCapsLock = KeyID::kMaskExtended | 0x000f;
constexpr uint16_t kShiftL = KeyID::kMaskExtended | 0x0010;
constexpr uint16_t kShiftR = KeyID::kMaskExtended | 0x0011;
constexpr uint16_t kCtrlL = KeyID::kMaskExtended | 0x0012;
constexpr uint16_t kCtrlR = KeyID::kMaskExtended | 0x0013;
constexpr uint16_t kAltL = KeyID::kMaskExtended | 0x0014;
constexpr uint16_t kAltR = KeyID::kMaskExtended | 0x0015;
constexpr uint16_t kDelete = KeyID::kMaskExtended | 0x0016;
constexpr uint16_t kInsert = KeyID::kMaskExtended | 0x0017;
constexpr uint16_t kPause = KeyID::kMaskExtended | 0x0018;
constexpr uint16_t kBreak = KeyID::kMaskExtended | 0x0019;
constexpr uint16_t kPrintScreen = KeyID::kMaskExtended | 0x001a;
constexpr uint16_t kSysRq = KeyID::kMaskExtended | 0x001b;
constexpr uint16_t kCursorUp = KeyID::kMaskExtended | 0x001c;
constexpr uint16_t kCursorDown = KeyID::kMaskExtended | 0x001d;
constexpr uint16_t kCursorLeft = KeyID::kMaskExtended | 0x001e;
constexpr uint16_t kCursorRight = KeyID::kMaskExtended | 0x001f;
constexpr uint16_t kPageUp = KeyID::kMaskExtended | 0x0020;
constexpr uint16_t kPageDown = KeyID::kMaskExtended | 0x0021;
constexpr uint16_t kHome = KeyID::kMaskExtended | 0x0022;
constexpr uint16_t kEnd = KeyID::kMaskExtended | 0x0023;
constexpr uint16_t kCmdL = KeyID::kMaskExtended | 0x0024;
constexpr uint16_t kCmdR = KeyID::kMaskExtended | 0x0025;
constexpr uint16_t kMenu = KeyID::kMaskExtended | 0x0026;
constexpr uint16_t kKanji = KeyID::kMaskExtended | 0x0027;
constexpr uint16_t kHiragana = KeyID::kMaskExtended | 0x0028;
constexpr uint16_t kHenkan = KeyID::kMaskExtended | 0x0029;
constexpr uint16_t kMuhenkan = KeyID::kMaskExtended | 0x002a;

constexpr uint16_t kBackspace = KeyID::kMaskExtended | 0x0040;
constexpr uint16_t kTab = KeyID::kMaskExtended | 0x0041;
constexpr uint16_t kEnter = KeyID::kMaskExtended | 0x0042;

constexpr uint16_t kError = KeyID::kMaskExtended | 0x007e;
constexpr uint16_t kUnknown = KeyID::kMaskExtended | 0x007f;
}  // namespace KeyID

// @liumos.c
[[noreturn]] void Panic(const char* s);
void MainForBootProcessor(void* image_handle, EFISystemTable* system_table);

// @palloc.c
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

// @static.c
extern const GUID EFI_ACPITableGUID;
extern const GUID EFI_GraphicsOutputProtocolGUID;
