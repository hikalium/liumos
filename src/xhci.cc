#include "xhci.h"

#include <cstdio>
#include <utility>

#include "kernel.h"
#include "keyid.h"
#include "liumos.h"

namespace XHCI {

Controller* Controller::xhci_;

void Controller::ResetHostController() {
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

class Controller::EventRing {
 public:
  EventRing(int num_of_trb, InterrupterRegisterSet& irs)
      : cycle_state_(1), index_(0), num_of_trb_(num_of_trb), irs_(irs) {
    const int erst_size = sizeof(Controller::EventRingSegmentTableEntry[1]);
    erst_ = AllocMemoryForMappedIO<
        volatile Controller::EventRingSegmentTableEntry*>(erst_size);
    const size_t trbs_size =
        sizeof(Controller::CommandCompletionEventTRB) * num_of_trb_;
    trbs_ = AllocMemoryForMappedIO<BasicTRB*>(trbs_size);
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
  uint64_t GetERSTPhysAddr() { return v2p(reinterpret_cast<uint64_t>(erst_)); }
  uint64_t GetTRBSPhysAddr() { return v2p(reinterpret_cast<uint64_t>(trbs_)); }
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
  volatile Controller::EventRingSegmentTableEntry* erst_;
  BasicTRB* trbs_;
  InterrupterRegisterSet& irs_;
};

void Controller::InitPrimaryInterrupter() {
  // 5.5.2 Interrupter Register Set
  // All registers of the Primary Interrupter shall be initialized
  // before setting the Run/Stop (RS) flag in the USBCMD register to 1.

  class EventRing& primary_event_ring =
      *new EventRing(kNumOfTRBForEventRing, rt_regs_->irs[0]);
  primary_event_ring_ = &primary_event_ring;
};

void Controller::InitSlotsAndContexts() {
  num_of_slots_enabled_ = max_slots_;
  constexpr uint64_t kOPREGConfigMaskMaxSlotsEn = 0b1111'1111;
  op_regs_->config = (op_regs_->config & ~kOPREGConfigMaskMaxSlotsEn) |
                     (num_of_slots_enabled_ & kOPREGConfigMaskMaxSlotsEn);

  device_context_base_array_ =
      AllocMemoryForMappedIO<volatile uint64_t*>(kPageSize);
  for (int i = 0; i <= num_of_slots_enabled_; i++) {
    device_context_base_array_[i] = 0;
  }
  op_regs_->device_ctx_base_addr_array_ptr = v2p(device_context_base_array_);
}

void Controller::InitCommandRing() {
  cmd_ring_ = AllocMemoryForMappedIO<
      TransferRequestBlockRing<kNumOfCmdTRBRingEntries>*>(
      sizeof(TransferRequestBlockRing<kNumOfCmdTRBRingEntries>));
  cmd_ring_phys_addr_ = v2p(cmd_ring_);
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

void Controller::NotifyHostControllerDoorbell() {
  db_regs_[0] = 0;
}
void Controller::NotifyDeviceContextDoorbell(int slot, int dci) {
  assert(1 <= slot && slot <= 255);
  db_regs_[slot] = dci;
}
uint32_t Controller::ReadPORTSC(int slot) {
  return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(op_regs_) +
                                      0x400 + 0x10 * (slot - 1));
}
void Controller::WritePORTSC(int slot, uint32_t data) {
  *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(op_regs_) + 0x400 +
                               0x10 * (slot - 1)) = data;
}

static std::optional<PCI::DeviceLocation> FindXHCIController() {
  for (auto& it : PCI::GetInstance().GetDeviceList()) {
    if (it.first != 0x000D'1B36 && it.first != 0x31A8'8086)
      continue;
    PutString("XHCI Controller Found: ");
    PutString(PCI::GetDeviceName(it.first));
    PutString("\n");
    return it.second;
  }
  PutString("XHCI Controller Not Found\n");
  return {};
}

void Controller::ResetPort(int port) {
  // Make Port Power is not off (PP = 1)
  // See 4.19.1.1 USB2 Root Hub Port
  // Figure 4-25: USB2 Root Hub Port State Machine
  WritePORTSC(port,
              (ReadPORTSC(port) & kPortSCPreserveMask) | kPortSCBitPortPower);
  while (!(ReadPORTSC(port) & kPortSCBitPortPower)) {
    // wait
  }
  WritePORTSC(port,
              (ReadPORTSC(port) & kPortSCPreserveMask) | kPortSCBitPortReset);
  while ((ReadPORTSC(port) & kPortSCBitPortReset)) {
    // wait
  }
  PutStringAndHex("ResetPort: done. port", port);
  PutStringAndHex("  PORTSC", ReadPORTSC(port));
  port_state_[port] = kNeedsSlotAssignment;
  port_is_initializing_[port] = true;
}

void Controller::DisablePort(int port) {
  PutStringAndHex("Disable port", port);
  WritePORTSC(port, 2);
  port_state_[port] = kDisabled;
  port_is_initializing_[port] = false;
}

void Controller::HandlePortStatusChange(int port) {
  uint32_t portsc = ReadPORTSC(port);
  PutStringAndHex("XHCI Port Status Changed", port);
  PutStringAndHex("  Port ID", port);
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
    PutHex64(GetBits<8, 5>(portsc));
    PutString("\n");
  }
}

class Controller::EndpointContext {
 public:
  void SetEPType(uint32_t type) {
    data_[1] = CombineFieldBits<5, 3>(data_[1], type);
  }
  void SetTRDequeuePointer(uint64_t tr_deq_ptr) {
    data_[2] = CombineFieldBits<31, 4>(data_[2],
                                       static_cast<uint32_t>(tr_deq_ptr >> 4));
    data_[3] = static_cast<uint32_t>(tr_deq_ptr >> 32);
  }
  uint64_t GetTRDequeuePointer() {
    return (static_cast<uint64_t>(data_[3]) << 32) | data_[2];
  }
  void SetDequeueCycleState(uint32_t dcs) {
    data_[2] = CombineFieldBits<0, 0>(data_[2], dcs);
  }
  void SetErrorCount(uint32_t c_err) {
    data_[1] = CombineFieldBits<2, 1>(data_[1], c_err);
  }
  void SetMaxPacketSize(uint32_t max_packet_size) {
    data_[1] = CombineFieldBits<31, 16>(data_[1], max_packet_size);
  }
  uint8_t GetEPState() { return GetBits<2, 0>(data_[0]); }
  void DumpEPContext() {
    PutString("EP Context");
    PutStringAndHex("  EP Type", GetBits<5, 3>(data_[1]));
    PutStringAndHex("  Max Packet Size", GetBits<31, 16>(data_[1]));
    PutStringAndHex("  TR Dequeue Pointer",
                    (GetBits<31, 4>(data_[2]) << 4) |
                        (static_cast<uint64_t>(data_[3]) << 32));
    PutStringAndHex("  Dequeue Cycle State", GetBits<0, 0>(data_[2]));
    PutStringAndHex("  Error Count", GetBits<2, 1>(data_[1]));
    for (int i = 0; i < 8; i++) {
      for (int x = 3; x >= 0; x--) {
        PutHex8ZeroFilled(data_[i] >> (x * 8));
      }
      PutString("\n");
    }
  }

