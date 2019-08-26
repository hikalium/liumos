#include "xhci.h"

#include "kernel.h"
#include "liumos.h"

#include <unordered_map>

XHCI* XHCI::xhci_;

static std::unordered_map<uint64_t, int> slot_request_for_port;

constexpr uint32_t kPCIRegOffsetBAR = 0x10;
constexpr uint32_t kPCIRegOffsetCommandAndStatus = 0x04;
constexpr uint64_t kPCIBARMaskType = 0b111;
constexpr uint64_t kPCIBARMaskAddr = ~0b1111ULL;
constexpr uint64_t kPCIBARBitsType64bitMemorySpace = 0b100;
constexpr uint64_t kPCIRegCommandAndStatusMaskBusMasterEnable = 1 << 2;
constexpr uint32_t kUSBCMDMaskRunStop = 0b01;
constexpr uint32_t kUSBCMDMaskHCReset = 0b10;

constexpr uint32_t kTRBTypeEnableSlotCommand = 9;
constexpr uint32_t kTRBTypeAddressDeviceCommand = 11;
constexpr uint32_t kTRBTypeNoOpCommand = 23;
constexpr uint32_t kTRBTypeCommandCompletionEvent = 33;
constexpr uint32_t kTRBTypePortStatusChangeEvent = 34;

constexpr uint32_t kPortSCBitConnectStatusChange = 1 << 17;
constexpr uint32_t kPortSCBitEnableStatusChange = 1 << 18;
constexpr uint32_t kPortSCBitPortResetChange = 1 << 21;
constexpr uint32_t kPortSCBitPortLinkStateChange = 1 << 22;
constexpr uint32_t kPortSCBitPortReset = 1 << 4;
constexpr uint32_t kPortSCBitPortPower = 1 << 9;
constexpr uint32_t kPortSCPreserveMask = 0b00001110000000011100001111100000;

constexpr uint32_t kUSBSTSBitHCHalted = 0b1;
constexpr uint32_t kUSBSTSBitHCError = 1 << 12;

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
  while (!(op_regs_->status & kUSBSTSBitHCHalted)) {
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
  EventRing(int num_of_trb, InterrupterRegisterSet& irs)
      : cycle_state_(1), index_(0), num_of_trb_(num_of_trb), irs_(irs) {
    constexpr int kMapAttr =
        kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable;
    const int erst_size = sizeof(XHCI::EventRingSegmentTableEntry[1]);
    erst_ = liumos->kernel_heap_allocator
                ->AllocPages<volatile XHCI::EventRingSegmentTableEntry*>(
                    ByteSizeToPageSize(erst_size), kMapAttr);
    const size_t trbs_size =
        sizeof(XHCI::CommandCompletionEventTRB) * num_of_trb_;
    trbs_ = liumos->kernel_heap_allocator->AllocPages<BasicTRB*>(
        ByteSizeToPageSize(trbs_size), kMapAttr);
    bzero(const_cast<void*>(reinterpret_cast<volatile void*>(trbs_)),
          trbs_size);
    erst_[0].ring_segment_base_address = GetTRBSPhysAddr();
    erst_[0].ring_segment_size = num_of_trb_;
    PutStringAndHex("erst phys", GetERSTPhysAddr());
    PutStringAndHex("erst[0].ring_segment_base_address",
                    erst_[0].ring_segment_base_address);
    PutStringAndHex("erst[0].ring_segment_size", erst_[0].ring_segment_size);

    irs_.erst_size = 1;
    irs_.erdp = GetTRBSPhysAddr();
    irs_.management = 0;
    irs_.erst_base = GetERSTPhysAddr();
    irs_.management = 0;
  }
  uint64_t GetERSTPhysAddr() {
    return liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(erst_));
  }
  uint64_t GetTRBSPhysAddr() {
    return liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(trbs_));
  }
  bool HasNextEvent() { return (trbs_[index_].control & 1) == cycle_state_; }
  BasicTRB& PeekEvent() {
    assert(HasNextEvent());
    return trbs_[index_];
  }
  void PopEvent() {
    assert(HasNextEvent());
    AdvanceERDP();
    index_++;
    if (index_ == num_of_trb_) {
      cycle_state_ ^= 1;
      index_ = 0;
    }
  }
  void AdvanceERDP() {
    uint64_t kERDPPreserveMask = 0b1111;
    irs_.erdp = (GetTRBSPhysAddr() + sizeof(BasicTRB) * index_) |
                (irs_.erdp & kERDPPreserveMask);
  }

 private:
  int cycle_state_;
  int index_;
  const int num_of_trb_;
  volatile XHCI::EventRingSegmentTableEntry* erst_;
  BasicTRB* trbs_;
  InterrupterRegisterSet& irs_;
};

