#include "liumos.h"
#include "scheduler.h"

packed_struct ContextSwitchRequest {
  CPUContext* from;
  CPUContext* to;
};

Scheduler* scheduler;
RingBuffer<uint8_t, 16> keycode_buffer;

IDTR idtr;
ContextSwitchRequest context_switch_request;

extern "C" ContextSwitchRequest* IntHandler(uint64_t intcode,
                                            uint64_t error_code,
                                            InterruptInfo* info) {
  uint64_t kernel_gs_base = ReadMSR(MSRIndex::kKernelGSBase);
  ExecutionContext* current_context =
      reinterpret_cast<ExecutionContext*>(kernel_gs_base);
  if (intcode == 0x20) {
    SendEndOfInterruptToLocalAPIC();
    ExecutionContext* next_context = scheduler->SwitchContext(current_context);
    if (!next_context) {
      // no need to switching context.
      return nullptr;
    }
    context_switch_request.from = current_context->GetCPUContext();
    context_switch_request.to = next_context->GetCPUContext();
    WriteMSR(MSRIndex::kKernelGSBase, reinterpret_cast<uint64_t>(next_context));
    return &context_switch_request;
  }
  if (intcode == 0x21) {
    SendEndOfInterruptToLocalAPIC();
    keycode_buffer.Push(ReadIOPort8(kIOPortKeyboardData));
    return NULL;
  }
  PutStringAndHex("Int#", intcode);
  PutStringAndHex("RIP", info->rip);
  PutStringAndHex("CS", info->cs);
  PutStringAndHex("Error Code", error_code);
  PutStringAndHex("Context#", current_context->GetID());

  if (intcode == 0x03) {
    Panic("Int3 Trap");
  }
  if (intcode == 0x0D) {
    Panic("General Protection Fault");
  }
  if (intcode == 0x0E) {
    Panic("Page Fault");
  }
  Panic("INTHandler not implemented");
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

void SetIntHandler(int index,
                   uint8_t segm_desc,
                   uint8_t ist,
                   IDTType type,
                   uint8_t dpl,
                   void (*handler)()) {
  IDTGateDescriptor* desc = &idtr.base[index];
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
  ReadIDTR(&idtr);

  new (&keycode_buffer) RingBuffer<uint8_t, 16>();

  uint16_t cs = ReadCSSelector();
  SetIntHandler(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetIntHandler(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetIntHandler(0x0e, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0E);
  SetIntHandler(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  SetIntHandler(0x21, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler21);
  WriteIDTR(&idtr);
}
