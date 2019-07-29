#include "xhci.h"

#include "liumos.h"

XHCI* XHCI::xhci_;

constexpr uint32_t kPCIRegOffsetBAR = 0x10;
constexpr uint32_t kPCIRegOffsetCommandAndStatus = 0x04;
constexpr uint64_t kPCIBARMaskType = 0b111;
constexpr uint64_t kPCIBARMaskAddr = ~0b1111ULL;
constexpr uint64_t kPCIBARBitsType64bitMemorySpace = 0b100;
constexpr uint64_t kPCIRegCommandAndStatusMaskBusMasterEnable = 1 << 2;
constexpr uint32_t kUSBCMDMaskRunStop = 0b01;
constexpr uint32_t kUSBCMDMaskHCReset = 0b10;
constexpr uint32_t kUSBSTSMaskHCHalted = 0b1;
constexpr uint32_t kTRBTypeNoOpCommand = 23;
constexpr uint32_t kTRBTypeCommandCompletionEvent = 33;
constexpr uint32_t kTRBTypePortStatusChanggeEvent = 34;
constexpr uint32_t kPortSCBitConnectStatusChange = 1 << 17;
constexpr uint32_t kPortSCBitPortReset = 1 << 4;

static uint32_t GetBits(uint32_t v, int hi, int lo) {
  assert(hi > lo);
  return (v >> lo) & ((1 << (hi - lo + 1)) - 1);
}

static uint64_t GetBits(uint64_t v, int hi, int lo) {
  assert(hi > lo);
  return (v >> lo) & ((1 << (hi - lo + 1)) - 1);
}

void EnsureBusMasterEnabled(PCI::DeviceLocation& dev) {
  uint32_t cmd_and_status =
      PCI::ReadConfigRegister32(dev, kPCIRegOffsetCommandAndStatus);
  cmd_and_status |= (1 << (10));
  PCI::WriteConfigRegister32(dev, kPCIRegOffsetCommandAndStatus,
                             cmd_and_status);
  assert(cmd_and_status & kPCIRegCommandAndStatusMaskBusMasterEnable);
}

void XHCI::ResetHostController() {
  op_regs_->command = op_regs_->command & ~kUSBCMDMaskRunStop;
  while (!(op_regs_->status & kUSBSTSMaskHCHalted)) {
    PutString("Waiting for HCHalt...\n");
  }
  op_regs_->command = op_regs_->command | kUSBCMDMaskHCReset;
  while (op_regs_->command & kUSBCMDMaskHCReset) {
    PutString("Waiting for HCReset done...\n");
  }
  PutString("HCReset done.\n");
}

class XHCI::EventRing {
 public:
  EventRing(int num_of_trb)
      : cycle_state_(1), index_(0), num_of_trb_(num_of_trb) {
    constexpr int kMapAttr =
        kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable;
    const int erst_size = sizeof(XHCI::EventRingSegmentTableEntry[1]);
    erst_ = liumos->kernel_heap_allocator
                ->AllocPages<volatile XHCI::EventRingSegmentTableEntry*>(
                    ByteSizeToPageSize(erst_size), kMapAttr);
    const size_t trbs_size =
        sizeof(XHCI::CommandCompletionEventTRB) * num_of_trb_;
    trbs_ = liumos->kernel_heap_allocator->AllocPages<volatile BasicTRB*>(
        ByteSizeToPageSize(trbs_size), kMapAttr);
    bzero(const_cast<void*>(reinterpret_cast<volatile void*>(trbs_)),
          trbs_size);
    erst_[0].ring_segment_base_address = GetTRBSPhysAddr();
    erst_[0].ring_segment_size = num_of_trb_;
    PutStringAndHex("erst phys", GetERSTPhysAddr());
    PutStringAndHex("erst[0].ring_segment_base_address",
                    erst_[0].ring_segment_base_address);
    PutStringAndHex("erst[0].ring_segment_size", erst_[0].ring_segment_size);
  }
  uint64_t GetERSTPhysAddr() {
    return liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(erst_));
  }
  uint64_t GetTRBSPhysAddr() {
    return liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(trbs_));
  }
  bool HasNextEvent() { return (trbs_[index_].control & 1) == cycle_state_; }
  volatile BasicTRB& PeekEvent() {
    assert(HasNextEvent());
    return trbs_[index_];
  }
  void PopEvent() {
    assert(HasNextEvent());
    index_++;
    if (index_ == num_of_trb_) {
      cycle_state_ ^= 1;
      index_ = 0;
    }
  }

 private:
  int cycle_state_;
  int index_;
  const int num_of_trb_;
  volatile XHCI::EventRingSegmentTableEntry* erst_;
  volatile BasicTRB* trbs_;
};

