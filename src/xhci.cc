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
    if (it.first != 0x000D'1B36 && it.first != 0x31A8'8086)
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

  volatile OperationalRegisters& op_regs =
      *reinterpret_cast<volatile OperationalRegisters*>(
          reinterpret_cast<uint64_t>(cap_regs) + cap_regs->length);
  PutStringAndHex("USBCMD", op_regs.command);
  PutStringAndHex("USBSTS", op_regs.status);

  constexpr uint32_t kUSBCMDMaskRunStop = 0b01;
  constexpr uint32_t kUSBCMDMaskHCReset = 0b10;
  constexpr uint32_t kUSBSTSMaskHCHalted = 0b1;
  op_regs.command = op_regs.command & ~kUSBCMDMaskRunStop;
  while (!(op_regs.status & kUSBSTSMaskHCHalted)) {
    PutString("Waiting for HCHalt...\n");
  }
  op_regs.command = op_regs.command | kUSBCMDMaskHCReset;
  while (op_regs.command & kUSBCMDMaskHCReset) {
    PutString("Waiting for HCReset done...\n");
  }
  PutString("HCReset done.\n");

  int max_slots_enabled = kMaxSlots;
  constexpr uint64_t kOPREGConfigMaskMaxSlotsEn = 0b1111'1111;
  op_regs.config = (op_regs.config & ~kOPREGConfigMaskMaxSlotsEn) |
                   (max_slots_enabled & kOPREGConfigMaskMaxSlotsEn);
  PutStringAndHex("MaxSlotsEn", op_regs.config & kOPREGConfigMaskMaxSlotsEn);

  cmd_ring_ =
      liumos->kernel_heap_allocator
          ->AllocPages<TransferRequestBlockRing<kNumOfCmdTRBRingEntries>*>(
              ByteSizeToPageSize(
                  sizeof(TransferRequestBlockRing<kNumOfCmdTRBRingEntries>)),
              kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  cmd_ring_phys_addr_ =
      liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(cmd_ring_));
  cmd_ring_->Init(cmd_ring_phys_addr_);
  constexpr uint64_t kOPREGCRCRMaskRsvdP = 0b11'0000;
  constexpr uint64_t kOPREGCRCRMaskRingPtr = ~0b11'1111ULL;
  volatile uint64_t& crcr = op_regs.cmd_ring_ctrl;
  crcr = (crcr & kOPREGCRCRMaskRsvdP) |
         (cmd_ring_phys_addr_ & kOPREGCRCRMaskRingPtr);
  PutStringAndHex("CmdRing(Virt)", cmd_ring_);
  PutStringAndHex("CmdRing(Phys)", cmd_ring_phys_addr_);
  PutStringAndHex("CRCR", crcr);
}
