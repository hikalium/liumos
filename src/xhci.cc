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

constexpr uint32_t kTRBTypeSetupStage = 2;
constexpr uint32_t kTRBTypeDataStage = 3;
constexpr uint32_t kTRBTypeStatusStage = 4;

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
    const int erst_size = sizeof(XHCI::EventRingSegmentTableEntry[1]);
    erst_ = liumos->kernel_heap_allocator
                ->AllocPages<volatile XHCI::EventRingSegmentTableEntry*>(
                    ByteSizeToPageSize(erst_size), kPageAttrMemMappedIO);
    const size_t trbs_size =
        sizeof(XHCI::CommandCompletionEventTRB) * num_of_trb_;
    trbs_ = liumos->kernel_heap_allocator->AllocPages<BasicTRB*>(
        ByteSizeToPageSize(trbs_size), kPageAttrMemMappedIO);
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
void XHCI::NotifyDeviceContextDoorbell(int slot, int dci) {
  assert(1 <= slot && slot <= 255);
  db_regs_[slot] = dci;
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
  max_slots_ = GetBits<31, 24, uint8_t>(kHCSPARAMS1);
  max_intrs_ = GetBits<18, 8, uint8_t>(kHCSPARAMS1);
  max_ports_ = GetBits<7, 0, uint8_t>(kHCSPARAMS1);

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
    PutHex64(GetBits<8, 5, uint32_t>(portsc));
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
    const int num_of_ctx = max_dci + 1;
    size_t size = 0x20 * num_of_ctx;
    DeviceContext* ctx =
        liumos->kernel_heap_allocator->AllocPages<DeviceContext*>(
            ByteSizeToPageSize(size), kPageAttrMemMappedIO);
    new (ctx) DeviceContext(max_dci);
    return *ctx;
  }
  void SetContextEntries(uint32_t num_of_ent) {
    slot_ctx[0] = CombineFieldBits<31, 27>(slot_ctx[0], num_of_ent);
  }
  void SetRootHubPortNumber(uint32_t root_port_num) {
    slot_ctx[1] = CombineFieldBits<23, 16>(slot_ctx[1], root_port_num);
  }
  void SetEPType(int dci, uint32_t type) {
    endpoint_ctx[dci][1] = CombineFieldBits<5, 3>(endpoint_ctx[dci][1], type);
  }
  void SetTRDequeuePointer(int dci, uint64_t tr_deq_ptr) {
    endpoint_ctx[dci][2] = CombineFieldBits<31, 4>(
        endpoint_ctx[dci][2], static_cast<uint32_t>(tr_deq_ptr >> 4));
    endpoint_ctx[dci][3] = static_cast<uint32_t>(tr_deq_ptr >> 32);
  }
  void SetDequeueCycleState(int dci, uint32_t dcs) {
    endpoint_ctx[dci][2] = CombineFieldBits<0, 0>(endpoint_ctx[dci][2], dcs);
  }
  void SetErrorCount(int dci, uint32_t c_err) {
    endpoint_ctx[dci][1] = CombineFieldBits<2, 1>(endpoint_ctx[dci][1], c_err);
  }
  void SetMaxPacketSize(int dci, uint32_t max_packet_size) {
    endpoint_ctx[dci][1] =
        CombineFieldBits<31, 16>(endpoint_ctx[dci][1], max_packet_size);
  }
  uint8_t GetSlotState() { return GetBits<31, 27, uint8_t>(slot_ctx[3]); }
  uint8_t GetEPState(int dci) {
    return GetBits<2, 0, uint8_t>(endpoint_ctx[dci][0]);
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
  static InputContext& Alloc(int max_dci) {
    static_assert(offsetof(InputContext, device_ctx_) == 0x20);
    assert(0 <= max_dci && max_dci <= 31);
    const int num_of_ctx = max_dci + 2;
    size_t size = 0x20 * num_of_ctx;
    InputContext& ctx =
        *liumos->kernel_heap_allocator->AllocPages<InputContext*>(
            ByteSizeToPageSize(size), kPageAttrMemMappedIO);
    bzero(&ctx, size);
    if (num_of_ctx > 1) {
      volatile uint32_t& add_ctx_flags = ctx.input_ctrl_ctx_[1];
      add_ctx_flags = (1 << (num_of_ctx - 1)) - 1;
    }
    new (&ctx.device_ctx_) DeviceContext(max_dci);
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

constexpr int kNumOfCtrlEPRingEntries = 32;
TransferRequestBlockRing<kNumOfCtrlEPRingEntries>* ctrl_ep_trings[256];
XHCI::DeviceContext* device_contexts[256];

void XHCI::HandleEnableSlotCompleted(int slot, int port) {
  PutStringAndHex("Slot enabled. Slot ID", slot);
  PutStringAndHex("  Use RootPort", port);
  // 4.3.3 Device Slot Initialization
  InputContext& ctx = InputContext::Alloc(1);
  DeviceContext& dctx = ctx.GetDeviceContext();
  // 3. Initialize the Input Slot Context data structure (6.2.2)
  dctx.SetRootHubPortNumber(port);
  dctx.SetContextEntries(1);
  // 4. Allocate and initialize the Transfer Ring for the Default Control
  // Endpoint
  TransferRequestBlockRing<kNumOfCtrlEPRingEntries>* ctrl_ep_tring =
      AllocMemoryForMappedIO<
          TransferRequestBlockRing<kNumOfCtrlEPRingEntries>*>(
          sizeof(TransferRequestBlockRing<kNumOfCtrlEPRingEntries>));
  uint64_t ctrl_ep_tring_phys_addr =
      v2p<TransferRequestBlockRing<kNumOfCtrlEPRingEntries>*, uint64_t>(
          ctrl_ep_tring);
  ctrl_ep_tring->Init(ctrl_ep_tring_phys_addr);
  // 5. Initialize the Input default control Endpoint 0 Context (6.2.3)
  uint32_t portsc = ReadPORTSC(port);
  uint32_t port_speed = GetBits<13, 10, uint32_t, uint32_t>(portsc);
  PutString("  Port Speed: ");
  PutString(GetSpeedNameFromPORTSCPortSpeed(port_speed));
  PutString("\n");
  dctx.SetEPType(DeviceContext::kDCIEPContext0, DeviceContext::kEPTypeControl);
  dctx.SetTRDequeuePointer(DeviceContext::kDCIEPContext0,
                           ctrl_ep_tring_phys_addr);
  dctx.SetDequeueCycleState(DeviceContext::kDCIEPContext0, 1);
  dctx.SetErrorCount(DeviceContext::kDCIEPContext0, 3);
  dctx.SetMaxPacketSize(DeviceContext::kDCIEPContext0,
                        GetMaxPacketSizeFromPORTSCPortSpeed(port_speed));
  DeviceContext& out_device_ctx = DeviceContext::Alloc(1);
  device_context_base_array_[slot] =
      v2p<DeviceContext*, DeviceContext*>(&out_device_ctx);
  device_contexts[slot] = &out_device_ctx;
  PutStringAndHex("  Slot State", device_contexts[slot]->GetSlotState());
  PutStringAndHex("  EP0  State", device_contexts[slot]->GetEPState(0));
  ctrl_ep_trings[slot] = ctrl_ep_tring;
  // 8. Issue an Address Device Command for the Device Slot
  volatile BasicTRB& trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  trb.data = v2p<InputContext*, uint64_t>(&ctx);
  trb.option = 0;
  trb.control = (kTRBTypeAddressDeviceCommand << 10) | (slot << 24);
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
}

struct SetupStageTRB {
  volatile uint8_t request_type;
  volatile uint8_t request;
  volatile uint16_t value;
  volatile uint16_t index;
  volatile uint16_t length;
  volatile uint32_t option;
  volatile uint32_t control;

  static constexpr uint8_t kReqTypeBitDirectionDeviceToHost = 1 << 7;

  static constexpr uint8_t kReqGetDescriptor = 6;

  static constexpr uint32_t kTransferTypeInDataStage = 3;
};
static_assert(sizeof(SetupStageTRB) == 16);

struct DataStageTRB {
  volatile uint64_t buf;
  volatile uint32_t option;
  volatile uint32_t control;
};
static_assert(sizeof(DataStageTRB) == 16);

struct StatusStageTRB {
  volatile uint64_t reserved;
  volatile uint32_t option;
  volatile uint32_t control;
};
static_assert(sizeof(StatusStageTRB) == 16);

static void ConfigureSetupStageTRBForGetDescriptorRequest(
    struct SetupStageTRB& trb,
    uint32_t slot,
    uint8_t desc_type,
    uint16_t desc_length) {
  trb.request_type = SetupStageTRB::kReqTypeBitDirectionDeviceToHost;
  trb.request = SetupStageTRB::kReqGetDescriptor;
  trb.value = desc_type;
  trb.index = 0;
  trb.length = desc_length;
  trb.option = 8;
  trb.control = (1 << 6) | (kTRBTypeSetupStage << 10) | (slot << 24) |
                (SetupStageTRB::kTransferTypeInDataStage << 16);
}

static void ConfigureDataStageTRBForInput(struct DataStageTRB& trb,
                                          uint64_t ptr,
                                          uint32_t length) {
  trb.buf = ptr;
  trb.option = length;
  trb.control = (1 << 16) | (kTRBTypeDataStage << 10) | (1 << 5);
}

static void ConfigureStatusStageTRBForInput(struct StatusStageTRB& trb) {
  trb.reserved = 0;
  trb.option = 0;
  trb.control = (1 << 16) | (kTRBTypeStatusStage << 10) | (1 << 5);
}

void XHCI::HandleAddressDeviceCompleted(int slot) {
  PutStringAndHex("Address Device Completed. Slot ID", slot);
  PutStringAndHex("  Slot State", device_contexts[slot]->GetSlotState());
  PutStringAndHex("  EP0  State", device_contexts[slot]->GetEPState(0));

  constexpr int buf_size = 64;
  uint8_t* buf = liumos->kernel_heap_allocator->AllocPages<uint8_t*>(
      ByteSizeToPageSize(buf_size),
      kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
  uint64_t buf_phys_addr = v2p<uint8_t*, uint64_t>(buf);

  assert(ctrl_ep_trings[slot]);
  auto& tring = *ctrl_ep_trings[slot];
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  constexpr uint8_t kDescriptorTypeConfiguration = 2;
  ConfigureSetupStageTRBForGetDescriptorRequest(
      setup, slot, kDescriptorTypeConfiguration, buf_size);

  tring.Push();
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  ConfigureDataStageTRBForInput(data, buf_phys_addr, buf_size);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status);
  tring.Push();

  // NotifyDeviceContextDoorbell(slot, 1);

  PutStringAndHex("  Slot State", device_contexts[slot]->GetSlotState());
  PutStringAndHex("  EP0  State", device_contexts[slot]->GetEPState(0));
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
    PutHex64(GetBits<13, 10, uint32_t>(portsc));

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
        PutStringAndHex("  CompletionCode", e.GetCompletionCode());
        if (e.GetCompletionCode() == 5) {
          PutString("  = TRBError\n");
        }
        if (e.GetCompletionCode() == 1) {
          PutString("  = Success\n");
        }
        {
          BasicTRB& cmd_trb = cmd_ring_->GetEntryFromPhysAddr(e.data);
          PutStringAndHex("  CommandType", cmd_trb.GetTRBType());
          if (cmd_trb.GetTRBType() == kTRBTypeEnableSlotCommand) {
            uint64_t cmd_trb_phys_addr = e.data;
            auto it = slot_request_for_port.find(cmd_trb_phys_addr);
            assert(it != slot_request_for_port.end());
            HandleEnableSlotCompleted(e.GetSlotID(), it->second);
            slot_request_for_port.erase(it);
            break;
          }
          if (cmd_trb.GetTRBType() == kTRBTypeAddressDeviceCommand) {
            HandleAddressDeviceCompleted(e.GetSlotID());
            break;
          }
        }
        break;
      case kTRBTypePortStatusChangeEvent:
        PutString("PortStatusChangeEvent\n");
        PutStringAndHex("  Port ID", GetBits<31, 24, uint64_t>(e.data));
        PutStringAndHex("  CompletionCode", e.GetCompletionCode());
        HandlePortStatusChange(
            static_cast<int>(GetBits<31, 24, uint64_t>(e.data)));
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