void XHCI::InitPrimaryInterrupter() {
  // 5.5.2 Interrupter Register Set
  // All registers of the Primary Interrupter shall be initialized
  // before setting the Run/Stop (RS) flag in the USBCMD register to 1.

  class EventRing& primary_event_ring =
      *new EventRing(kNumOfTRBForEventRing, rt_regs_->irs[0]);
  primary_event_ring_ = &primary_event_ring;
};

void XHCI::InitSlotsAndContexts() {
  num_of_slots_enabled_ = max_slots_;
  constexpr uint64_t kOPREGConfigMaskMaxSlotsEn = 0b1111'1111;
  op_regs_->config = (op_regs_->config & ~kOPREGConfigMaskMaxSlotsEn) |
                     (num_of_slots_enabled_ & kOPREGConfigMaskMaxSlotsEn);

  device_context_base_array_ =
      liumos->kernel_heap_allocator->AllocPages<DeviceContext volatile**>(
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

  rt_regs_ = reinterpret_cast<RuntimeRegisters*>(
      reinterpret_cast<uint64_t>(cap_regs_) + cap_regs_->rtsoff);

  db_regs_ = reinterpret_cast<volatile uint32_t*>(
      reinterpret_cast<uint64_t>(cap_regs_) + cap_regs_->dboff);

  ResetHostController();
  InitPrimaryInterrupter();
  InitSlotsAndContexts();
  InitCommandRing();

  op_regs_->command = op_regs_->command | kUSBCMDMaskRunStop | (1 << 3);
  while (op_regs_->status & kUSBSTSBitHCHalted) {
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

  // 4.3.1 Resetting a Root Hub Port
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    PutStringAndHex("Port Reset", slot);
    WritePORTSC(slot, kPortSCBitPortReset);
    while ((ReadPORTSC(slot) & kPortSCBitPortReset)) {
      PutString(".");
    }
    PutStringAndHex("done. PORTSC", ReadPORTSC(slot));
  }
  // Make Port Power is not off (PP = 1)
  // See 4.19.1.1 USB2 Root Hub Port
  // Figure 4-25: USB2 Root Hub Port State Machine
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    WritePORTSC(slot, ReadPORTSC(slot) | kPortSCBitPortPower);
  }
}

void XHCI::HandlePortStatusChange(int port) {
  uint32_t portsc = ReadPORTSC(port);
  PutStringAndHex("XHCI Port Status Changed", port);
  PutStringAndHex("  PORTSC", portsc);
  if (portsc & kPortSCBitConnectStatusChange) {
    PutString("  Connect Status: ");
    PutString((portsc & 1) ? "Connected\n" : "Disconnected\n");
  }
  if (portsc & kPortSCBitEnableStatusChange) {
    PutString("  Enable Status: ");
    PutString((portsc & 0b10) ? "Enabled\n" : "Disabled\n");
  }
  if (portsc & kPortSCBitPortResetChange) {
    PutString("  PortReset: ");
    PutString((portsc & 0b1000) ? "Ongoing\n" : "Done\n");
  }
  if (portsc & kPortSCBitPortLinkStateChange) {
    PutString("  LinkState: 0x");
    PutHex64(GetBits(portsc, 8, 5));
    PutString("\n");
  }

  WritePORTSC(port, (portsc & kPortSCPreserveMask) | (0b1111111 << 17));
  if ((portsc & 1) == 0)
    return;

  // 4.3.2 Device Slot Assignment
  // 4.6.3 Enable Slot
  PutString("  Send Enable Slot\n");
  BasicTRB& enable_slot_trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  slot_request_for_port.insert(
      {liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(&enable_slot_trb)),
       port});
  enable_slot_trb.data = 0;
  enable_slot_trb.option = 0;
  enable_slot_trb.control = (kTRBTypeEnableSlotCommand << 10);
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
}

