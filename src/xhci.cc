#include "xhci.h"

#include "kernel.h"
#include "keyid.h"
#include "liumos.h"

#include <cstdio>
#include <utility>

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
    erst_ = liumos->kernel_heap_allocator
                ->AllocPages<volatile Controller::EventRingSegmentTableEntry*>(
                    ByteSizeToPageSize(erst_size), kPageAttrMemMappedIO);
    const size_t trbs_size =
        sizeof(Controller::CommandCompletionEventTRB) * num_of_trb_;
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
  cmd_ring_ =
      liumos->kernel_heap_allocator
          ->AllocPages<TransferRequestBlockRing<kNumOfCmdTRBRingEntries>*>(
              ByteSizeToPageSize(
                  sizeof(TransferRequestBlockRing<kNumOfCmdTRBRingEntries>)),
              kPageAttrCacheDisable | kPageAttrPresent | kPageAttrWritable);
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
  /*
    WritePORTSC(port, (portsc & kPortSCPreserveMask) | (0b1111111 << 17));
    if ((portsc & 1) == 0) {
      port_state_[port] = kDisconnected;
      return;
    }
    if (port_state_[port] != kDisconnected) {
      PutStringAndHex("  Status Changed but already gave up. port", port);
      return;
    }
    PutStringAndHex("  Reset requested on port", port);
    port_state_[port] = kNeedsPortReset;
    */
}

class Controller::DeviceContext {
  friend class InputContext;

 public:
  static constexpr int kDCISlotContext = 0;
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
    endpoint_ctx[kDCISlotContext][0] =
        CombineFieldBits<31, 27>(endpoint_ctx[kDCISlotContext][0], num_of_ent);
  }
  void SetPortSpeed(uint32_t port_speed) {
    endpoint_ctx[kDCISlotContext][0] =
        CombineFieldBits<23, 20>(endpoint_ctx[kDCISlotContext][0], port_speed);
  }
  void SetRootHubPortNumber(uint32_t root_port_num) {
    endpoint_ctx[kDCISlotContext][1] = CombineFieldBits<23, 16>(
        endpoint_ctx[kDCISlotContext][1], root_port_num);
  }
  void SetEPType(int dci, uint32_t type) {
    endpoint_ctx[dci][1] = CombineFieldBits<5, 3>(endpoint_ctx[dci][1], type);
  }
  void SetTRDequeuePointer(int dci, uint64_t tr_deq_ptr) {
    endpoint_ctx[dci][2] = CombineFieldBits<31, 4>(
        endpoint_ctx[dci][2], static_cast<uint32_t>(tr_deq_ptr >> 4));
    endpoint_ctx[dci][3] = static_cast<uint32_t>(tr_deq_ptr >> 32);
  }
  uint64_t GetTRDequeuePointer(int dci) {
    return (static_cast<uint64_t>(endpoint_ctx[dci][3]) << 32) |
           endpoint_ctx[dci][2];
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
  uint8_t GetSlotState() {
    return GetBits<31, 27>(endpoint_ctx[kDCISlotContext][3]);
  }
  uint8_t GetEPState(int dci) { return GetBits<2, 0>(endpoint_ctx[dci][0]); }
  void DumpSlotContext() {
    PutString("Slot Context:\n");
    PutStringAndHex("  Root Hub Port Number",
                    GetBits<23, 16>(endpoint_ctx[kDCISlotContext][1]));
    PutStringAndHex("  Route String",
                    GetBits<19, 0>(endpoint_ctx[kDCISlotContext][0]));
    PutStringAndHex("  Context Entries",
                    GetBits<31, 27>(endpoint_ctx[kDCISlotContext][0]));
    for (int i = 0; i < 8; i++) {
      for (int x = 3; x >= 0; x--) {
        PutHex8ZeroFilled(endpoint_ctx[kDCISlotContext][i] >> (x * 8));
      }
      PutString("\n");
    }
  }
  void DumpEPContext(int dci) {
    assert(dci >= 1);
    PutStringAndHex("EP Context", dci - 1);
    PutStringAndHex("  EP Type", GetBits<5, 3>(endpoint_ctx[dci][1]));
    PutStringAndHex("  Max Packet Size", GetBits<31, 16>(endpoint_ctx[dci][1]));
    PutStringAndHex("  TR Dequeue Pointer",
                    (GetBits<31, 4>(endpoint_ctx[dci][2]) << 4) |
                        (static_cast<uint64_t>(endpoint_ctx[dci][3]) << 32));
    PutStringAndHex("  Dequeue Cycle State",
                    GetBits<0, 0>(endpoint_ctx[dci][2]));
    PutStringAndHex("  Error Count", GetBits<2, 1>(endpoint_ctx[dci][1]));
    for (int i = 0; i < 8; i++) {
      for (int x = 3; x >= 0; x--) {
        PutHex8ZeroFilled(endpoint_ctx[dci][i] >> (x * 8));
      }
      PutString("\n");
    }
  }

  DeviceContext(int max_dci) {
    assert(0 <= max_dci && max_dci <= 31);
    bzero(this, sizeof(endpoint_ctx[0]) * (max_dci + 1));
  }

 private:
  volatile uint32_t endpoint_ctx[32][8];
};