void XHCI::InitPrimaryInterrupter() {
  // 5.5.2 Interrupter Register Set
  // All registers of the Primary Interrupter shall be initialized
  // before setting the Run/Stop (RS) flag in the USBCMD register to 1.

  class EventRing& primary_event_ring = *new EventRing(kNumOfTRBForEventRing);

  auto& irs0 = rt_regs_->irs[0];
  irs0.erst_size = 1;
  irs0.erdp = primary_event_ring.GetTRBSPhysAddr();
  irs0.management = 0;
  irs0.erst_base = primary_event_ring.GetERSTPhysAddr();
  irs0.management = 1;

  primary_event_ring_ = &primary_event_ring;
};

void XHCI::InitSlotsAndContexts() {
  num_of_slots_enabled_ = max_slots_;
  constexpr uint64_t kOPREGConfigMaskMaxSlotsEn = 0b1111'1111;
  op_regs_->config = (op_regs_->config & ~kOPREGConfigMaskMaxSlotsEn) |
                     (num_of_slots_enabled_ & kOPREGConfigMaskMaxSlotsEn);

  device_context_base_array_ =
      liumos->kernel_heap_allocator->AllocPages<uint64_t*>(
          1, kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  for (int i = 0; i <= num_of_slots_enabled_; i++) {
    device_context_base_array_[i] = 0;
  }
  device_context_base_array_phys_addr_ = liumos->kernel_pml4->v2p(
      reinterpret_cast<uint64_t>(device_context_base_array_));
  op_regs_->device_ctx_base_addr_array_ptr =
      device_context_base_array_phys_addr_;
}

void XHCI::InitCommandRing() {
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
  volatile uint64_t& crcr = op_regs_->cmd_ring_ctrl;
  crcr = (crcr & kOPREGCRCRMaskRsvdP) |
         (cmd_ring_phys_addr_ & kOPREGCRCRMaskRingPtr) | 1;
  PutStringAndHex("CmdRing(Virt)", cmd_ring_);
  PutStringAndHex("CmdRing(Phys)", cmd_ring_phys_addr_);
  PutStringAndHex("CRCR", crcr);
}

void XHCI::NotifyHostControllerDoorbell() {
  db_regs_[0] = 0;
}
uint32_t XHCI::ReadPORTSC(int slot) {
  return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(op_regs_) +
                                      0x400 + 0x10 * (slot - 1));
}
void XHCI::WritePORTSC(int slot, uint32_t data) {
  *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(op_regs_) + 0x400 +
                               0x10 * (slot - 1)) = data;
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

  EnsureBusMasterEnabled(dev_);

  const uint64_t bar_raw_val =
      PCI::ReadConfigRegister64(dev_, kPCIRegOffsetBAR);
  PCI::WriteConfigRegister64(dev_, kPCIRegOffsetBAR, ~static_cast<uint64_t>(0));
  uint64_t base_addr_size_mask =
      PCI::ReadConfigRegister64(dev_, kPCIRegOffsetBAR) & kPCIBARMaskAddr;
  uint64_t base_addr_size = ~base_addr_size_mask + 1;
  PCI::WriteConfigRegister64(dev_, kPCIRegOffsetBAR, bar_raw_val);
  assert((bar_raw_val & kPCIBARMaskType) == kPCIBARBitsType64bitMemorySpace);
  const uint64_t base_addr = bar_raw_val & kPCIBARMaskAddr;

  cap_regs_ = liumos->kernel_heap_allocator->MapPages<CapabilityRegisters*>(
      base_addr, ByteSizeToPageSize(base_addr_size),
      kPageAttrPresent | kPageAttrWritable | kPageAttrCacheDisable);

  const uint32_t kHCSPARAMS1 = cap_regs_->params[0];
  max_slots_ = GetBits(kHCSPARAMS1, 31, 24);
  max_intrs_ = GetBits(kHCSPARAMS1, 18, 8);
  max_ports_ = GetBits(kHCSPARAMS1, 7, 0);

  op_regs_ = reinterpret_cast<volatile OperationalRegisters*>(
      reinterpret_cast<uint64_t>(cap_regs_) + cap_regs_->length);

  rt_regs_ = reinterpret_cast<volatile RuntimeRegisters*>(
      reinterpret_cast<uint64_t>(cap_regs_) + cap_regs_->rtsoff);

  db_regs_ = reinterpret_cast<volatile uint32_t*>(
      reinterpret_cast<uint64_t>(cap_regs_) + cap_regs_->dboff);

  ResetHostController();
  InitPrimaryInterrupter();
  InitSlotsAndContexts();
  InitCommandRing();

  op_regs_->command = op_regs_->command | kUSBCMDMaskRunStop | (1 << 3);
  while (op_regs_->status & kUSBSTSMaskHCHalted) {
    PutString("Waiting for HCHalt == 0...\n");
  }

  {
    volatile BasicTRB* no_op_trb = cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
    no_op_trb[0].data = 0;
    no_op_trb[0].option = 0;
    no_op_trb[0].control = (kTRBTypeNoOpCommand << 10);
    cmd_ring_->Push();
  }
  {
    volatile BasicTRB* no_op_trb = cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
    no_op_trb[0].data = 0;
    no_op_trb[0].option = 0;
    no_op_trb[0].control = (kTRBTypeNoOpCommand << 10);
    cmd_ring_->Push();
  }

  NotifyHostControllerDoorbell();

  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    PutStringAndHex("Port Reset", slot);
    WritePORTSC(slot, kPortSCBitPortReset);
  }
}

