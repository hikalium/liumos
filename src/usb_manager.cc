#include "kernel.h"
#include "network.h"
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

static int EndpointAddressToDeviceContextIndex(uint8_t ep_addr) {
  return ((ep_addr & 0b111) << 1) | ((ep_addr >> 7) & 1);
}

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
        kprintf("slot %d: Got Config Descriptor[%d/%d]\n", slot_,
                config_desc_idx_ + 1, xhc.GetNumOfConfigs(slot_));
        ConfigDescriptor* config_desc =
            xhc.ReadDescriptorBuffer<ConfigDescriptor>(slot_, 0);
        assert(config_desc);
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to get config descriptor\n");
          state_ = kFailed;
          return;
        }
        // Copy descriptors into cache in this class
        if (config_desc->total_length > kConfigDescCacheSize) {
          kprintf("slot %d: Config descriptor too large %d\n", slot_,
                  config_desc->total_length);
        }
        memcpy(config_desc_cache_, config_desc, config_desc->total_length);
        // Read interface descriptors
        for (auto e : GetCachedConfigDescriptor()) {
          if (e->type == kDescriptorTypeInterface) {
            InterfaceDescriptor* interface_desc =
                reinterpret_cast<InterfaceDescriptor*>(e);
            kprintf(
                "  Interface:\n"
                "    Class=0x%02X SubClass=0x%02X Protocol=0x%02X\n"
                "    num_of_endpoints=%d alt_setting=%d\n",
                interface_desc->interface_class,
                interface_desc->interface_subclass,
                interface_desc->interface_protocol,
                interface_desc->num_of_endpoints, interface_desc->alt_setting);
            if (state_ != kFoundECMConfig &&
                interface_desc->interface_class == 0x02 &&
                interface_desc->interface_subclass == 0x06 &&
                interface_desc->interface_protocol == 0x00) {
              state_ = kFoundECMConfig;
              config_string_idx_ = config_desc->config_string_index;
              config_value_ = config_desc->config_value;
              continue;
            }
            if (state_ == kFoundECMConfig &&
                interface_desc->interface_class == 0x0A &&
                interface_desc->interface_subclass == 0x00 &&
                interface_desc->interface_protocol == 0x00 &&
                interface_desc->num_of_endpoints >= 2) {
              data_interface_number_ = interface_desc->interface_number;
              data_interface_alt_setting_ = interface_desc->alt_setting;
              break;
            }
          }
          if (e->type == kDescriptorTypeEndpoint) {
            EndpointDescriptor* ep_desc =
                reinterpret_cast<EndpointDescriptor*>(e);
            kprintf(
                "  Endpoint:\n"
                "    addr = 0x%02X, attr = 0x%02X\n",
                ep_desc->endpoint_address, ep_desc->attributes);
          }
          if (e->type == 0x24 /* CS_INTERFACE */) {
            const uint8_t(*cs_hdr)[3] = reinterpret_cast<uint8_t(*)[3]>(e);
            const uint8_t subtype = (*cs_hdr)[2];
            kprintf("  CS_INTERFACE desc: subtype = 0x%02X\n", subtype);
            if (subtype ==
                0x0F /* Ethernet Networking Functional Descriptor */) {
              const EthNetFuncDescriptor* eth_func_desc =
                  reinterpret_cast<EthNetFuncDescriptor*>(e);
              mac_addr_string_idx_ = eth_func_desc->mac_addr_string_index;
            }
          }
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
        kprintf(
            "slot %d: ECM config/interface found (config_value=%d, "
            "alt_setting)=%d\n",
            slot_, config_value_, data_interface_alt_setting_);
        xhc.RequestStringDescriptor(slot_, config_string_idx_);
        state_ = kWaitingForConfigStringDesc;
      } break;
      case kWaitingForConfigStringDesc: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to get string descriptor\n");
          state_ = kFailed;
          return;
        }
        StringDescriptor* string_desc =
            xhc.ReadDescriptorBuffer<StringDescriptor>(slot_, 0);
        assert(string_desc);
        const char* s = xhc.ReadDescriptorBuffer<char>(slot_, 2);
        kprintf("Selecting Config value = %d \"", config_value_);
        for (int i = 0; i < string_desc->length - 2; i += 2) {
          kprintf("%c", s[i]);
        }
        kprintf("\"...\n");
        xhc.SetConfig(slot_, config_value_);
        state_ = kWaitingForConfigCompletion;
      } break;
      case kWaitingForConfigCompletion: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to set config value\n");
          state_ = kFailed;
          return;
        }
        kprintf("slot %d: Configure done.\n", slot_);
        xhc.RequestStringDescriptor(slot_, mac_addr_string_idx_);
        state_ = kWaitingForMACAddrStringDesc;
      } break;
      case kWaitingForMACAddrStringDesc: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to get string descriptor\n");
          state_ = kFailed;
          return;
        }
        StringDescriptor* string_desc =
            xhc.ReadDescriptorBuffer<StringDescriptor>(slot_, 0);
        assert(string_desc);
        const char* s = xhc.ReadDescriptorBuffer<char>(slot_, 2);
        kprintf("MAC Addr: len = %d, ", string_desc->length);
        for (int i = 0; i < string_desc->length - 2; i += 2) {
          mac_addr_.mac[i / 4] =
              mac_addr_.mac[i / 4] << 4 | ((s[i] - '0') & 0xF);
        }
        mac_addr_.Print();
        kprintf("\n");

        xhc.SetInterface(slot_, data_interface_alt_setting_,
                         data_interface_number_);
        state_ = kWaitingForSetInterfaceCompletion;
      } break;
      case kWaitingForSetInterfaceCompletion: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to set interface\n");
          state_ = kFailed;
          return;
        }
        kprintf("Interface switched\n");

        // Setup Endpoints
        auto& config_desc = GetCachedConfigDescriptor();
        auto it = config_desc.begin();
        auto end = config_desc.end();
        int data_ep_in_dci = 0;
        int data_ep_out_dci = 0;
        int data_ep_in_max_packet_size = 0;
        int data_ep_out_max_packet_size = 0;

        for (; it != end; ++it) {
          if ((*it)->type == kDescriptorTypeInterface) {
            InterfaceDescriptor* interface_desc =
                reinterpret_cast<InterfaceDescriptor*>((*it));
            if (data_interface_alt_setting_ == interface_desc->alt_setting) {
              ++it;
              break;
            }
          }
        }
        for (; it != end; ++it) {
          if ((*it)->type != kDescriptorTypeEndpoint) {
            break;
          }
          EndpointDescriptor* ep_desc =
              reinterpret_cast<EndpointDescriptor*>((*it));
          int dci =
              EndpointAddressToDeviceContextIndex(ep_desc->endpoint_address);
          kprintf(
              "  Endpoint:\n"
              "    addr = 0x%02X, attr = 0x%02X, dci = %d\n",
              ep_desc->endpoint_address, ep_desc->attributes, dci);
          if (ep_desc->endpoint_address & (1 << 7)) {
            // IN
            data_ep_in_dci = dci;
            data_ep_in_max_packet_size = ep_desc->max_packet_size;
          } else {
            // OUT
            data_ep_out_dci = dci;
            data_ep_out_max_packet_size = ep_desc->max_packet_size;
          }
        }
        if (!data_ep_in_dci || !data_ep_out_dci) {
          kprintf("Endpoint not found\n");
          state_ = kFailed;
          return;
        }
        kprintf("data_ep_out_dci = %d, data_ep_in_dci = %d\n", data_ep_out_dci,
                data_ep_in_dci);
        xhc.ConfigureEndpointBulkInOut(
            slot_, data_ep_in_dci, data_ep_in_max_packet_size, data_ep_out_dci,
            data_ep_out_max_packet_size);
        state_ = kWaitingForConfigureEndpointCompletion;
      } break;
      case kWaitingForConfigureEndpointCompletion: {
        auto e = xhc.PopSlotEvent(slot_);
        if (!e.has_value()) {
          return;
        }
        if (*e != XHCI::Controller::SlotEvent::kTransferSucceeded) {
          kprintf("Failed to set config value\n");
          state_ = kFailed;
          return;
        }
        kprintf("slot %d: Configure endpoint done.\n", slot_);
        state_ = kFailed;
      } break;
      case kFailed: {
        // do nothing
      } break;
    }
  }

 private:
  int config_desc_idx_;
  int config_string_idx_;
  int config_value_;
  int data_interface_number_;
  int data_interface_alt_setting_;
  int mac_addr_string_idx_;
  int slot_;
  static constexpr int kConfigDescCacheSize = 4096;
  uint8_t config_desc_cache_[kConfigDescCacheSize];
  enum {
    kDriverAttached,
    kCheckingConfigDescriptor,
    kFoundECMConfig,
    kWaitingForConfigStringDesc,
    kWaitingForConfigCompletion,
    kWaitingForMACAddrStringDesc,
    kWaitingForSetInterfaceCompletion,
    kWaitingForConfigureEndpointCompletion,
    kFailed,
  } state_;
  Network::EtherAddr mac_addr_;

  ConfigDescriptor& GetCachedConfigDescriptor() {
    return *reinterpret_cast<ConfigDescriptor*>(&config_desc_cache_);
  }
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