 private:
  volatile uint32_t data_[8];
};
static_assert(sizeof(Controller::EndpointContext) == 0x20);

class Controller::DeviceContext {
  friend class InputContext;

 public:
  static constexpr int kDCISlotContext = 0;
  static constexpr int kDCIEPContext0 = 1;
  static constexpr int kDCIEPContext1Out = 2;
  static constexpr int kEPTypeControl = 4;

  uint8_t GetSlotState() { return GetBits<31, 27>(slot_ctx_[3]); }
  void SetContextEntries(uint32_t num_of_ent) {
    slot_ctx_[0] = CombineFieldBits<31, 27>(slot_ctx_[0], num_of_ent);
  }
  void SetPortSpeed(uint32_t port_speed) {
    slot_ctx_[0] = CombineFieldBits<23, 20>(slot_ctx_[0], port_speed);
  }
  void SetRootHubPortNumber(uint32_t root_port_num) {
    slot_ctx_[1] = CombineFieldBits<23, 16>(slot_ctx_[1], root_port_num);
  }
  void DumpSlotContext() {
    PutString("Slot Context:\n");
    PutStringAndHex("  Root Hub Port Number", GetBits<23, 16>(slot_ctx_[1]));
    PutStringAndHex("  Route String", GetBits<19, 0>(slot_ctx_[0]));
    PutStringAndHex("  Context Entries", GetBits<31, 27>(slot_ctx_[0]));
    for (int i = 0; i < 8; i++) {
      for (int x = 3; x >= 0; x--) {
        PutHex8ZeroFilled(slot_ctx_[i] >> (x * 8));
      }
      PutString("\n");
    }
  }
  EndpointContext& GetEndpointContext(int dci) {
    assert(1 <= dci && dci <= 31);
    return endpoint_ctx_[dci - 1];
  }

  static DeviceContext& Alloc(int max_dci) {
    const int num_of_ctx = max_dci + 1;
    size_t size = 0x20 * num_of_ctx;
    DeviceContext* ctx = AllocMemoryForMappedIO<DeviceContext*>(size);
    new (ctx) DeviceContext(max_dci);
    return *ctx;
  }
  DeviceContext(int max_dci) {
    assert(0 <= max_dci && max_dci <= 31);
    bzero(this, sizeof(slot_ctx_) + sizeof(EndpointContext) * max_dci);
  }

 private:
  volatile uint32_t slot_ctx_[8];
  EndpointContext endpoint_ctx_[2 * 15 + 1];
};
static_assert(sizeof(Controller::DeviceContext) == 0x400);

class Controller::InputContext {
 public:
  static InputContext& Alloc(int max_dci) {
    static_assert(offsetof(InputContext, device_ctx_) == 0x20);
    assert(0 <= max_dci && max_dci <= 31);
    const int num_of_ctx = max_dci + 2;
    size_t size = 0x20 * num_of_ctx;
    InputContext& ctx = *AllocMemoryForMappedIO<InputContext*>(size);
    bzero(&ctx, size);
    new (&ctx.device_ctx_) DeviceContext(max_dci);
    return ctx;
  }