class XHCI::DeviceContext {
  friend class InputContext;

 public:
  static constexpr int kDCIEPContext0 = 1;
  static constexpr int kEPTypeControl = 4;
  static DeviceContext& Alloc(int max_dci) {
    constexpr int kMapAttr =
        kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable;
    const int num_of_ctx = max_dci + 1;
    size_t size = 0x20 * num_of_ctx;
    DeviceContext* ctx =
        liumos->kernel_heap_allocator->AllocPages<DeviceContext*>(
            ByteSizeToPageSize(size), kMapAttr);
    new (ctx) DeviceContext(max_dci);
    return *ctx;
  }
  void SetContextEntries(uint32_t num_of_ent) {
    constexpr uint32_t kNumOfEntBits = 5;
    constexpr uint32_t kNumOfEntShift = 27;
    constexpr uint32_t kNumOfEntMask = ((1 << kNumOfEntBits) - 1)
                                       << kNumOfEntShift;
    slot_ctx[0] &= ~kNumOfEntMask;
    slot_ctx[0] |= (num_of_ent << kNumOfEntShift) & kNumOfEntMask;
  }
  void SetRootHubPortNumber(uint32_t root_port_num) {
    constexpr uint32_t kMaskBits = 8;
    constexpr uint32_t kMaskShift = 16;
    constexpr uint32_t kMask = ((1 << kMaskBits) - 1) << kMaskShift;
    slot_ctx[1] &= ~kMask;
    slot_ctx[1] |= (root_port_num << kMaskShift) & kMask;
  }
  void SetEPType(int dci, uint32_t type) {
    constexpr uint32_t kMaskBits = 3;
    constexpr uint32_t kMaskShift = 3;
    constexpr uint32_t kMask = ((1 << kMaskBits) - 1) << kMaskShift;
    endpoint_ctx[dci][1] &= ~kMask;
    endpoint_ctx[dci][1] |= (type << kMaskShift) & kMask;
  }
  void SetTRDequeuePointer(int dci, uint64_t tr_deq_ptr) {
    endpoint_ctx[dci][2] &= 0b1111;
    endpoint_ctx[dci][2] |= static_cast<uint32_t>(tr_deq_ptr & ~0b1111ULL);
    endpoint_ctx[dci][3] |= tr_deq_ptr >> 32;
  }
  void SetDequeueCycleState(int dci, uint64_t dcs) {
    endpoint_ctx[dci][2] &= ~0b1;
    endpoint_ctx[dci][2] |= (dcs & 0b1);
  }
  void SetErrorCount(int dci, uint32_t c_err) {
    constexpr uint32_t kMaskBits = 2;
    constexpr uint32_t kMaskShift = 1;
    constexpr uint32_t kMask = ((1 << kMaskBits) - 1) << kMaskShift;
    endpoint_ctx[dci][1] &= ~kMask;
    endpoint_ctx[dci][1] |= (c_err << kMaskShift) & kMask;
  }
  void SetMaxPacketSize(int dci, uint16_t max_packet_size) {
    endpoint_ctx[dci][1] &= 0xFFFF;
    endpoint_ctx[dci][1] |= static_cast<uint32_t>(max_packet_size) << 16;
  }

 private:
  DeviceContext(int max_dci) {
    assert(0 <= max_dci && max_dci <= 31);
    bzero(this, 0x20 * (max_dci + 1));
  }
  volatile uint32_t slot_ctx[8];
  volatile uint32_t endpoint_ctx[31][8];
};

