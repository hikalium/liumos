#pragma once

#include <optional>
#include <unordered_map>

#include "liumos.h"
#include "pci.h"
#include "process_lock.h"
#include "ring_buffer.h"
#include "xhci_trb.h"
#include "xhci_trbring.h"

static constexpr uint8_t kDescriptorTypeDevice = 1;
static constexpr uint8_t kDescriptorTypeConfig = 2;
static constexpr uint8_t kDescriptorTypeString = 3;
static constexpr uint8_t kDescriptorTypeInterface = 4;
static constexpr uint8_t kDescriptorTypeEndpoint = 5;

struct DeviceDescriptor {
  uint8_t length;
  uint8_t type;
  uint16_t version;
  uint8_t device_class;
  uint8_t device_subclass;
  uint8_t device_protocol;
  uint8_t max_packet_size;
  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t device_version;
  uint8_t manufacturer_idx;
  uint8_t product_idx;
  uint8_t serial_idx;
  uint8_t num_of_config;
};
static_assert(sizeof(DeviceDescriptor) == 18);

packed_struct ConfigDescriptor {
  uint8_t length;
  uint8_t type;
  uint16_t total_length;
  uint8_t num_of_interfaces;
  uint8_t config_value;
  uint8_t config_string_index;
  uint8_t attribute;
  uint8_t max_power;
};
static_assert(sizeof(ConfigDescriptor) == 9);

packed_struct InterfaceDescriptor {
  uint8_t length;
  uint8_t type;
  uint8_t interface_number;
  uint8_t alt_setting;
  uint8_t num_of_endpoints;
  uint8_t interface_class;
  uint8_t interface_subclass;
  uint8_t interface_protocol;
  uint8_t interface_index;

  static constexpr uint8_t kClassHID = 3;
  static constexpr uint8_t kSubClassSupportBootProtocol = 1;
  static constexpr uint8_t kProtocolKeyboard = 1;
};
static_assert(sizeof(InterfaceDescriptor) == 9);

packed_struct StringDescriptor {
  uint8_t length;
  uint8_t type;
};
static_assert(sizeof(StringDescriptor) == 2);

/* ECM120 5.4 */
packed_struct EthNetFuncDescriptor {
  uint8_t length;
  uint8_t type;
  uint8_t subtype;
  uint8_t mac_addr_string_index;
};

namespace XHCI {

class Controller {
 public:
  class DeviceContext;
  class InputContext;
  class EndpointContext;
  void Init();
  void PollEvents();
  void PrintPortSC();
  void PrintUSBSTS();
  void PrintUSBDevices();
  uint16_t ReadKeyboardInput();
  void RequestDescriptor(int slot, uint8_t descriptor_type);
  void RequestConfigDescriptor(int slot, uint8_t desc_idx);
  void RequestStringDescriptor(int slot, uint8_t desc_idx);
  void SetConfig(int slot, uint8_t config_value);
  enum class SlotEvent : uint8_t {
    kTransferSucceeded,
    kTransferFailed,
  };
  std::optional<SlotEvent> PopSlotEvent(int slot) {
    auto& si = slot_info_[slot];
    if (si.event_queue.IsEmpty()) {
      return std::nullopt;
    }
    return si.event_queue.Pop();
  };
  template <typename T>
  T* ReadDescriptorBuffer(int slot, int ofs) {
    if (ofs < 0 || ofs + sizeof(T) >= kSizeOfDescriptorBuffer) {
      return nullptr;
    }
    return RefWithOffset<T*>(descriptor_buffers_[slot], ofs);
  }
  int FindSlotIndexWithDeviceClass(uint8_t device_class) {
    // returns -1 if there is no such device
    for (int slot = 1; slot <= num_of_slots_enabled_; slot++) {
      auto& info = slot_info_[slot];
      if (info.state != SlotInfo::kAvailable) {
        continue;
      }
      if (info.device_class == device_class) {
        return slot;
      }
    }
    return -1;
  }
  int GetNumOfConfigs(int slot) { return slot_info_[slot].num_of_config; }
  int GetProductManufacturerIndex(int slot) {
    return slot_info_[slot].manufacturer_idx;
  }
  int GetProductStringIndex(int slot) { return slot_info_[slot].product_idx; }

  static Controller& GetInstance() {
    if (!xhci_) {
      xhci_ = liumos->kernel_heap_allocator->Alloc<Controller>();
      bzero(xhci_, sizeof(Controller));
      new (xhci_) Controller();
    }
    assert(xhci_);
    return *xhci_;
  }

