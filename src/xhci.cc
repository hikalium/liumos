#include "xhci.h"

#include "liumos.h"

XHCI* XHCI::xhci_;

static uint32_t GetBits(uint32_t v, int hi, int lo) {
  assert(hi > lo);
  return (v >> lo) & ((1 << (hi - lo + 1)) - 1);
}

void XHCI::Init() {
  PutString("XHCI::Init()\n");

  is_found_ = false;
  for (auto& it : PCI::GetInstance().GetDeviceList()) {
    if (it.first != 0x000D'1B36)
      continue;
    is_found_ = true;
    dev_ = it.second;
    PutString("XHCI Controller Found: ");
    PutString(PCI::GetDeviceName(it.first));
    PutString("\n");
    break;
  }
  if (!is_found_) {
    PutString("XHCI Controller Not Found\n");
    return;
  }

  constexpr uint32_t kPCIRegOffsetBAR = 0x10;
  constexpr uint64_t kPCIBARMaskType = 0b111;
  constexpr uint64_t kPCIBARMaskAddr = ~0b1111ULL;
  constexpr uint64_t kPCIBARBitsType64bitMemorySpace = 0b100;
  const uint64_t bar_raw_val =
      PCI::ReadConfigRegister64(dev_, kPCIRegOffsetBAR);
  PCI::WriteConfigRegister64(dev_, kPCIRegOffsetBAR, ~static_cast<uint64_t>(0));
  uint64_t base_addr_size_mask =
      PCI::ReadConfigRegister64(dev_, kPCIRegOffsetBAR) & kPCIBARMaskAddr;
  uint64_t base_addr_size = ~base_addr_size_mask + 1;
  PCI::WriteConfigRegister64(dev_, kPCIRegOffsetBAR, bar_raw_val);
  assert((bar_raw_val & kPCIBARMaskType) == kPCIBARBitsType64bitMemorySpace);
  const uint64_t base_addr = bar_raw_val & kPCIBARMaskAddr;
  PutStringAndHex("base_addr", base_addr);
  PutStringAndHex("size", base_addr_size);

  CapabilityRegisters* cap_regs =
      liumos->kernel_heap_allocator->MapPages<CapabilityRegisters*>(
          base_addr, ByteSizeToPageSize(base_addr_size),
          kPageAttrPresent | kPageAttrWritable | kPageAttrCacheDisable);
  PutStringAndHex("CAPLENGTH", cap_regs->length);
  PutStringAndHex("HCIVERSION", cap_regs->version);
  const uint32_t kHCSPARAMS1 = cap_regs->params[0];
  const uint8_t kMaxSlots = GetBits(kHCSPARAMS1, 31, 24);
  const uint8_t kMaxIntrs = GetBits(kHCSPARAMS1, 18, 8);
  const uint8_t kMaxPorts = GetBits(kHCSPARAMS1, 7, 0);
  PutStringAndHex("HCSPARAMS1", kHCSPARAMS1);
  PutStringAndHex("  MaxSlots", kMaxSlots);
  PutStringAndHex("  MaxIntrs", kMaxIntrs);
  PutStringAndHex("  MaxPorts", kMaxPorts);
}