class Controller::InputContext {
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

void Controller::SendAddressDeviceCommand(int slot,
                                          bool block_set_addr_req,
                                          uint16_t max_packet_size) {
  auto& slot_info = slot_info_[slot];
  int port = slot_info.port;
  // 4.3.3 Device Slot Initialization
  assert(slot_info.input_ctx);
  InputContext& ctx = *slot_info.input_ctx;
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
  dctx.SetEPType(DeviceContext::kDCIEPContext0, DeviceContext::kEPTypeControl);
  dctx.SetTRDequeuePointer(DeviceContext::kDCIEPContext0,
                           ctrl_ep_tring_phys_addr);
  dctx.SetDequeueCycleState(DeviceContext::kDCIEPContext0, 1);
  dctx.SetErrorCount(DeviceContext::kDCIEPContext0, 3);
  if (max_packet_size) {
    dctx.SetMaxPacketSize(DeviceContext::kDCIEPContext0, max_packet_size);

  } else {
    dctx.SetMaxPacketSize(DeviceContext::kDCIEPContext0,
                          GetMaxPacketSizeFromPORTSCPortSpeed(port_speed));
  }
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
  ctx.DumpInputControlContext();
  dctx.DumpSlotContext();
  dctx.DumpEPContext(1);
  out_device_ctx.DumpSlotContext();
  volatile BasicTRB& trb = *cmd_ring_->GetNextEnqueueEntry<BasicTRB*>();
  trb.data = v2p(&ctx);
  trb.option = 0;
  trb.control = (kTRBTypeAddressDeviceCommand << 10) | (slot << 24) |
                ((block_set_addr_req ? 1 : 0) << 9);
  PutStringAndHex("AddressDeviceCommand Enqueued to",
                  cmd_ring_->GetNextEnqueueIndex());
  PutStringAndHex("  phys addr", v2p(&trb));
  cmd_ring_->Push();
  NotifyHostControllerDoorbell();
  PrintUSBSTS();
  slot_info.state =
      block_set_addr_req
          ? SlotInfo::kWaitingForFirstAddressDeviceCommandCompletion
          : SlotInfo::kWaitingForSecondAddressDeviceCommandCompletion;
}

void Controller::HandleEnableSlotCompleted(int slot, int port) {
  port_state_[port] = kSlotAssigned;

  auto& slot_info = slot_info_[slot];
  slot_info.port = port;
  PutStringAndHex("EnableSlotCommand completed.. Slot ID", slot);
  PutStringAndHex("  With RootPort ID", port);
  PrintUSBSTS();

  slot_info.input_ctx = &InputContext::Alloc(1);
  new (slot_info.output_ctx) DeviceContext(1);
  slot_info.ctrl_ep_tring = AllocMemoryForMappedIO<
      TransferRequestBlockRing<kNumOfCtrlEPRingEntries>*>(
      sizeof(TransferRequestBlockRing<kNumOfCtrlEPRingEntries>));

  SendAddressDeviceCommand(slot, false, 0);
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

  static constexpr uint8_t kReqTypeBitRecipientInterface = 1;

  static constexpr uint8_t kReqGetDescriptor = 6;
  static constexpr uint8_t kReqSetConfiguration = 9;

  static constexpr uint32_t kTransferTypeNoDataStage = 0;
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
    uint8_t desc_idx,
    uint16_t desc_length) {
  trb.request_type = SetupStageTRB::kReqTypeBitDirectionDeviceToHost;
  trb.request = SetupStageTRB::kReqGetDescriptor;
  trb.value = (static_cast<uint16_t>(desc_type) << 8) | desc_idx;
  trb.index = 0;
  trb.length = desc_length;
  trb.option = 8;
  trb.control = (1 << 6) | (BasicTRB::kTRBTypeSetupStage << 10) | (slot << 24) |
                (SetupStageTRB::kTransferTypeInDataStage << 16);
}

static void ConfigureSetupStageTRBForSetConfiguration(struct SetupStageTRB& trb,
                                                      uint32_t slot,
                                                      uint16_t config_value) {
  trb.request_type = 0;
  trb.request = SetupStageTRB::kReqSetConfiguration;
  trb.value = config_value;
  trb.index = 0;
  trb.length = 0;
  trb.option = 8;
  trb.control = (1 << 6) | (BasicTRB::kTRBTypeSetupStage << 10) | (slot << 24) |
                (SetupStageTRB::kTransferTypeNoDataStage << 16);
}

static void ConfigureDataStageTRBForInput(struct DataStageTRB& trb,
                                          uint64_t ptr,
                                          uint32_t length,
                                          bool interrupt_on_completion) {
  trb.buf = ptr;
  trb.option = length;
  trb.control = (1 << 16) | (BasicTRB::kTRBTypeDataStage << 10) |
                (interrupt_on_completion ? 1 << 5 : 0);
}

static void ConfigureStatusStageTRBForInput(struct StatusStageTRB& trb,
                                            bool interrupt_on_completion) {
  trb.reserved = 0;
  trb.option = 0;
  trb.control = (1 << 16) | (BasicTRB::kTRBTypeStatusStage << 10) |
                (interrupt_on_completion ? 1 << 5 : 0);
}

void Controller::RequestDeviceDescriptor(int slot,
                                         SlotInfo::SlotState next_state) {
  int transfer_size = (next_state == SlotInfo::kCheckingMaxPacketSize
                           ? 8
                           : kSizeOfDescriptorBuffer);
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  ConfigureSetupStageTRBForGetDescriptorRequest(
      setup, slot, kDescriptorTypeDevice, 0, transfer_size);
  tring.Push();
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  ConfigureDataStageTRBForInput(data, v2p(descriptor_buffers_[slot]),
                                transfer_size, true);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status, false);
  tring.Push();

