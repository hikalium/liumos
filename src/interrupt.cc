#include "liumos.h"
#include "scheduler.h"

__attribute__((ms_abi)) extern "C" ContextSwitchRequest*
IntHandler(uint64_t intcode, uint64_t error_code, InterruptInfo* info) {
  return liumos->idt->IntHandler(intcode, error_code, info);
}
ContextSwitchRequest* IDT::IntHandler(uint64_t intcode,
                                      uint64_t error_code,
                                      InterruptInfo* info) {
  if (intcode <= 0xFF && handler_list_[intcode]) {
    handler_list_[intcode](intcode, error_code, info);
    return nullptr;
  }
  ExecutionContext* current_context = liumos->scheduler->GetCurrentContext();
  if (intcode == 0x20) {
    liumos->bsp_local_apic->SendEndOfInterrupt();
    ExecutionContext* next_context = liumos->scheduler->SwitchContext();
    if (!next_context) {
      // no need to switching context.
      return nullptr;
    }
    context_switch_request_.from = current_context->GetCPUContext();
    context_switch_request_.to = next_context->GetCPUContext();
    return &context_switch_request_;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS Index", info->cs >> 3);
  PutStringAndHex("CS   RPL", info->cs & 3);
  PutStringAndHex("Error Code", error_code);
  PutStringAndHex("Context#", current_context->GetID());
  if (intcode == 0x08) {
    Panic("Double Fault");
  }
  if (intcode != 0x0E || ReadCR2() != info->rip) {
    PutStringAndHex("Memory dump at", info->rip);
    for (int i = 0; i < 16; i++) {
      PutChar(' ');
      PutHex8ZeroFilled(reinterpret_cast<uint8_t*>(info->rip)[i]);
    }
    PutChar('\n');
  }
  if (intcode == 0x0E) {
    PutStringAndHex("CR3", ReadCR3());
    PutStringAndHex("CR2", ReadCR2());
    if (error_code & 1) {
      // present but not ok. print entries.
      reinterpret_cast<IA_PML4*>(ReadCR3())->DebugPrintEntryForAddr(ReadCR2());
    }
    Panic("Page Fault");
  }

  if (intcode == 0x03) {
    Panic("Int3 Trap");
  }
  if (intcode == 0x06) {
    Panic("Invalid Opcode");
  }
  if (intcode == 0x0D) {
    Panic("General Protection Fault");
  }
  Panic("INTHandler not implemented");
}

void IDT::SetIntHandler(uint64_t intcode, InterruptHandler handler) {
  assert(intcode <= 0xFF);
  handler_list_[intcode] = handler;
}

void PrintIDTGateDescriptor(IDTGateDescriptor* desc) {
  PutStringAndHex("desc.ofs", ((uint64_t)desc->offset_high << 32) |
                                  ((uint64_t)desc->offset_mid << 16) |
                                  desc->offset_low);
  PutStringAndHex("desc.segmt", desc->segment_descriptor);
  PutStringAndHex("desc.IST", desc->interrupt_stack_table);
  PutStringAndHex("desc.type", desc->type);
  PutStringAndHex("desc.DPL", desc->descriptor_privilege_level);
  PutStringAndHex("desc.present", desc->present);
}

void IDT::SetEntry(int index,
                   uint8_t segm_desc,
                   uint8_t ist,
                   IDTType type,
                   uint8_t dpl,
                   __attribute__((ms_abi)) void (*handler)()) {
  IDTGateDescriptor* desc = &descriptors_[index];
  desc->segment_descriptor = segm_desc;
  desc->interrupt_stack_table = ist;
  desc->type = static_cast<int>(type);
  desc->descriptor_privilege_level = dpl;
  desc->present = 1;
  desc->offset_low = (uint64_t)handler & 0xffff;
  desc->offset_mid = ((uint64_t)handler >> 16) & 0xffff;
  desc->offset_high = ((uint64_t)handler >> 32) & 0xffffffff;
  desc->reserved0 = 0;
  desc->reserved1 = 0;
  desc->reserved2 = 0;
}

void IDT::Init() {
  uint16_t cs = ReadCSSelector();

  IDTR idtr;
  idtr.limit = sizeof(descriptors_) - 1;
  idtr.base = descriptors_;

  for (int i = 0; i < 0x100; i++) {
    SetEntry(i, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandlerNotImplemented);
    handler_list_[i] = nullptr;
  }

  SetEntry(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetEntry(0x06, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler06);
  SetEntry(0x08, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler08);
  SetEntry(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetEntry(0x0e, cs, 1, IDTType::kInterruptGate, 0, AsmIntHandler0E);
  SetEntry(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  SetEntry(0x21, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler21);
  WriteIDTR(&idtr);
  liumos->idt = this;
}
