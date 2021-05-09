#include "kernel.h"
#include "xhci.h"

/*

USB control flow:

Slot will be assigned after the device is plugged in.
if port_state_[port] == kSlotAssigned, the slot for a device is available.

With this port:
- Address Device

*/

class USBClassDriver {
 public:
  virtual void tick(XHCI::Controller&){

  };
};

class USBCommunicationClassDriver : public USBClassDriver {
 public:
  USBCommunicationClassDriver(int slot)
      : slot_(slot), state_(kDriverAttached){};
  void tick(XHCI::Controller& xhc) {
    switch (state_) {
      case kDriverAttached: {
        kprintf("slot %d: Requesting Config Descriptor\n", slot_);
        config_desc_idx_ = 0;
        xhc.RequestConfigDescriptor(slot_, config_desc_idx_);
        state_ = kCheckingConfigDescriptor;
      } break;
      case kCheckingConfigDescriptor: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to get config descriptor\n");
          state_ = kFailed;
          return;
        }
        kprintf("slot %d: Got Config Descriptor[%d/%d]\n", slot_,
                config_desc_idx_ + 1, xhc.GetNumOfConfigs(slot_));
        ConfigDescriptor* config_desc =
            xhc.ReadDescriptorBuffer<ConfigDescriptor>(slot_, 0);
        assert(config_desc);
        kprintf("slot %d: config value = %d, index = %d\n", slot_,
                config_desc->config_value, config_desc->config_index);
        // Read interface descriptors
        int ofs = config_desc->length;
        while (ofs < config_desc->total_length) {
          const uint8_t(*hdr)[2] =
              xhc.ReadDescriptorBuffer<uint8_t[2]>(slot_, ofs);
          const uint8_t length = (*hdr)[0];
          const uint8_t type = (*hdr)[1];
          if (type == kDescriptorTypeInterface) {
            InterfaceDescriptor* interface_desc =
                xhc.ReadDescriptorBuffer<InterfaceDescriptor>(slot_, ofs);
            kprintf("      Class=0x%02X SubClass=0x%02X Protocol=0x%02X\n",
                    interface_desc->interface_class,
                    interface_desc->interface_subclass,
                    interface_desc->interface_protocol);
            if (interface_desc->interface_class == 0x02 &&
                interface_desc->interface_subclass == 0x06 &&
                interface_desc->interface_protocol == 0x00) {
              state_ = kFoundECMConfig;
            }
          }
          ofs += length;
        }
        if (state_ != kCheckingConfigDescriptor) {
          break;
        }
        config_desc_idx_++;
        if (config_desc_idx_ < xhc.GetNumOfConfigs(slot_)) {
          xhc.RequestConfigDescriptor(slot_, config_desc_idx_);
          break;
        }
        state_ = kFailed;
      } break;
      case kFoundECMConfig: {
        kprintf("slot %d: ECM config found\n", slot_);
        state_ = kFailed;
      } break;
      case kFailed: {
        // do nothing
      } break;
    }
  }

 private:
  int config_desc_idx_;
  int slot_;
  enum {
    kDriverAttached,
    kCheckingConfigDescriptor,
    kFoundECMConfig,
    kFailed,
  } state_;
};

struct SlotStatus {
  enum State {
    kUninitialized,
    kDriverAttached,
    kNoDriverFound,
  } state;
  USBClassDriver* driver;
};

SlotStatus slot_state[XHCI::Controller::kMaxNumOfSlots];

static constexpr uint8_t kUSBDeviceClassCommunication = 0x02;

static void InitDrivers(XHCI::Controller& xhc) {
  int slot = xhc.FindSlotIndexWithDeviceClass(kUSBDeviceClassCommunication);
  if (slot == -1) {
    return;
  }
  auto& ss = slot_state[slot];
  if (ss.state != SlotStatus::kUninitialized) {
    return;
  }
  kprintf("Found device_class == %d on slot %d\n", kUSBDeviceClassCommunication,
          slot);
  ss.driver = new USBCommunicationClassDriver(slot);
  ss.state = SlotStatus::kDriverAttached;
}

void USBManager() {
  kprintf("USBManager started\n");
  auto& xhc = XHCI::Controller::GetInstance();
  xhc.Init();
  for (;;) {
    xhc.PollEvents();
    InitDrivers(xhc);
    for (int i = 0; i < XHCI::Controller::kMaxNumOfSlots; i++) {
      if (slot_state[i].state != SlotStatus::kDriverAttached ||
          !slot_state[i].driver) {
        continue;
      }
      slot_state[i].driver->tick(xhc);
    }
    Sleep();
  }
}
