#pragma once
#include "ring_buffer.h"
#include "scheduler.h"

using InterruptHandler = void (*)(uint64_t intcode, InterruptInfo* info);

class IDT {
 public:
  void IntHandler(uint64_t intcode, InterruptInfo* info);
  void SetIntHandler(uint64_t intcode, InterruptHandler handler);

  static IDT& GetInstance() {
    assert(idt_);
    return *idt_;
  }
  static void Init();

 private:
  static IDT* idt_;
  IDTGateDescriptor descriptors_[256];
  InterruptHandler handler_list_[256];

  IDT() = delete;
  void InitInternal();
  void SetEntry(int index,
                uint8_t segm_desc,
                uint8_t ist,
                IDTType type,
                uint8_t dpl,
                __attribute__((ms_abi)) void (*handler)());
};