  void Clear() { bzero(input_ctrl_ctx_, sizeof(input_ctrl_ctx_)); }

  void SetAddContext(int dci, bool value) {
    volatile uint32_t& add_ctx_flags = input_ctrl_ctx_[1];
    add_ctx_flags =
        (~(1 << dci) & add_ctx_flags) | (static_cast<uint32_t>(value) << dci);
  }

  void DumpInputControlContext() {
    PutString("Input Control Context\n");
    for (int i = 0; i < 8; i++) {
      for (int x = 3; x >= 0; x--) {
        PutHex8ZeroFilled(input_ctrl_ctx_[i] >> (x * 8));
      }
      PutString("\n");
    }
  }

  InputContext() = delete;
  DeviceContext& GetDeviceContext() { return device_ctx_; }

 private:
  volatile uint32_t input_ctrl_ctx_[8];
  DeviceContext device_ctx_;
};
static_assert(sizeof(Controller::InputContext) == 0x420);

constexpr uint32_t kPortSpeedFS = 1;
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
  if (port_speed == kPortSpeedLS || port_speed == kPortSpeedFS)
    return 8;
  if (port_speed == kPortSpeedHS)
    return 64;
  if (port_speed == kPortSpeedSS)
    return 512;
  PutStringAndHex("  requested port_speed", port_speed);
  Panic("GetMaxPacketSizeFromPORTSCPortSpeed: Not supported speed");
}

static const char* GetSpeedNameFromPORTSCPortSpeed(uint32_t port_speed) {
  if (port_speed == kPortSpeedFS)
    return "Full-speed";
  if (port_speed == kPortSpeedLS)
    return "Low-speed";
  if (port_speed == kPortSpeedHS)
    return "High-speed";
  if (port_speed == kPortSpeedSS)
    return "SuperSpeed Gen1 x1";
  PutString("GetSpeedNameFromPORTSCPortSpeed: Not supported speed\n");
  PutStringAndHex("  port_speed", port_speed);
  return nullptr;
}

void Controller::SendAddressDeviceCommand(int slot) {
  auto& slot_info = slot_info_[slot];
  int port = slot_info.port;
  // 4.3.3 Device Slot Initialization
  assert(slot_info.input_ctx);
  InputContext& ctx = *slot_info.input_ctx;
  ctx.Clear();
  ctx.SetAddContext(0, true);
  ctx.SetAddContext(1, true);
  DeviceContext& dctx = ctx.GetDeviceContext();
  // 3. Initialize the Input Slot Context data structure (6.2.2)
  dctx.SetRootHubPortNumber(port);
  dctx.SetContextEntries(1);
  // 4. Allocate and initialize the Transfer Ring for the Default Control
  // Endpoint
  TransferRequestBlockRing<kNumOfCtrlEPRingEntries>* ctrl_ep_tring =
      slot_info.ctrl_ep_tring;
  assert(ctrl_ep_tring);
  uint64_t ctrl_ep_tring_phys_addr = v2p(ctrl_ep_tring);
  ctrl_ep_tring->Init(ctrl_ep_tring_phys_addr);
  // 5. Initialize the Input default control Endpoint 0 Context (6.2.3)
  uint32_t portsc = ReadPORTSC(port);
  uint32_t port_speed = GetBits<13, 10>(portsc);
  dctx.SetPortSpeed(port_speed);
  PutString("  Port Speed: ");
  const char* speed_str = GetSpeedNameFromPORTSCPortSpeed(port_speed);
  if (!speed_str) {
    DisablePort(port);
    return;
  }
  PutString(speed_str);
  PutString("\n");
  EndpointContext& ep0 = dctx.GetEndpointContext(DeviceContext::kDCIEPContext0);
  ep0.SetEPType(DeviceContext::kEPTypeControl);
  ep0.SetTRDequeuePointer(ctrl_ep_tring_phys_addr);
  ep0.SetDequeueCycleState(1);
  ep0.SetErrorCount(3);
  slot_info.max_packet_size = GetMaxPacketSizeFromPORTSCPortSpeed(port_speed);
  ep0.SetMaxPacketSize(slot_info.max_packet_size);

  assert(slot_info.output_ctx);
  DeviceContext& out_device_ctx = *slot_info.output_ctx;
  device_context_base_array_[slot] = v2p(&out_device_ctx);
  // 8. Issue an Address Device Command for the Device Slot
  // 6.2.2.1 Address Device Command Usage
  // The Input Slot Context is considered “valid” by the Address Device Command
  // if: 1) the Route String field defines a valid route string > ok 2) the
  // Speed field identifies the speed of the device > ok 3) the Context Entries
  // field is set to ‘1’ (i.e. Only the Control Endpoint Context is valid), 4)
  // the value of the Root Hub Port Number field is between 1 and MaxPorts, 5)
  // if the device is LS/FS and connected through a HS hub, then the Parent Hub
  // Slot ID field references a Device Slot that is assigned to the HS hub, the
  // MTT field indicates whether the HS hub supports Multi-TTs, and the Parent
  // Port Number field indicates the correct Parent Port Number on the HS hub,
  // else these fields are cleared to ‘0’, 6) the Interrupter Target field set
  // to a valid value, and 7) all other fields are cleared to ‘0’.
  volatile BasicTRB& trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  trb.data = v2p(&ctx);
  trb.option = 0;
  trb.control = (BasicTRB::kTRBTypeAddressDeviceCommand << 10) | (slot << 24);
  PutStringAndHex("AddressDeviceCommand Enqueued to",
                  cmd_ring_->GetNextEnqueueIndex());
  PutStringAndHex("  phys addr", v2p(&trb));
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
  PrintUSBSTS();
  slot_info.state = SlotInfo::kWaitingForSecondAddressDeviceCommandCompletion;
}

