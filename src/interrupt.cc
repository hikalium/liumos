#include "liumos.h"
#include "scheduler.h"

packed_struct ContextSwitchRequest {
  CPUContext* from;
  CPUContext* to;
};

ContextSwitchRequest context_switch_request;
IDTGateDescriptor idt[256];
InterruptHandler handler_list[256];

extern "C" ContextSwitchRequest* IntHandler(uint64_t intcode,
                                            uint64_t error_code,
                                            InterruptInfo* info) {
  if (intcode <= 0xFF && handler_list[intcode]) {
    handler_list[intcode](intcode, error_code, info);
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
    context_switch_request.from = current_context->GetCPUContext();
    context_switch_request.to = next_context->GetCPUContext();
    return &context_switch_request;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS", info->cs);
  PutStringAndHex("Error Code", error_code);
  PutStringAndHex("Context#", current_context->GetID());
  if (intcode == 0x0E) {
    PutStringAndHex("CR3", ReadCR3());
    PutStringAndHex("CR2", ReadCR2());
    Panic("Page Fault");
  }

  PutStringAndHex("Memory dump at", info->rip);
  for (int i = 0; i < 16; i++) {
    PutChar(' ');
    PutHex8ZeroFilled(reinterpret_cast<uint8_t*>(info->rip)[i]);
  }
  PutChar('\n');

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

void SetIntHandler(uint64_t intcode, InterruptHandler handler) {
  assert(intcode <= 0xFF);
  handler_list[intcode] = handler;
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

static void SetIntHandler(int index,
                          uint8_t segm_desc,
                          uint8_t ist,
                          IDTType type,
                          uint8_t dpl,
                          __attribute__((ms_abi)) void (*handler)()) {
  IDTGateDescriptor* desc = &idt[index];
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

void InitIDT() {
  uint16_t cs = ReadCSSelector();

  IDTR idtr;
  idtr.limit = sizeof(idt) - 1;
  idtr.base = idt;

  for (int i = 0; i < 0x100; i++) {
    SetIntHandler(i, cs, 0, IDTType::kInterruptGate, 0,
                  AsmIntHandlerNotImplemented);
  }

  SetIntHandler(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetIntHandler(0x06, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler06);
  SetIntHandler(0x08, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler08);
  SetIntHandler(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetIntHandler(0x0e, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0E);
  SetIntHandler(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  SetIntHandler(0x21, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler21);
  WriteIDTR(&idtr);
}
