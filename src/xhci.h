#pragma once

#include "liumos.h"
#include "pci.h"
#include "xhci_trbring.h"

class XHCI {
 public:
  class DeviceContext;
  class InputContext;
  void Init();
  void PollEvents();
  void PrintPortSC();
  void PrintUSBSTS();

  static XHCI& GetInstance() {
    if (!xhci_)
      xhci_ = new XHCI();
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
  static constexpr int kNumOfTRBForEventRing = 64;

  void ResetHostController();
  void InitPrimaryInterrupter();
  void InitSlotsAndContexts();
  void InitCommandRing();
  void NotifyHostControllerDoorbell();
  uint32_t ReadPORTSC(int slot);
  void WritePORTSC(int slot, uint32_t data);
  void HandlePortStatusChange(int port);
  void HandleEnableSlotCompleted(int slot, int port);

  static XHCI* xhci_;
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
};