static void SetConfigureEndpointCommandTRB(
    volatile BasicTRB& trb,
    int slot,
    Controller::InputContext& input_ctx) {
  // 4.6.6 Configure Endpoint
  //   If the Drop Context flag is ‘0’ and the Add Context flag is ‘1’, the xHC
  //   shall...
  // 6.4.3.5 Configure Endpoint Command TRB
  constexpr uint32_t kTRBTypeConfigureEndpointCommand = 12;
  const uint64_t input_ctx_paddr = v2p(&input_ctx);
  assert((input_ctx_paddr & 0b1111) == 0);
  trb.data = input_ctx_paddr;
  trb.option = 0;
  trb.control = (kTRBTypeConfigureEndpointCommand << 10) | (slot << 24);
  trb.PrintHex();
}

void Controller::SendConfigureEndpointCommand(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.input_ctx);

  InputContext& ctx = *slot_info.input_ctx;
  ctx.Clear();
  ctx.SetAddContext(DeviceContext::kDCIEPContext1Out, true);

  DeviceContext& dctx = ctx.GetDeviceContext();
  dctx.SetRootHubPortNumber(slot_info.port);
  dctx.SetContextEntries(1);
  /*
     [xHCI] 6.2.3.2 Configure Endpoint Command Usage
  An Input Endpoint Context is considered “valid” by the Configure Endpoint
  Command if the Add Context flag is ‘1’ and: 1) the values of the Max Packet
  Size, Max Burst Size, and the Interval are considered within range for
  endpoint type and the speed of the device, 2) if MaxPStreams > 0, then the TR
  Dequeue Pointer field points to an array of valid Stream Contexts, or if
  MaxPStreams = 0, then the TR Dequeue Pointer field points to a Transfer Ring,
    3) the EP State field = Disabled, and 4) all other fields are within their
  valid range of values.
  */

  volatile BasicTRB& trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  SetConfigureEndpointCommandTRB(trb, slot, ctx);
  PutStringAndHex("ConfigureEndpointCommand Enqueued to",
                  cmd_ring_->GetNextEnqueueIndex());
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
  PrintUSBSTS();
}

void Controller::HandleEnableSlotCompleted(int slot, int port) {
  port_state_[port] = kSlotAssigned;

  auto& slot_info = slot_info_[slot];
  slot_info.port = port;
  PutStringAndHex("EnableSlotCommand completed.. Slot ID", slot);
  PutStringAndHex("  With RootPort ID", port);

  slot_info.input_ctx = &InputContext::Alloc(DeviceContext::kDCIEPContext1Out);
  new (slot_info.output_ctx) DeviceContext(DeviceContext::kDCIEPContext1Out);
  slot_info.ctrl_ep_tring =
      AllocMemoryForMappedIO<CtrlEPTRing*>(sizeof(CtrlEPTRing));
  slot_info.int_ep_tring =
      AllocMemoryForMappedIO<IntEPTRing*>(sizeof(IntEPTRing));

  SendAddressDeviceCommand(slot);
}

static void PutDataStageTD(XHCI::Controller::CtrlEPTRing& tring,
                           uint64_t buf_phys_addr,
                           int size,
                           bool is_data_direction_in) {
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  data.buf = buf_phys_addr;
  data.option = size;
  data.SetControl(is_data_direction_in, false, true);
  tring.Push();
}

void Controller::RequestDeviceDescriptor(int slot,
                                         SlotInfo::SlotState next_state) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  int desc_size = sizeof(DeviceDescriptor);
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.SetParams(SetupStageTRB::kReqTypeBitDirectionDeviceToHost,
                  SetupStageTRB::kReqGetDescriptor,
                  (static_cast<uint16_t>(kDescriptorTypeDevice) << 8) | 0, 0,
                  desc_size, false);
  tring.Push();

  PutDataStageTD(tring, v2p(descriptor_buffers_[slot]), desc_size, true);

  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(false, false);
  tring.Push();

  slot_info_[slot].state = next_state;
  NotifyDeviceContextDoorbell(slot, 1);
  PutStringAndHex("DeviceDescriptor requested for slot", slot);
}