  slot_info_[slot].state = next_state;
  NotifyDeviceContextDoorbell(slot, 1);
  PutStringAndHex("DeviceDescriptor requested for slot", slot);
  PutStringAndHex("  Transfer Size requested", transfer_size);
}

void Controller::RequestConfigDescriptor(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  ConfigureSetupStageTRBForGetDescriptorRequest(
      setup, slot, kDescriptorTypeConfig, 0, kSizeOfDescriptorBuffer);
  tring.Push();
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  ConfigureDataStageTRBForInput(data, v2p(descriptor_buffers_[slot]),
                                kSizeOfDescriptorBuffer, true);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status, false);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kCheckingConfigDescriptor;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::SetConfig(int slot, uint8_t config_value) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  ConfigureSetupStageTRBForSetConfiguration(setup, slot, config_value);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status, true);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kSettingConfiguration;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::SetHIDBootProtocol(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.request_type = 0b00100001;
  setup.request = 0x0B;
  setup.value = 0;
  setup.index = 0;
  setup.length = 0;
  setup.option = 8;
  setup.control = (1 << 6) | (BasicTRB::kTRBTypeSetupStage << 10) |
                  (slot << 24) |
                  (SetupStageTRB::kTransferTypeNoDataStage << 16);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status, true);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kSettingBootProtocol;
  NotifyDeviceContextDoorbell(slot, 1);
}
void Controller::GetHIDReport(int slot) {
  auto& slot_info = slot_info_[slot];
  assert(slot_info.ctrl_ep_tring);
  auto& tring = *slot_info.ctrl_ep_tring;
  SetupStageTRB& setup = *tring.GetNextEnqueueEntry<SetupStageTRB*>();
  setup.request_type = 0b10100001;
  setup.request = 0x01;
  setup.value = 0x2201;
  setup.index = 0;
  setup.length = kSizeOfDescriptorBuffer;
  setup.option = 8;
  setup.control = (1 << 6) | (BasicTRB::kTRBTypeSetupStage << 10) |
                  (slot << 24) |
                  (SetupStageTRB::kTransferTypeInDataStage << 16);
  tring.Push();
  DataStageTRB& data = *tring.GetNextEnqueueEntry<DataStageTRB*>();
  ConfigureDataStageTRBForInput(data, v2p(descriptor_buffers_[slot]),
                                kSizeOfDescriptorBuffer, true);
  tring.Push();
  StatusStageTRB& status = *tring.GetNextEnqueueEntry<StatusStageTRB*>();
  ConfigureStatusStageTRBForInput(status, false);
  tring.Push();

  slot_info_[slot].state = SlotInfo::kGettingReport;
  NotifyDeviceContextDoorbell(slot, 1);
}