class XHCI::InputContext {
 public:
  static InputContext& Alloc(int num_of_ctx) {
    static_assert(offsetof(InputContext, device_ctx_) == 0x20);
    constexpr int kMapAttr =
        kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable;
    assert(1 <= num_of_ctx && num_of_ctx <= 33);
    size_t size = 0x20 * num_of_ctx;
    InputContext& ctx =
        *liumos->kernel_heap_allocator->AllocPages<InputContext*>(
            ByteSizeToPageSize(size), kMapAttr);
    bzero(&ctx.input_ctrl_ctx_[0], sizeof(input_ctrl_ctx_));
    if (num_of_ctx > 1) {
      volatile uint32_t& add_ctx_flags = ctx.input_ctrl_ctx_[1];
      add_ctx_flags = (1 << (num_of_ctx - 1)) - 1;
    }
    new (&ctx.device_ctx_) DeviceContext(num_of_ctx - 2);
    return ctx;
  }

  InputContext() = delete;
  DeviceContext& GetDeviceContext() { return device_ctx_; }

 private:
  volatile uint32_t input_ctrl_ctx_[8];
  DeviceContext device_ctx_;
};
static_assert(sizeof(XHCI::InputContext) == 0x420);

constexpr uint32_t kPortSpeedLS = 2;
constexpr uint32_t kPortSpeedHS = 3;
constexpr uint32_t kPortSpeedSS = 4;

static uint16_t GetMaxPacketSizeFromPORTSCPortSpeed(uint32_t port_speed) {
  // 4.3 USB Device Initialization
  // 7. For LS, HS, and SS devices; 8, 64, and 512 bytes, respectively,
  // are the only packet sizes allowed for the Default Control Endpoint
  //    • Max Packet Size = The default maximum packet size for the Default
  //    Control Endpoint, as function of the PORTSC Port Speed field.
  // 7.2.2.1.1 Default USB Speed ID Mapping
  if (port_speed == kPortSpeedLS)
    return 8;
  if (port_speed == kPortSpeedHS)
    return 64;
  if (port_speed == kPortSpeedSS)
    return 512;
  Panic("GetMaxPacketSizeFromPORTSCPortSpeed: Not supported speed");
}

static const char* GetSpeedNameFromPORTSCPortSpeed(uint32_t port_speed) {
  if (port_speed == kPortSpeedLS)
    return "Low-speed";
  if (port_speed == kPortSpeedHS)
    return "High-speed";
  if (port_speed == kPortSpeedSS)
    return "SuperSpeed Gen1 x1";
  Panic("GetSpeedNameFromPORTSCPortSpeed: Not supported speed");
}

void XHCI::HandleEnableSlotCompleted(int slot, int port) {
  PutStringAndHex("Slot enabled. Slot ID", slot);
  PutStringAndHex("  Use RootPort", port);
  // 4.3.3 Device Slot Initialization
  InputContext& ctx = InputContext::Alloc(1 + 2);
  DeviceContext& dctx = ctx.GetDeviceContext();
  dctx.SetRootHubPortNumber(port);
  dctx.SetContextEntries(1);
  // Allocate and initialize the Transfer Ring for the Default Control Endpoint
  constexpr int kNumOfCtrlEPRingEntries = 32;
  TransferRequestBlockRing<kNumOfCtrlEPRingEntries>* ctrl_ep_tring =
      liumos->kernel_heap_allocator
          ->AllocPages<TransferRequestBlockRing<kNumOfCtrlEPRingEntries>*>(
              ByteSizeToPageSize(
                  sizeof(TransferRequestBlockRing<kNumOfCtrlEPRingEntries>)),
              kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  uint64_t ctrl_ep_tring_phys_addr =
      liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(ctrl_ep_tring));
  ctrl_ep_tring->Init(ctrl_ep_tring_phys_addr);

  // Initialize the Input default control Endpoint 0 Context (6.2.3).
  dctx.SetEPType(DeviceContext::kDCIEPContext0, DeviceContext::kEPTypeControl);
  dctx.SetTRDequeuePointer(DeviceContext::kDCIEPContext0,
                           ctrl_ep_tring_phys_addr);
  dctx.SetDequeueCycleState(DeviceContext::kDCIEPContext0, 1);
  dctx.SetErrorCount(DeviceContext::kDCIEPContext0, 3);
  uint32_t portsc = ReadPORTSC(port);
  uint32_t port_speed = GetBits<13, 10, uint32_t, uint32_t>(portsc);
  PutString("  Port Speed: ");
  PutString(GetSpeedNameFromPORTSCPortSpeed(port_speed));
  PutString("\n");
  dctx.SetMaxPacketSize(DeviceContext::kDCIEPContext0,
                        GetMaxPacketSizeFromPORTSCPortSpeed(port_speed));
  DeviceContext& out_device_ctx = DeviceContext::Alloc(1);
  device_context_base_array_[slot] = &out_device_ctx;
  // 8. Issue an Address Device Command for the Device Slot
  volatile BasicTRB& trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  trb.data = v2p<InputContext*, uint64_t>(&ctx);
  trb.option = 0;
  trb.control = (kTRBTypeAddressDeviceCommand << 10) | (slot << 24);
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
}