void Controller::RequestConfigDescriptor(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  int desc_size = kSizeOfDescriptorBuffer;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.SetParams(SetupStageTRB::kReqTypeBitDirectionDeviceToHost,
                  SetupStageTRB::kReqGetDescriptor,
                  (static_cast<uint16_t>(kDescriptorTypeConfig) << 8) | 0, 0,
                  desc_size, false);
  tring.Push();
  PutDataStageTD(tring, v2p(descriptor_buffers_[slot]), desc_size, true);
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(false, false);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kCheckingConfigDescriptor;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::SetConfig(int slot, uint8_t config_value) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.SetParams(0, SetupStageTRB::kReqSetConfiguration, config_value, 0, 0,
                  false);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(true, true);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kSettingConfiguration;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::SetHIDBootProtocol(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.SetParams(0b00100001, 0x0B /*SET_PROTOCOL*/, 0 /*Boot Protocol*/,
                  0 /* interface */, 0, false);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(true, true);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kSettingBootProtocol;
  NotifyDeviceContextDoorbell(slot, 1);
}
void Controller::GetHIDProtocol(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  int desc_size = 1;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  // [HID] 7.2.5 Get_Protocol Request
  setup.SetParams(0b10100001, 0x01 /*GET_REPORT*/, 0, 0, 1, false);
  setup.Print();
  tring.Push();
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  PutDataStageTD(tring, v2p(descriptor_buffers_[slot]), desc_size, true);
  data.Print();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(false, false);
  status.Print();
  tring.Push();

  slot_info_[slot].state = SlotInfo::kCheckingProtocol;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::GetHIDReport(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  int desc_size = 8;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  // [HID] 7.2.1 Get_Report Request
  setup.SetParams(0b10100001, 0x01 /*GET_REPORT*/, 0x2201 /*Report|Input*/, 0,
                  desc_size, false);
  tring.Push();
  PutDataStageTD(tring, v2p(descriptor_buffers_[slot]), desc_size, true);
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  status.SetParams(false, false);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kGettingReport;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::HandleAddressDeviceCompleted(int slot) {
  PutStringAndHex("Address Device Completed. Slot ID", slot);
  uint8_t* buf = AllocMemoryForMappedIO<uint8_t*>(kSizeOfDescriptorBuffer);
  descriptor_buffers_[slot] = buf;
  port_is_initializing_[slot_info_[slot].port] = false;
  RequestDeviceDescriptor(slot, SlotInfo::kCheckingIfHIDClass);
}

void Controller::PressKey(int hid_idx, uint8_t) {
  static uint16_t mapping[256] = {0,
                                  0,
                                  0,
                                  0,
                                  'a',
                                  'b',
                                  'c',
                                  'd',
                                  'e',
                                  'f',
                                  'g',
                                  'h',
                                  'i',
                                  'j',
                                  'k',
                                  'l',
                                  'm',
                                  'n',
                                  'o',
                                  'p',
                                  'q',
                                  'r',
                                  's',
                                  't',
                                  'u',
                                  'v',
                                  'w',
                                  'x',
                                  'y',
                                  'z',
                                  '1',
                                  '2',
                                  '3',
                                  '4',
                                  '5',
                                  '6',
                                  '7',
                                  '8',
                                  '9',
                                  '0',
                                  '\n',
                                  0,
                                  KeyID::kBackspace,
                                  0,
                                  ' ',
                                  '-',
                                  '=',
                                  '[',
                                  ']',
                                  '\\',
                                  0,
                                  ';',
                                  '\'',
                                  '~',
                                  ',',
                                  '.'};
  if (0 < hid_idx && hid_idx < 256)
    keyid_buffer_.Push(mapping[hid_idx]);
}

void Controller::HandleKeyInput(int slot, uint8_t data[8]) {
  uint8_t new_key_bits[33] = {};
  for (int i = 2; i < 8; i++) {
    if (!data[i])
      continue;
    new_key_bits[data[i] >> 3] |= (1 << (data[i] & 7));
  }
  new_key_bits[32] = data[0];

  for (int i = 0; i < 33; i++) {
    uint8_t diff_make = new_key_bits[i] & ~key_buffers_[slot][i];
    for (int k = 0; k < 8; k++) {
      if ((diff_make >> k) & 1) {
        PressKey(i * 8 + k, data[0]);
      }
    }
    key_buffers_[slot][i] = new_key_bits[i];
  }
}

uint16_t Controller::ReadKeyboardInput() {
  if (keyid_buffer_.IsEmpty())
    return 0;
  return keyid_buffer_.Pop();
}

void Controller::HandleTransferEvent(BasicTRB& e) {
  const int slot = e.GetSlotID();
  if (!e.IsCompletedWithSuccess() && !e.IsCompletedWithShortPacket()) {
    PutString("TransferEvent\n");
    PutStringAndHex("  Slot ID", slot);
    PutStringAndHex("  CompletionCode", e.GetCompletionCode());
    if (e.GetCompletionCode() == 6) {
      PutString("  = Stall Error\n");
    }
    return;
  }
  switch (slot_info_[slot].state) {
    case SlotInfo::kCheckingIfHIDClass: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      DeviceDescriptor& device_desc =
          *reinterpret_cast<DeviceDescriptor*>(descriptor_buffers_[slot]);
      PutString("DeviceDescriptor\n");
      PutStringAndHex("  length", device_desc.length);
      PutStringAndHex("  type", device_desc.type);
      PutStringAndHex("  device_class", device_desc.device_class);
      PutStringAndHex("  device_subclass", device_desc.device_subclass);
      PutStringAndHex("  device_protocol", device_desc.device_protocol);
      PutStringAndHex("  max_packet_size", device_desc.max_packet_size);
      PutStringAndHex("  num_of_config", device_desc.num_of_config);
      if (device_desc.device_class != 0) {
        PutStringAndHex("  Not supported device class",
                        device_desc.device_class);
        slot_info_[slot].state = SlotInfo::kNotSupportedDevice;
        return;
      }
      PutString("  This is an HID Class Device\n");
      RequestConfigDescriptor(slot);
    } break;
    case SlotInfo::kCheckingConfigDescriptor: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      ConfigDescriptor& config_desc =
          *reinterpret_cast<ConfigDescriptor*>(descriptor_buffers_[slot]);
      PutString("ConfigurationDescriptor\n");
      for (int i = 0; i < kSizeOfDescriptorBuffer - e.GetTransferSizeResidue();
           i++) {
        PutHex8ZeroFilled(descriptor_buffers_[slot][i]);
        if ((i & 0b1111) == 0b1111)
          PutChar('\n');
        else
          PutChar(' ');
      }
      PutChar('\n');
      PutStringAndHex("  total length", config_desc.total_length);
      assert(config_desc.total_length <= kSizeOfDescriptorBuffer);
      PutStringAndHex("  Num of Interfaces", config_desc.num_of_interfaces);
      int ofs = config_desc.length;
      InterfaceDescriptor* boot_interface_desc = nullptr;
      EndpointDescriptor* boot_endpoint_desc = nullptr;
      while (ofs < config_desc.total_length) {
        const uint8_t length =
            RefWithOffset<uint8_t*>(descriptor_buffers_[slot], ofs)[0];
        const uint8_t type =
            RefWithOffset<uint8_t*>(descriptor_buffers_[slot], ofs)[1];
        PutStringAndHex("Descriptor type", type);
        PutStringAndHex("Descriptor length", length);
        if (type == kDescriptorTypeInterface) {
          if (boot_interface_desc && !boot_endpoint_desc) {
            boot_interface_desc = nullptr;
          }
          InterfaceDescriptor& interface_desc =
              *RefWithOffset<InterfaceDescriptor*>(descriptor_buffers_[slot],
                                                   ofs);
          PutStringAndHex("Interface #       ",
                          interface_desc.interface_number);
          PutStringAndHex("Num of endpoints", interface_desc.num_of_endpoints);
          {
            char s[128];
            snprintf(s, sizeof(s),
                     "  Class=0x%02X SubClass=0x%02X Protocol=0x%02X\n",
                     interface_desc.interface_class,
                     interface_desc.interface_subclass,
                     interface_desc.interface_protocol);
            PutString(s);
          }
          if (interface_desc.interface_class ==
                  InterfaceDescriptor::kClassHID &&
              interface_desc.interface_subclass ==
                  InterfaceDescriptor::kSubClassSupportBootProtocol &&
              interface_desc.interface_protocol ==
                  InterfaceDescriptor::kProtocolKeyboard) {
            boot_interface_desc = &interface_desc;
          }
        } else if (type == kDescriptorTypeEndpoint) {
          if (boot_interface_desc && !boot_endpoint_desc) {
            boot_endpoint_desc = RefWithOffset<EndpointDescriptor*>(
                descriptor_buffers_[slot], ofs);
          }
        }
        ofs += length;
      }
      if (boot_interface_desc && boot_endpoint_desc) {
        PutStringAndHex(
            "Found USB HID Keyboard (with boot protocol). Interface#",
            boot_interface_desc->interface_number);
        PutStringAndHex("  EP address", boot_endpoint_desc->endpoint_address);
        PutStringAndHex("  EP attr", boot_endpoint_desc->attributes);
        PutStringAndHex("  Max Packet Size",
                        boot_endpoint_desc->max_packet_size);
        PutStringAndHex("  Interval", boot_endpoint_desc->interval_ms);
        SetConfig(slot, config_desc.config_value);
        return;
      }
      PutString("No supported interface found\n");
      slot_info_[slot].state = SlotInfo::kNotSupportedDevice;
    } break;
    case SlotInfo::kSettingConfiguration:
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      PutString("Configuration done\n");
      SetHIDBootProtocol(slot);
      break;
    case SlotInfo::kSettingBootProtocol:
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      PutString("Setting Boot Protocol done\n");
      PutString("Start running USB Keyboard\n");
      bzero(key_buffers_[slot], sizeof(key_buffers_[0]));
      SendConfigureEndpointCommand(slot);
      // GetHIDReport(slot);
      break;
    case SlotInfo::kCheckingProtocol: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      PutString("Checking Protocol Received Data:\n");
      for (int i = 0; i < 1; i++) {
        PutHex8ZeroFilled(descriptor_buffers_[slot][i]);
        if ((i & 0b1111) == 0b1111)
          PutChar('\n');
        else
          PutChar(' ');
      }
      PutChar('\n');
      // slot_info_[slot].state = SlotInfo::kNotSupportedDevice;
    } break;
    case SlotInfo::kGettingReport: {
      assert(e.GetTransferSizeResidue() == 0);
      HandleKeyInput(slot, descriptor_buffers_[slot]);
      // GetHIDReport(slot);
    } break;
    default: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      PutStringAndHex("  Transfered Size", e.GetTransferSizeResidue());
      /*
      for (int i = 0; i < size; i++) {
        PutHex8ZeroFilled(descriptor_buffers_[slot][i]);
        if ((i & 0b1111) == 0b1111)
          PutChar('\n');
        else
          PutChar(' ');
      }
      PutChar('\n');
      */
      PutStringAndHex("Unexpected Transfer Event In Slot State",
                      slot_info_[slot].state);
    }
      return;
  }
}

