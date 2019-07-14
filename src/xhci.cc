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

static uint32_t GetBits(uint32_t v, int hi, int lo) {
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

void XHCI::InitPrimaryInterrupter() {
  // 5.5.2 Interrupter Register Set
  // All registers of the Primary Interrupter shall be initialized
  // before setting the Run/Stop (RS) flag in the USBCMD register to 1.

  size_t erst_size = sizeof(EventRingSegmentTableEntry[kNumOfERSForEventRing]);
  volatile EventRingSegmentTableEntry* erst =
      liumos->kernel_heap_allocator
          ->AllocPages<volatile EventRingSegmentTableEntry*>(
              ByteSizeToPageSize(erst_size),
              kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  size_t trbs_size = sizeof(CommandCompletionEventTRB[kNumOfTRBForEventRing]);
  volatile CommandCompletionEventTRB* trbs =
      liumos->kernel_heap_allocator
          ->AllocPages<volatile CommandCompletionEventTRB*>(
              ByteSizeToPageSize(trbs_size),
              kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  bzero(const_cast<void*>(reinterpret_cast<volatile void*>(trbs)), trbs_size);

  erst[0].ring_segment_base_address =
      liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(trbs));
  erst[0].ring_segment_size = kNumOfTRBForEventRing;
  PutStringAndHex("erst phys",
                  liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(erst)));
  PutStringAndHex("erst[0].ring_segment_base_address",
                  erst[0].ring_segment_base_address);
  PutStringAndHex("erst[0].ring_segment_size", erst[0].ring_segment_size);

  auto& irs0 = rt_regs_->irs[0];
  irs0.erst_size = 1;
  irs0.erdp = liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(erst));
  irs0.management = 0;
  irs0.erst_base = irs0.erdp;
  irs0.management = 1;

  primary_event_ring_buf_ = trbs;
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

  BasicTRB* ent = cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  for (int i = 0; i < kNumOfCmdTRBRingEntries; i++) {
    ent[i].control = 0;
  }
  constexpr uint64_t kOPREGCRCRMaskRsvdP = 0b11'0000;
  constexpr uint64_t kOPREGCRCRMaskRingPtr = ~0b11'1111ULL;
  volatile uint64_t& crcr = op_regs_->cmd_ring_ctrl;
  crcr = (crcr & kOPREGCRCRMaskRsvdP) |
         (cmd_ring_phys_addr_ & kOPREGCRCRMaskRingPtr);
  PutStringAndHex("CmdRing(Virt)", cmd_ring_);
  PutStringAndHex("CmdRing(Phys)", cmd_ring_phys_addr_);
  PutStringAndHex("CRCR", crcr);
}

void XHCI::NotifyHostControllerDoorbell() {
  db_regs_[0] = 0;
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

  BasicTRB* no_op_trb = cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  constexpr uint32_t kTRBTypeNoOp = 23;
  no_op_trb[0].data = 0;
  no_op_trb[0].option = 0;
  no_op_trb[0].control = (kTRBTypeNoOp << 10) | 1;

  auto& irs0 = rt_regs_->irs[0];
  PutStringAndHex("IRS[0].management", irs0.management);
  PutStringAndHex("IRS[0].erst_size", irs0.erst_size);
  PutStringAndHex("IRS[0].erst_base", irs0.erst_base);
  PutStringAndHex("IRS[0].erdp", irs0.erdp);

  op_regs_->command = op_regs_->command | kUSBCMDMaskRunStop | (1 << 3);
  while (op_regs_->status & kUSBSTSMaskHCHalted) {
    PutString("Waiting for HCHalt == 0...\n");
  }

  PutStringAndHex("Event.info", primary_event_ring_buf_[0].info);

  NotifyHostControllerDoorbell();

  for (int i = 0; i < 3; i++) {
    PutStringAndHex("#", i);
    PutStringAndHex("op.cmd", op_regs_->command);
    PutStringAndHex("op.status", op_regs_->status);
    PutStringAndHex("e.cmd_trb_ptr", primary_event_ring_buf_[i].cmd_trb_ptr);
    PutStringAndHex("e.ccode", primary_event_ring_buf_[i].param[3]);
    PutStringAndHex("e.info", primary_event_ring_buf_[i].info);
    PutStringAndHex("e.type", GetBits(primary_event_ring_buf_[i].info, 15, 10));
  }
}
