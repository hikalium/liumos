extern "C" {

#define CPUID_01_EDX_APIC (1 << 9)
#define CPUID_01_EDX_MSR (1 << 5)
#define MAX_PHY_ADDR_BITS 36
#define IO_APIC_BASE_ADDR 0xfec00000

typedef packed_struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
}
CPUID;

typedef packed_struct {
  uint16_t limit;
  uint64_t base;
}
GDTR;

typedef packed_struct {
  // uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t eflags;
  uint64_t rsp;
  uint64_t ss;
}
InterruptInfo;

typedef packed_struct {
  uint16_t offset_low;
  uint16_t segment_descriptor;
  unsigned interrupt_stack_table : 3;
  unsigned reserved0 : 5;
  unsigned type : 4;
  unsigned reserved1 : 1;
  unsigned descriptor_privilege_level : 2;
  unsigned present : 1;
  unsigned offset_mid : 16;
  uint32_t offset_high;
  uint32_t reserved2;
}
IDTGateDescriptor;

typedef packed_struct {
  uint16_t limit;
  IDTGateDescriptor* base;
}
IDTR;

void ReadCPUID(CPUID*, uint32_t eax, uint32_t ecx);

uint64_t ReadMSR(uint32_t);

void ReadGDTR(GDTR*);

void ReadIDTR(IDTR*);
void WriteIDTR(IDTR*);

void Int03(void);

void StoreIntFlag(void);
void StoreIntFlagAndHalt(void);
void ClearIntFlag(void);

uint16_t ReadCSSelector(void);

void AsmIntHandler03(void);
void AsmIntHandler0D(void);
void AsmIntHandler20(void);

void Disable8259PIC(void);
}
