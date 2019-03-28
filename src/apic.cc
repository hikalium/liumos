#include "liumos.h"

void LocalAPIC::Init(void) {
  uint64_t base_msr = ReadMSR(MSRIndex::kLocalAPICBase);

  if (!(base_msr & kLocalAPICBaseBitAPICEnabled))
    Panic("APIC not enabled");

  if (liumos->cpu_features->x2apic &&
      !(base_msr & kLocalAPICBaseBitx2APICEnabled)) {
    base_msr |= kLocalAPICBaseBitx2APICEnabled;
    WriteMSR(MSRIndex::kLocalAPICBase, base_msr);
    base_msr = ReadMSR(MSRIndex::kLocalAPICBase);
  }

  base_addr_ = (base_msr & GetPhysAddrMask()) & ~0xfffULL;
  kernel_virt_base_addr_ = liumos->kernel_heap_allocator->MapPages<uint64_t>(
      base_addr_, ByteSizeToPageSize(kRegisterAreaSize),
      kPageAttrPresent | kPageAttrWritable | kPageAttrWriteThrough |
          kPageAttrCacheDisable);

  PutString("LocalAPIC at 0x");
  PutHex64(base_addr_);
  PutString(" is mapped at 0x");
  PutHex64(kernel_virt_base_addr_);
  PutString(" in kernel.\nmode: ");
  is_x2apic_ = (base_msr & (1 << 10));
  if (is_x2apic_)
    PutString("x2APIC");
  else
    PutString("xAPIC");

  CPUID cpuid;
  ReadCPUID(&cpuid, CPUIDIndex::kXTopology, 0);
  id_ = cpuid.edx;
  PutStringAndHex(" id", id_);
  liumos->bsp_local_apic = this;
}

void LocalAPIC::SendEndOfInterrupt(void) {
  if (is_x2apic_) {
    // WRMSR of a non-zero value causes #GP(0).
    WriteMSR(MSRIndex::kx2APICEndOfInterrupt, 0);
    return;
  }
  WriteRegister(0xB0ULL, 0);
}

static uint32_t ReadIOAPICRegister(uint8_t reg_index) {
  *reinterpret_cast<volatile uint32_t*>(kIOAPICRegIndexAddr) = reg_index;
  return *reinterpret_cast<volatile uint32_t*>(kIOAPICRegDataAddr);
}

static void WriteIOAPICRegister(uint8_t reg_index, uint32_t value) {
  *reinterpret_cast<volatile uint32_t*>(kIOAPICRegIndexAddr) = reg_index;
  *reinterpret_cast<volatile uint32_t*>(kIOAPICRegDataAddr) = value;
}

static uint64_t ReadIOAPICRedirectTableRegister(uint8_t irq_index) {
  return (uint64_t)ReadIOAPICRegister(0x10 + irq_index * 2) |
         ((uint64_t)ReadIOAPICRegister(0x10 + irq_index * 2 + 1) << 32);
}

static void WriteIOAPICRedirectTableRegister(uint8_t irq_index,
                                             uint64_t value) {
  WriteIOAPICRegister(0x10 + irq_index * 2, (uint32_t)value);
  WriteIOAPICRegister(0x10 + irq_index * 2 + 1, (uint32_t)(value >> 32));
}

static void SetInterruptRedirection(uint64_t local_apic_id,
                                    int from_irq_num,
                                    int to_vector_index) {
  uint64_t redirect_table = ReadIOAPICRedirectTableRegister(from_irq_num);
  redirect_table &= 0x00fffffffffe0000UL;
  redirect_table |= (local_apic_id << 56) | to_vector_index;
  WriteIOAPICRedirectTableRegister(from_irq_num, redirect_table);
}

void InitIOAPIC(uint64_t local_apic_id) {
  SetInterruptRedirection(local_apic_id, 2, 0x20);  // HPET
  SetInterruptRedirection(local_apic_id, 1, 0x21);  // KBC
}
