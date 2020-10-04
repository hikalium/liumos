#include "liumos.h"
#include "panic_printer.h"
#include "scheduler.h"

IDT* IDT::idt_;

__attribute__((ms_abi)) extern "C" void IntHandler(uint64_t intcode,
                                                   InterruptInfo* info) {
  IDT::GetInstance().IntHandler(intcode, info);
}

static void PrintInterruptInfo(PanicPrinter& pp,
                               uint64_t intcode,
                               const InterruptInfo* info) {
  pp.PrintLineWithHex("Int#  ", intcode);
  pp.PrintLineWithHex("CS    ", info->int_ctx.cs);
  pp.PrintLineWithHex("at CPL", info->int_ctx.cs & 3);
  pp.PrintLineWithHex("SS    ", info->int_ctx.ss);
  pp.PrintLineWithHex("RFLAGS", info->int_ctx.rflags);
  pp.PrintLineWithHex("RIP   ", info->int_ctx.rip);
  pp.PrintLineWithHex("RSP   ", info->int_ctx.rsp);
  pp.PrintLineWithHex("RBP   ", info->greg.rbp);
  pp.PrintLineWithHex("CR3   ", ReadCR3());
  uint64_t rbp = info->greg.rbp;
  for (int i = 0; i < 5; i++) {
    if (rbp < liumos->cpu_features->kernel_phys_page_map_begin) {
      break;
    }
    pp.PrintLineWithHex("ret to", *reinterpret_cast<uint64_t*>(rbp + 8));
    rbp = *reinterpret_cast<uint64_t*>(rbp);
  }
  pp.PrintLineWithHex("Error Code", info->error_code);
  pp.PrintLine("");
}

void IDT::IntHandler(uint64_t intcode, InterruptInfo* info) {
  if (liumos->is_multi_task_enabled && ReadRSP() < kKernelBaseAddr) {
    auto& pp = PanicPrinter::BeginPanic();
    PrintInterruptInfo(pp, intcode, info);
    Process& proc = liumos->scheduler->GetCurrentProcess();
    pp.PrintLineWithHex("Context#", proc.GetID());
    pp.EndPanicAndDie("Handling exception in user mode stack");
  }
  if (intcode <= 0xFF && handler_list_[intcode]) {
    handler_list_[intcode](intcode, info);
    return;
  }
  auto& pp = PanicPrinter::BeginPanic();
  PrintInterruptInfo(pp, intcode, info);
  if (liumos->is_multi_task_enabled) {
    Process& proc = liumos->scheduler->GetCurrentProcess();
    pp.PrintLineWithHex("Context#", proc.GetID());
  }
  if (intcode == 0x08) {
    pp.EndPanicAndDie("Double Fault");
  }
  if (intcode != 0x0E || ReadCR2() != info->int_ctx.rip) {
    pp.PrintLineWithHex("Memory dump at", info->int_ctx.rip);
    StringBuffer<128> line;
    for (int i = 0; i < 16; i++) {
      line.WriteChar(' ');
      line.WriteHex8ZeroFilled(
          reinterpret_cast<uint8_t*>(info->int_ctx.rip)[i]);
    }
    pp.PrintLine(line.GetString());
  }
  if (intcode == 0x0E) {
    pp.PrintLineWithHex("CR3", ReadCR3());
    pp.PrintLineWithHex("CR2", ReadCR2());
    if (info->error_code & 1) {
      // present but not ok. print entries.
      reinterpret_cast<IA_PML4*>(ReadCR3())->DebugPrintEntryForAddr(ReadCR2());
    }
    pp.PrintLine("Page Fault");
  }
  for (int i = 0; i < 16; i++) {
    StringBuffer<128> line;
    line.WriteHex64ZeroFilled(info->int_ctx.rsp + i * 8);
    line.WriteString(": ");
    line.WriteHex64ZeroFilled(
        *reinterpret_cast<uint64_t*>(info->int_ctx.rsp + i * 8));
    pp.PrintLine(line.GetString());
  }

  if (intcode == 0x03) {
    pp.PrintLineWithHex("Handler current RSP", ReadRSP());
    pp.EndPanicAndDie("Trap");
  }
  if (intcode == 0x06) {
    pp.EndPanicAndDie("Invalid Opcode");
  }
  if (intcode == 0x0D) {
    pp.EndPanicAndDie("General Protection Fault");
  }
  pp.EndPanicAndDie("INTHandler not implemented");
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
  assert(!idt_);
  static uint8_t idt_buf[sizeof(IDT)];
  idt_ = reinterpret_cast<IDT*>(idt_buf);
  idt_->InitInternal();
}

void IDT::InitInternal() {
  uint16_t cs = ReadCSSelector();

  IDTR idtr;
  idtr.limit = sizeof(descriptors_) - 1;
  idtr.base = descriptors_;

  for (int i = 0; i < 0x100; i++) {
    SetEntry(i, cs, 1, IDTType::kInterruptGate, 0, AsmIntHandlerNotImplemented);
    handler_list_[i] = nullptr;
  }

  SetEntry(0x00, cs, 0, IDTType::kInterruptGate, 0,
           AsmIntHandler00_DivideError);
  SetEntry(0x03, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler03);
  SetEntry(0x06, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler06);
  SetEntry(0x07, cs, 0, IDTType::kInterruptGate, 0,
           AsmIntHandler07_DeviceNotAvailable);
  SetEntry(0x08, cs, 1, IDTType::kInterruptGate, 0, AsmIntHandler08);
  SetEntry(0x0d, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler0D);
  SetEntry(0x0e, cs, 1, IDTType::kInterruptGate, 0, AsmIntHandler0E);
  SetEntry(0x10, cs, 0, IDTType::kInterruptGate, 0,
           AsmIntHandler10_x87FPUError);
  SetEntry(0x13, cs, 0, IDTType::kInterruptGate, 0,
           AsmIntHandler13_SIMDFPException);
  SetEntry(0x20, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler20);
  SetEntry(0x21, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler21);
  SetEntry(0x22, cs, 0, IDTType::kInterruptGate, 0, AsmIntHandler22);
  WriteIDTR(&idtr);
}