void XHCI::HandleAddressDeviceCompleted(int slot) {
  PutStringAndHex("Address Device Completed. Slot ID", slot);
}

void XHCI::PrintPortSC() {
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    uint32_t portsc = ReadPORTSC(slot);
    PutStringAndHex("Port", slot);
    PutStringAndHex("PORTSC", portsc);
    PutString("PortSC: ");
    PutHex64ZeroFilled(portsc);
    PutString(" (");
    PutString((portsc & (1 << 9)) ? "1" : "0");
    PutString((portsc & (1 << 0)) ? "1" : "0");
    PutString((portsc & (1 << 1)) ? "1" : "0");
    PutString((portsc & (1 << 4)) ? "1" : "0");
    PutString(") ");
    // 7.2.2.1.1 Default USB Speed ID Mapping
    PutString("Speed=0x");
    PutHex64(GetBits(portsc, 13, 10));

    PutString("\n");
  }
}
void XHCI::PrintUSBSTS() {
  const uint32_t status = op_regs_->status;
  PutStringAndHex("USBSTS", status);
  PutString((status & kUSBSTSBitHCHalted) ? " Halted\n" : " Runnning\n");
  if (status & kUSBSTSBitHCError) {
    PutString(" HC Error Detected!\n");
  }
}

void XHCI::PollEvents() {
  if (!primary_event_ring_)
    return;
  if (primary_event_ring_->HasNextEvent()) {
    BasicTRB& e = primary_event_ring_->PeekEvent();
    uint8_t type = e.GetTRBType();

    switch (type) {
      case kTRBTypeCommandCompletionEvent:
        PutString("CommandCompletionEvent\n");
        PutStringAndHex("  CompletionCode", GetBits(e.option, 31, 24));
        {
          BasicTRB& cmd_trb = cmd_ring_->GetEntryFromPhysAddr(e.data);
          PutStringAndHex("  CommandType", cmd_trb.GetTRBType());
          if (cmd_trb.GetTRBType() == kTRBTypeEnableSlotCommand) {
            uint64_t cmd_trb_phys_addr = e.data;
            auto it = slot_request_for_port.find(cmd_trb_phys_addr);
            assert(it != slot_request_for_port.end());
            HandleEnableSlotCompleted(GetBits(e.control, 31, 24), it->second);
            slot_request_for_port.erase(it);
            break;
          }
          if (cmd_trb.GetTRBType() == kTRBTypeAddressDeviceCommand) {
            HandleAddressDeviceCompleted(GetBits(e.control, 31, 24));
            break;
          }
        }
        break;
      case kTRBTypePortStatusChangeEvent:
        PutString("PortStatusChangeEvent\n");
        PutStringAndHex("  Port ID", GetBits(e.data, 31, 24));
        PutStringAndHex("  CompletionCode", GetBits(e.option, 31, 24));
        HandlePortStatusChange(static_cast<int>(GetBits(e.data, 31, 24)));
        PutString("kTRBTypePortStatusChangeEvent end\n");
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