void XHCI::PollEvents() {
  if (!primary_event_ring_)
    return;
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    uint32_t portsc = ReadPORTSC(slot);
    if (!(portsc & kPortSCBitConnectStatusChange))
      continue;
    PutStringAndHex("XHCI Connect Status Changed", slot);
    PutStringAndHex("  PORTSC", portsc);
    WritePORTSC(slot, kPortSCBitConnectStatusChange);
  }
  if (primary_event_ring_->HasNextEvent()) {
    volatile BasicTRB& e = primary_event_ring_->PeekEvent();
    uint8_t type = GetBits(e.control, 15, 10);

    switch (type) {
      case kTRBTypeCommandCompletionEvent:
        PutString("CommandCompletionEvent\n");
        PutStringAndHex("  CompletionCode", GetBits(e.option, 31, 24));
        break;
      case kTRBTypePortStatusChanggeEvent:
        PutString("PortStatusChangeEvent\n");
        PutStringAndHex("  Port ID", GetBits(e.data, 31, 24));
        PutStringAndHex("  CompletionCode", GetBits(e.option, 31, 24));
        break;
      default:
        PutString("Event ");
        PutStringAndHex("type", type);
        PutStringAndHex("  e.data", e.data);
        PutStringAndHex("  e.opt ", e.option);
        PutStringAndHex("  e.ctrl", e.control);
        break;
    }
    primary_event_ring_->PopEvent();
  }
}
