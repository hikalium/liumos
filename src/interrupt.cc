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

static void PanicPrintPageEntriesForAddr(PanicPrinter& pp,
                                         IA_PML4& pml4,
                                         uint64_t vaddr) {
  StringBuffer<128> line;

  IA_PML4E& pml4e = pml4.GetEntryForAddr(vaddr);
  line.WriteString("PML4@");
  line.WriteHex64ZeroFilled(&pml4);
  line.WriteString("[");
  line.WriteHex64(IA_PML4::addr2index(vaddr), 3);
  line.WriteString("]: ");
  line.WriteHex64(pml4e.data);
  pp.PrintLine(line.GetString());
  line.Clear();
  if (!pml4e.IsPresent()) {
    pp.PrintLine("  -> Not present");
    return;
  }
  IA_PDPT& pdpt = *pml4e.GetTableAddr();

  IA_PDPTE& pdpte = pdpt.GetEntryForAddr(vaddr);
  line.WriteString("PDPT@");
  line.WriteHex64ZeroFilled(&pdpt);
  line.WriteString("[");
  line.WriteHex64(IA_PDPT::addr2index(vaddr), 3);
  line.WriteString("]: ");
  line.WriteHex64(pdpte.data);
  pp.PrintLine(line.GetString());
  line.Clear();
  if (!pdpte.IsPresent()) {
    pp.PrintLine("  -> Not present");
    return;
  }
  if (pdpte.IsPage()) {
    pp.PrintLine("  -> 1GiB Page");
    return;
  }
  IA_PDT& pdt = *pdpte.GetTableAddr();

  IA_PDE& pdte = pdt.GetEntryForAddr(vaddr);
  line.WriteString(" PDT@");
  line.WriteHex64ZeroFilled(&pdt);
  line.WriteString("[");
  line.WriteHex64(IA_PDT::addr2index(vaddr), 3);
  line.WriteString("]: ");
  line.WriteHex64(pdte.data);
  pp.PrintLine(line.GetString());
  line.Clear();
  if (!pdte.IsPresent()) {
    pp.PrintLine("  -> Not present");
    return;
  }
  if (pdte.IsPage()) {
    pp.PrintLine("  -> 2MiB Page");
    return;
  }
  IA_PT& pt = *pdte.GetTableAddr();

  IA_PTE& pte = pt.GetEntryForAddr(vaddr);
  line.WriteString("  PT@");
  line.WriteHex64ZeroFilled(&pt);
  line.WriteString("[");
  line.WriteHex64(IA_PT::addr2index(vaddr), 3);
  line.WriteString("]: ");
  line.WriteHex64(pte.data);
  pp.PrintLine(line.GetString());
  if (!pte.IsPresent()) {
    pp.PrintLine("  -> Not present");
    return;
  }
  pp.PrintLine("  -> 4KiB Page");
}

void IDT::IntHandler(uint64_t intcode, InterruptInfo* info) {
  if (intcode <= 0xFF && handler_list_[intcode]) {
    handler_list_[intcode](intcode, info);
    return;
  }
  auto& pp = PanicPrinter::BeginPanic();
  PrintInterruptInfo(pp, intcode, info);
  uint64_t cr3_at_interrupt = ReadCR3();
  if (liumos->is_multi_task_enabled) {
    Process& proc = liumos->scheduler->GetCurrentProcess();
    pp.PrintLineWithHex("Context#", proc.GetID());
    // Switch CR3 to kernel to access full memory space
    WriteCR3(liumos->kernel_pml4_phys);
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
      PanicPrintPageEntriesForAddr(
          pp, *reinterpret_cast<IA_PML4*>(cr3_at_interrupt), ReadCR2());
      pp.PrintLine("Page is present but not accessible in this context");
    }
    pp.EndPanicAndDie("Page Fault");
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