void Controller::HandleAddressDeviceCompleted(int slot) {
  PutStringAndHex("Address Device Completed. Slot ID", slot);
  uint8_t* buf = AllocMemoryForMappedIO<uint8_t*>(kSizeOfDescriptorBuffer);
  descriptor_buffers_[slot] = buf;
  if (slot_info_[slot].state ==
      SlotInfo::kWaitingForFirstAddressDeviceCommandCompletion) {
    RequestDeviceDescriptor(slot, SlotInfo::kCheckingMaxPacketSize);
    slot_info_[slot].state = SlotInfo::kCheckingMaxPacketSize;
    return;
  }
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
  if (e.IsCompletedWithSuccess() && e.IsCompletedWithShortPacket()) {
    PutString("TransferEvent\n");
    PutStringAndHex("  Slot ID", slot);
    PutStringAndHex("  CompletionCode", e.GetCompletionCode());
    if (e.GetCompletionCode() == 6) {
      PutString("  = Stall Error\n");
    }
    return;
  }
  switch (slot_info_[slot].state) {
    case SlotInfo::kCheckingMaxPacketSize: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      DeviceDescriptor& device_desc =
          *reinterpret_cast<DeviceDescriptor*>(descriptor_buffers_[slot]);
      PutStringAndHex("  Transfered Size",
                      kSizeOfDescriptorBuffer - e.GetTransferSize());
      PutStringAndHex("  max_packet_size", device_desc.max_packet_size);
      SendAddressDeviceCommand(slot, false, device_desc.max_packet_size);
    } break;
    case SlotInfo::kCheckingIfHIDClass: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      DeviceDescriptor& device_desc =
          *reinterpret_cast<DeviceDescriptor*>(descriptor_buffers_[slot]);
      PutStringAndHex("  Transfered Size",
                      kSizeOfDescriptorBuffer - e.GetTransferSize());
      PutStringAndHex("  max_packet_size", device_desc.max_packet_size);
      if (device_desc.device_class != 0) {
        PutStringAndHex("  Not supported device class",
                        device_desc.device_class);
        slot_info_[slot].state = SlotInfo::kNotSupportedDevice;
        return;
      }
      PutStringAndHex("  Num of Config", device_desc.num_of_config);
      RequestConfigDescriptor(slot);
    } break;
    case SlotInfo::kCheckingConfigDescriptor: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      PutStringAndHex("  Transfered Size",
                      kSizeOfDescriptorBuffer - e.GetTransferSize());
      ConfigDescriptor& config_desc =
          *reinterpret_cast<ConfigDescriptor*>(descriptor_buffers_[slot]);
      PutStringAndHex("  Num of Interfaces", config_desc.num_of_interfaces);
      InterfaceDescriptor& interface_desc =
          *reinterpret_cast<InterfaceDescriptor*>(descriptor_buffers_[slot] +
                                                  config_desc.length);
      PutStringAndHex("  Interface #       ", interface_desc.interface_number);
      {
        char s[128];
        snprintf(
            s, sizeof(s), "  Class=0x%02X SubClass=0x%02X Protocol=0x%02X\n",
            interface_desc.interface_class, interface_desc.interface_subclass,
            interface_desc.interface_protocol);
        PutString(s);
      }
      if (interface_desc.interface_class != InterfaceDescriptor::kClassHID ||
          interface_desc.interface_subclass !=
              InterfaceDescriptor::kSubClassSupportBootProtocol ||
          interface_desc.interface_protocol !=
              InterfaceDescriptor::kProtocolKeyboard) {
        PutString("Not supported interface\n");
        slot_info_[slot].state = SlotInfo::kNotSupportedDevice;
        return;
      }
      PutString("Found USB HID Keyboard (with boot protocol)\n");
      SetConfig(slot, config_desc.config_value);
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
      GetHIDReport(slot);
      break;
    case SlotInfo::kGettingReport: {
      int size = kSizeOfDescriptorBuffer - e.GetTransferSize();
      assert(size == 8);
      HandleKeyInput(slot, descriptor_buffers_[slot]);
      GetHIDReport(slot);
    } break;
    default: {
      PutString("TransferEvent\n");
      PutStringAndHex("  Slot ID", slot);
      int size = kSizeOfDescriptorBuffer - e.GetTransferSize();
      PutStringAndHex("  Transfered Size", size);
      for (int i = 0; i < size; i++) {
        PutHex8ZeroFilled(descriptor_buffers_[slot][i]);
        if ((i & 0b1111) == 0b1111)
          PutChar('\n');
        else
          PutChar(' ');
      }
      PutChar('\n');
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
      enable_slot_trb.control = (kTRBTypeEnableSlotCommand << 10);
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
    if (port_state_[i] == kNeedsPortReset) {
      ResetPort(i);
      return;
    }
  }
}