void Controller::PrintPortSC() {
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
    PutHex64(GetBits<13, 10>(portsc));

    PutString("\n");
  }
}
void Controller::PrintUSBSTS() {
  const uint32_t status = op_regs_->status;
  PutStringAndHex("USBSTS", status);
  PutString((status & kUSBSTSBitHCHalted) ? "  Halted\n" : "  Runnning\n");
  if (status & kUSBSTSBitHCError) {
    PutString("  Host Controller Error Detected!\n");
  }
  if (status & kUSBSTSBitHSError) {
    PutString("  Host System Error Detected!\n");
  }
}

void Controller::CheckPortAndInitiateProcess() {
  for (int i = 0; i < kMaxNumOfPorts; i++) {
    if (port_state_[i] == kNeedsSlotAssignment) {
      port_state_[i] = kWaitingForSlotAssignment;
      // 4.3.2 Device Slot Assignment
      // 4.6.3 Enable Slot
      PrintUSBSTS();
      PutStringAndHex("Send EnableSlotCommand for port", i);
      uint32_t portsc = ReadPORTSC(i);
      PutStringAndHex("  PORTSC", portsc);
      BasicTRB& enable_slot_trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
      slot_request_for_port_.insert({v2p(&enable_slot_trb), i});
      enable_slot_trb.data = 0;
      enable_slot_trb.option = 0;
      enable_slot_trb.control = (BasicTRB::kTRBTypeEnableSlotCommand << 10);
      cmd_ring_->Push();
      NotifyHostControllerDoorbell();
      return;
    }
  }
  for (int i = 0; i < kMaxNumOfPorts; i++) {
    if (port_is_initializing_[i])
      return;
  }
  for (int i = 0; i < kMaxNumOfPorts; i++) {
    if (port_state_[i] == kAttachedUSB2) {
      ResetPort(i);
      return;
    }
  }
}

