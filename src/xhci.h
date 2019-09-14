#pragma once

#include <unordered_map>

#include "liumos.h"
#include "pci.h"
#include "ring_buffer.h"
#include "xhci_trb.h"
#include "xhci_trbring.h"

namespace XHCI {

class Controller {
 public:
  class DeviceContext;
  class InputContext;
  void Init();
  void PollEvents();
  void PrintPortSC();
  void PrintUSBSTS();
  void PrintUSBDevices();
  uint16_t ReadKeyboardInput();

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
    uint32_t command;
    uint32_t status;
    uint32_t page_size;
    uint32_t rsvdz1[2];
    uint32_t notification_ctrl;
    uint64_t cmd_ring_ctrl;
    uint64_t rsvdz2[2];
    uint64_t device_ctx_base_addr_array_ptr;
    uint64_t config;
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

 private:
  class EventRing;

  static constexpr int kNumOfCmdTRBRingEntries = 255;
  static constexpr int kNumOfERSForEventRing = 1;
  static constexpr int kNumOfTRBForEventRing = 128;
  static constexpr int kMaxNumOfSlots = 256;
  static constexpr int kMaxNumOfPorts = 256;
  static constexpr int kSizeOfDescriptorBuffer = 4096;

  static constexpr uint8_t kDescriptorTypeDevice = 1;
  static constexpr uint8_t kDescriptorTypeConfig = 2;

  static constexpr int kKeyBufModifierIndex = 32;

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
    uint8_t config_index;
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

  void ResetHostController();
  void InitPrimaryInterrupter();
  void InitSlotsAndContexts();
  void InitCommandRing();
  void NotifyHostControllerDoorbell();
  void NotifyDeviceContextDoorbell(int slot, int dci);
  uint32_t ReadPORTSC(int slot);
  void WritePORTSC(int slot, uint32_t data);
  void ResetPort(int port);
  void HandlePortStatusChange(int port);
  void HandleEnableSlotCompleted(int slot, int port);
  void PressKey(int hid_idx, uint8_t mod);
  void HandleKeyInput(int slot, uint8_t data[8]);
  void HandleAddressDeviceCompleted(int slot);
  void RequestDeviceDescriptor(int slot);
  void RequestConfigDescriptor(int slot);
  void SetConfig(int slot, uint8_t config_value);
  void SetHIDBootProtocol(int slot);
  void GetHIDReport(int slot);
  void HandleTransferEvent(BasicTRB& e);
  void CheckPortAndInitiateProcess();

  static Controller* xhci_;
  bool is_found_;
  PCI::DeviceLocation dev_;
  TransferRequestBlockRing<kNumOfCmdTRBRingEntries>* cmd_ring_;
  uint64_t cmd_ring_phys_addr_;
  DeviceContext volatile** device_context_base_array_;
  uint64_t device_context_base_array_phys_addr_;
  volatile CapabilityRegisters* cap_regs_;
  volatile OperationalRegisters* op_regs_;
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
  enum SlotState {
    kUndefined,
    kCheckingIfHIDClass,
    kCheckingConfigDescriptor,
    kSettingConfiguration,
    kSettingBootProtocol,
    kGettingReport,
    kNotSupportedDevice,
  } slot_state_[kMaxNumOfSlots];
  RingBuffer<uint16_t, 16> keyid_buffer_;
  enum PortState {
    kDisconnected,
    kNeedsInitializing,
    kInitializing,
    kInitialized,
    kGiveUp,
  } port_state_[kMaxNumOfPorts];
  int slot_to_port_map_[kMaxNumOfSlots];
};

}  // namespace XHCI