void Controller::PollEvents() {
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
      case kTRBTypeCommandCompletionEvent:
        if (e.IsCompletedWithSuccess()) {
          BasicTRB& cmd_trb = cmd_ring_->GetEntryFromPhysAddr(e.data);
          if (cmd_trb.GetTRBType() == kTRBTypeEnableSlotCommand) {
            uint64_t cmd_trb_phys_addr = e.data;
            auto it = slot_request_for_port_.find(cmd_trb_phys_addr);
            assert(it != slot_request_for_port_.end());
            HandleEnableSlotCompleted(e.GetSlotID(), it->second);
            slot_request_for_port_.erase(it);
            break;
          }
          if (cmd_trb.GetTRBType() == kTRBTypeAddressDeviceCommand) {
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
      case kTRBTypePortStatusChangeEvent:
        if (e.IsCompletedWithSuccess()) {
          HandlePortStatusChange(static_cast<int>(GetBits<31, 24>(e.data)));
          break;
        }
        PutString("PortStatusChangeEvent\n");
        e.PrintCompletionCode();
        PutStringAndHex("  Slot ID", GetBits<31, 24>(e.data));
        break;
      case kTRBTypeTransferEvent:
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
    slot_info_[i].output_ctx = &DeviceContext::Alloc(1);
  }

  op_regs_->command = op_regs_->command | kUSBCMDMaskRunStop;
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

  for (int port = 1; port <= max_ports_; port++) {
    uint32_t portsc = ReadPORTSC(port);
    if (!portsc)
      continue;
    PutStringAndHex("port", port);
    PutStringAndHex("  PORTSC", portsc);
    if (portsc & 1 && port_state_[port] == kDisconnected) {
      port_state_[port] = kNeedsPortReset;
    }
  }
}

}  // namespace XHCI