  packed_struct CapabilityRegisters {
    uint8_t length;
    uint8_t reserved;
    uint16_t version;
    uint32_t params[3];
    uint32_t cap_params1;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t cap_params2;
  };
  static_assert(sizeof(CapabilityRegisters) == 0x20);
  packed_struct OperationalRegisters {
    volatile uint32_t command;
    volatile uint32_t status;
    volatile uint32_t page_size;
    volatile uint32_t rsvdz1[2];
    volatile uint32_t notification_ctrl;
    volatile uint64_t cmd_ring_ctrl;
    volatile uint64_t rsvdz2[2];
    volatile uint64_t device_ctx_base_addr_array_ptr;
    volatile uint64_t config;
  };
  static_assert(offsetof(OperationalRegisters, config) == 0x38);
  packed_struct InterrupterRegisterSet {
    volatile uint32_t management;
    volatile uint32_t moderation;
    volatile uint32_t erst_size;
    volatile uint32_t rsvdp;
    volatile uint64_t erst_base;
    volatile uint64_t erdp;
  };
  static_assert(sizeof(InterrupterRegisterSet) == 0x20);
  packed_struct RuntimeRegisters {
    volatile uint32_t microframe_index;
    volatile uint32_t rsvdz1[3 + 4];
    InterrupterRegisterSet irs[1024];
  };
  static_assert(offsetof(RuntimeRegisters, irs) == 0x20);
  static_assert(sizeof(RuntimeRegisters) == 0x8020);

  packed_struct EventRingSegmentTableEntry {
    uint64_t ring_segment_base_address;  // 64-byte aligned
    uint16_t ring_segment_size;
    uint16_t rsvdz[3];
  };
  static_assert(sizeof(EventRingSegmentTableEntry) == 0x10);

  struct CommandCompletionEventTRB {
    uint64_t cmd_trb_ptr;
    uint8_t param[4];
    uint32_t info;
  };
  static_assert(sizeof(CommandCompletionEventTRB) == 16);

  static constexpr int kNumOfCtrlEPRingEntries = 32;
  using CtrlEPTRing = TransferRequestBlockRing<kNumOfCtrlEPRingEntries>;

  static constexpr int kNumOfIntEPRingEntries = 32;
  using IntEPTRing = TransferRequestBlockRing<kNumOfIntEPRingEntries>;

  static constexpr int kNumOfCmdTRBRingEntries = 128;
  static constexpr int kNumOfERSForEventRing = 1;
  static constexpr int kNumOfTRBForEventRing = 128;
  static constexpr int kMaxNumOfSlots = 256;
  static constexpr int kMaxNumOfPorts = 256;
  static constexpr int kSizeOfDescriptorBuffer = 1024;

 private:
  class EventRing;

  static constexpr int kKeyBufModifierIndex = 32;

  static constexpr uint32_t kUSBCMDMaskRunStop = 0b01;
  static constexpr uint32_t kUSBCMDMaskHCReset = 0b10;

  static constexpr uint32_t kPortSCBitCurrentConnectStatus = 1 << 0;
  static constexpr uint32_t kPortSCBitPortEnableDisable = 1 << 1;
  static constexpr uint32_t kPortSCBitPortReset = 1 << 4;
  static constexpr uint32_t kPortSCBitPortLinkState = 0b111100000;
  static constexpr uint32_t kPortSCPortLinkStateShift = 5;
  static constexpr uint32_t kPortSCBitPortPower = 1 << 9;
  static constexpr uint32_t kPortSCBitConnectStatusChange = 1 << 17;
  static constexpr uint32_t kPortSCBitEnableStatusChange = 1 << 18;
  static constexpr uint32_t kPortSCBitPortResetChange = 1 << 21;
  static constexpr uint32_t kPortSCBitPortLinkStateChange = 1 << 22;
  static constexpr uint32_t kPortSCPreserveMask =
      0b00001110000000011100001111100000;

  static constexpr uint32_t kUSBSTSBitHCHalted = 0b1;
  static constexpr uint32_t kUSBSTSBitHCError = 1 << 12;
  static constexpr uint32_t kUSBSTSBitHSError = 1 << 2;

  packed_struct EndpointDescriptor {
    uint8_t length;
    uint8_t type;
    uint8_t endpoint_address;
    uint8_t attributes;
    uint16_t max_packet_size;
    uint8_t interval_ms;
  };
  static_assert(sizeof(EndpointDescriptor) == 7);

