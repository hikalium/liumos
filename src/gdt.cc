#include "liumos.h"

IA_TSS64 tss64;

void GDT::Init() {
  descriptors_.null_segment = 0;
  descriptors_.kernel_code_segment = kDescBitTypeCode | kDescBitPresent |
                                     kCSDescBitLongMode | kCSDescBitReadable;
  descriptors_.kernel_data_segment =
      kDescBitTypeData | kDescBitPresent | kDSDescBitWritable;
  descriptors_.task_state_segment.attr = 0b1000'0000'1000'1001;
  descriptors_.task_state_segment.SetBaseAddr(&tss64);
  descriptors_.task_state_segment.SetLimit(sizeof(tss64) - 1);

  gdtr_.base = reinterpret_cast<uint64_t*>(&descriptors_);
  gdtr_.limit = sizeof(GDTDescriptors) - 1;

  WriteGDTR(&gdtr_);
  WriteCSSelector(kKernelCSSelector);
  WriteSSSelector(kKernelDSSelector);
  WriteDataAndExtraSegmentSelectors(kKernelDSSelector);
  WriteTaskRegister(kTSS64Selector);

  PutStringAndHex("GDT base", gdtr_.base);
  PutStringAndHex("GDT limit", gdtr_.limit);
  Print();
}

void GDT::Print() {
  for (size_t i = 0; i < (gdtr_.limit + 1) / sizeof(uint64_t); i++) {
    PutStringAndHex("ent", gdtr_.base[i]);
  }
}
