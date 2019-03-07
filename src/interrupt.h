#pragma once
#include "ring_buffer.h"
#include "scheduler.h"

packed_struct ContextSwitchRequest {
  CPUContext* from;
  CPUContext* to;
};

using InterruptHandler = void (*)(uint64_t intcode,
                                  uint64_t error_code,
                                  InterruptInfo* info);

class IDT {
 public:
  void Init();
  ContextSwitchRequest* IntHandler(uint64_t intcode,
                                   uint64_t error_code,
                                   InterruptInfo* info);
  void SetIntHandler(uint64_t intcode, InterruptHandler handler);

 private:
  ContextSwitchRequest context_switch_request_;
  IDTGateDescriptor descriptors_[256];
  InterruptHandler handler_list_[256];
  void SetEntry(int index,
                uint8_t segm_desc,
                uint8_t ist,
                IDTType type,
                uint8_t dpl,
                __attribute__((ms_abi)) void (*handler)());
};