  struct ConfigDescriptorAndInterfaceDescriptors {
    ConfigDescriptor config;
    uint8_t num_of_interfaces;
    static const int kMaxNumOfInterfaceDescriptors = 8;
    InterfaceDescriptor interfaces[kMaxNumOfInterfaceDescriptors];
  };

  struct SlotInfo {
    enum SlotState {
      kUndefined,
      kWaitingForSecondAddressDeviceCommandCompletion,
      kWaitingForDeviceDescriptor,
      kWaitingForConfigDescriptor,
      kAvailable,
      kCheckingIfHIDClass,
      kCheckingConfigDescriptor,
      kSettingConfiguration,
      kSettingBootProtocol,
      kCheckingProtocol,
      kWaitingForConfigureEndpointCommandCompletion,
      kGettingReport,
      kNotSupportedDevice,
    } state;
    int port;
    InputContext* input_ctx;
    DeviceContext* output_ctx;
    CtrlEPTRing* ctrl_ep_tring;
    IntEPTRing* int_ep_tring;
    int max_packet_size;
    //
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t manufacturer_idx;
    uint8_t product_idx;
    uint8_t num_of_config;
    uint8_t num_of_config_retrieved;
    static const int kMaxNumOfConfigDescriptors = 8;
    ConfigDescriptorAndInterfaceDescriptors
        config_descriptors[kMaxNumOfConfigDescriptors];
    //
    RingBuffer<SlotEvent, 16> event_queue;
    const char* GetSlotStateStr();
  };

  void ResetHostController();
  void InitPrimaryInterrupter();
  void InitSlotsAndContexts();
  void InitCommandRing();
  void NotifyHostControllerDoorbell();
  void NotifyDeviceContextDoorbell(int slot, int dci);
  uint32_t ReadPORTSC(int slot);
  uint32_t ReadPORTSCLinkState(int slot) {
    return (ReadPORTSC(slot) & kPortSCBitPortLinkState) >>
           kPortSCPortLinkStateShift;
  };
  void WritePORTSC(int slot, uint32_t data);
  void ResetPort(int port);
  void DisablePort(int port);
  void HandlePortStatusChange(int port);
  void SendAddressDeviceCommand(int slot);
  void SendConfigureEndpointCommand(int slot);
  void HandleEnableSlotCompleted(int slot, int port);
  void PressKey(int hid_idx, uint8_t mod);
  void HandleKeyInput(int slot, uint8_t data[8]);
  void HandleAddressDeviceCompleted(int slot);
  void RequestDeviceDescriptor(int slot, SlotInfo::SlotState);
  void SetHIDBootProtocol(int slot);
  void GetHIDProtocol(int slot);
  void GetHIDReport(int slot);
  void HandleTransferEvent(BasicTRB& e);
  void CheckPortAndInitiateProcess();

  ProcessLock lock_;
  bool initialized_ = false;
  static Controller* xhci_;
  PCI::DeviceLocation dev_;
  TransferRequestBlockRing<kNumOfCmdTRBRingEntries>* cmd_ring_;
  uint64_t cmd_ring_phys_addr_;
  volatile uint64_t* device_context_base_array_;
  volatile CapabilityRegisters* cap_regs_;
  OperationalRegisters* op_regs_;
  RuntimeRegisters* rt_regs_;
  volatile uint32_t* db_regs_;
  EventRing* primary_event_ring_;
  uint8_t max_slots_;
  uint8_t max_intrs_;
  uint8_t max_ports_;
  int num_of_slots_enabled_;
  uint8_t* descriptor_buffers_[kMaxNumOfSlots];
  uint8_t key_buffers_[kMaxNumOfSlots][33];
  std::unordered_map<uint64_t, int> slot_request_for_port_;
  struct SlotInfo slot_info_[kMaxNumOfSlots];
  RingBuffer<uint16_t, 16> keyid_buffer_;
  enum PortState {
    kDisconnected,
    kAttached,
    kAttachedUSB2,
    //
    kNeedsPortReset,
    kNeedsSlotAssignment,
    kWaitingForSlotAssignment,
    kSlotAssigned,
    kDisabled,
  } port_state_[kMaxNumOfPorts];
  bool port_is_initializing_[kMaxNumOfPorts];
  bool controller_reset_requested_ = false;
  int max_num_of_scratch_pad_buf_entries_;
  volatile uint64_t* scratchpad_buffer_array_;
};

}  // namespace XHCI