void Controller::PollEvents() {
  if (!initialized_)
    return;
  static int counter = 0;
  if (controller_reset_requested_) {
    Init();
    return;
  }
  CheckPortAndInitiateProcess();
  if (!primary_event_ring_)
    return;
  counter++;
  if (counter > 1000) {
    counter = 0;
    if (op_regs_->status & kUSBSTSBitHCHalted) {
      PrintUSBSTS();
    }
  }
  if (primary_event_ring_->HasNextEvent()) {
    BasicTRB& e = primary_event_ring_->PeekEvent();
    uint8_t type = e.GetTRBType();

    switch (type) {
      case BasicTRB::kTRBTypeCommandCompletionEvent:
        if (e.IsCompletedWithSuccess()) {
          BasicTRB& cmd_trb = cmd_ring_->GetEntryFromPhysAddr(e.data);
          if (cmd_trb.GetTRBType() == BasicTRB::kTRBTypeEnableSlotCommand) {
            uint64_t cmd_trb_phys_addr = e.data;
            auto it = slot_request_for_port_.find(cmd_trb_phys_addr);
            assert(it != slot_request_for_port_.end());
            HandleEnableSlotCompleted(e.GetSlotID(), it->second);
            slot_request_for_port_.erase(it);
            break;
          }
          if (cmd_trb.GetTRBType() == BasicTRB::kTRBTypeAddressDeviceCommand) {
            HandleAddressDeviceCompleted(e.GetSlotID());
            break;
          }
          PutStringAndHex("  Not Handled Completion Event(Success)",
                          cmd_trb.GetTRBType());
          break;
        }
        PutString("CommandCompletionEvent\n");
        e.PrintCompletionCode();
        PutStringAndHex("  data", e.data);
        DisablePort(slot_info_[e.GetSlotID()].port);
        {
          const uint32_t status = op_regs_->status;
          if (status & kUSBSTSBitHCError) {
            PutString(" HC Error Detected! Request resetting controller...\n");
            controller_reset_requested_ = true;
          }
        }
        break;
      case BasicTRB::kTRBTypePortStatusChangeEvent:
        if (e.IsCompletedWithSuccess()) {
          HandlePortStatusChange(static_cast<int>(GetBits<31, 24>(e.data)));
          break;
        }
        PutString("PortStatusChangeEvent\n");
        e.PrintCompletionCode();
        PutStringAndHex("  Slot ID", GetBits<31, 24>(e.data));
        break;
      case BasicTRB::kTRBTypeTransferEvent:
        HandleTransferEvent(e);
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
  for (int port = 1; port <= max_ports_; port++) {
    uint32_t portsc = ReadPORTSC(port);
    if (port_state_[port] == kDisconnected &&
        (portsc & kPortSCBitCurrentConnectStatus)) {
      port_state_[port] = kAttached;
      PutStringAndHex("Device attached. port", port);
      PutStringAndHex("  PORTSC", portsc);
    }
    if (port_state_[port] == kAttached &&
        !(portsc & kPortSCBitPortEnableDisable) &&
        !(portsc & kPortSCBitPortReset) && ReadPORTSCLinkState(port) == 7) {
      port_state_[port] = kAttachedUSB2;
      PutStringAndHex("USB2 Device attached. port", port);
      PutStringAndHex("  PORTSC", portsc);
    }
  }
}

void Controller::PrintUSBDevices() {
  for (int port = 0; port < kMaxNumOfPorts; port++) {
    if (!port_state_[port] && !port_is_initializing_[port])
      continue;
    PutStringAndHex("port", port);
    PutStringAndHex("  state", port_state_[port]);
    if (port_is_initializing_[port])
      PutString("  Initializing...");
  }
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    auto& info = slot_info_[slot];
    if (info.state == SlotInfo::kUndefined)
      continue;
    PutStringAndHex("slot", slot);
    PutStringAndHex("  state", info.state);
  }
  for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
    if (!device_context_base_array_[slot])
      continue;
    PutStringAndHex("device_context_base_array_ index", slot);
    PutStringAndHex("  paddr", device_context_base_array_[slot]);
  }
}

