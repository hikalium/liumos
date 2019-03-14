#include "liumos.h"

void GDT::Init(uint64_t kernel_stack_pointer) {
  descriptors_.null_segment = 0;
  descriptors_.kernel_code_segment = kDescBitTypeCode | kDescBitPresent |
                                     kCSDescBitLongMode | kCSDescBitReadable;
  descriptors_.kernel_data_segment =
      kDescBitTypeData | kDescBitPresent | kDSDescBitWritable;
  descriptors_.user_code_segment = kDescBitTypeCode | kDescBitPresent |
                                   kCSDescBitLongMode | kCSDescBitReadable |
                                   (3ULL << kDescBitOfsDPL);
  descriptors_.user_data_segment = kDescBitTypeData | kDescBitPresent |
                                   kDSDescBitWritable |
                                   (3ULL << kDescBitOfsDPL);
  bzero(&descriptors_.task_state_segment, sizeof(GDTDescriptors::TSS64Entry));
  descriptors_.task_state_segment.attr = 0b1000'0000'1000'1001 | (3 << 5);
  descriptors_.task_state_segment.SetBaseAddr(&tss64_);
  descriptors_.task_state_segment.SetLimit(sizeof(tss64_) - 1);
  bzero(&tss64_, sizeof(tss64_));
  tss64_.rsp[0] = kernel_stack_pointer;
  tss64_.rsp[1] = 0;
  tss64_.rsp[2] = 0;
  PutStringAndHex("rsp[0]", tss64_.rsp[0]);
  PutStringAndHex("rsp[1]", tss64_.rsp[1]);
  PutStringAndHex("rsp[2]", tss64_.rsp[2]);
  PutStringAndHex("ist[0]", tss64_.ist[0]);
  PutStringAndHex("ist[1]", tss64_.ist[1]);
  PutStringAndHex("ist[2]", tss64_.ist[2]);
  PutStringAndHex("ist[3]", tss64_.ist[3]);
  PutStringAndHex("ist[4]", tss64_.ist[4]);
  PutStringAndHex("ist[5]", tss64_.ist[5]);
  PutStringAndHex("ist[6]", tss64_.ist[6]);
  PutStringAndHex("ist[7]", tss64_.ist[7]);
  PutStringAndHex("ist[8]", tss64_.ist[8]);

  gdtr_.base = reinterpret_cast<uint64_t*>(&descriptors_);
  gdtr_.limit = sizeof(GDTDescriptors) - 1;
  PutStringAndHex("GDT base", gdtr_.base);
  PutStringAndHex("GDT limit", gdtr_.limit);

  WriteGDTR(&gdtr_);
  WriteCSSelector(kKernelCSSelector);
  WriteSSSelector(kKernelDSSelector);
  WriteDataAndExtraSegmentSelectors(kKernelDSSelector);
  WriteTaskRegister(kTSS64Selector);

  Print();
}

void GDT::Print() {
  for (size_t i = 0; i < (gdtr_.limit + 1) / sizeof(uint64_t); i++) {
    PutStringAndHex("ent", gdtr_.base[i]);
  }
}