void Controller::Init() {
  PutString("XHCI::Init()\n");

  if (auto dev = FindXHCIController()) {
    dev_ = *dev;
  } else {
    return;
  }
  PCI::EnsureBusMasterEnabled(dev_);
  PCI::BAR64 bar0 = PCI::GetBAR64(dev_);

  cap_regs_ = MapMemoryForIO<CapabilityRegisters*>(bar0.phys_addr, bar0.size);

  const uint32_t kHCSPARAMS1 = cap_regs_->params[0];
  max_slots_ = GetBits<31, 24>(kHCSPARAMS1);
  max_intrs_ = GetBits<18, 8>(kHCSPARAMS1);
  max_ports_ = GetBits<7, 0>(kHCSPARAMS1);

  const uint32_t kHCSPARAMS2 = cap_regs_->params[1];
  max_num_of_scratch_pad_buf_entries_ =
      (GetBits<25, 21>(kHCSPARAMS2) << 5) | GetBits<31, 27>(kHCSPARAMS2);

  op_regs_ = RefWithOffset<OperationalRegisters*>(cap_regs_, cap_regs_->length);
  rt_regs_ = RefWithOffset<RuntimeRegisters*>(cap_regs_, cap_regs_->rtsoff);
  db_regs_ = RefWithOffset<volatile uint32_t*>(cap_regs_, cap_regs_->dboff);

  ResetHostController();
  InitPrimaryInterrupter();
  InitSlotsAndContexts();
  InitCommandRing();

  PutStringAndHex("max_num_of_scratch_pad_buf_entries_",
                  max_num_of_scratch_pad_buf_entries_);
  if (max_num_of_scratch_pad_buf_entries_) {
    // 4.20 Scratchpad Buffers
    scratchpad_buffer_array_ = AllocMemoryForMappedIO<uint64_t*>(
        sizeof(uint64_t) * max_num_of_scratch_pad_buf_entries_);
    device_context_base_array_[0] = v2p(scratchpad_buffer_array_);
    for (int i = 0; i < max_num_of_scratch_pad_buf_entries_; i++) {
      scratchpad_buffer_array_[i] =
          v2p(AllocMemoryForMappedIO<void*>(kPageSize));
    }
  }

  for (int i = 0; i < max_ports_; i++) {
    port_state_[i] = kDisconnected;
    port_is_initializing_[i] = false;
  }
  for (int i = 1; i < max_slots_; i++) {
    bzero(&slot_info_[i], sizeof(slot_info_[0]));
    slot_info_[i].output_ctx =
        &DeviceContext::Alloc(DeviceContext::kDCIEPContext1Out);
  }

  op_regs_->command = op_regs_->command | kUSBCMDMaskRunStop;
  while (op_regs_->status & kUSBSTSBitHCHalted) {
    PutString("Waiting for HCHalt == 0...\n");
  }

  NotifyHostControllerDoorbell();
  initialized_ = true;
}

}  // namespace XHCI
