#include "liumos.h"

void GDT::Init(uint64_t kernel_stack_pointer, uint64_t ist1_pointer) {
  descriptors_.null_segment = 0;
  descriptors_.kernel_code_segment = kDescBitTypeCode | kDescBitPresent |
                                     kCSDescBitLongMode | kCSDescBitReadable;
  descriptors_.kernel_data_segment =
      kDescBitTypeData | kDescBitPresent | kDSDescBitWritable;
  descriptors_.user_code_segment_32 = 0;
  descriptors_.user_data_segment = kDescBitTypeData | kDescBitPresent |
                                   kDSDescBitWritable |
                                   (3ULL << kDescBitOfsDPL);
  descriptors_.user_code_segment_64 = kDescBitTypeCode | kDescBitPresent |
                                      kCSDescBitLongMode | kCSDescBitReadable |
                                      (3ULL << kDescBitOfsDPL);
  bzero(&descriptors_.task_state_segment, sizeof(GDTDescriptors::TSS64Entry));
  descriptors_.task_state_segment.attr = 0b1000'0000'1000'1001 | (3 << 5);
  descriptors_.task_state_segment.SetBaseAddr(&tss64_);
  descriptors_.task_state_segment.SetLimit(sizeof(tss64_) - 1);
  bzero(&tss64_, sizeof(tss64_));
  tss64_.rsp[0] = kernel_stack_pointer;
  tss64_.ist[1] = ist1_pointer;

  gdtr_.base = reinterpret_cast<uint64_t*>(&descriptors_);
  gdtr_.limit = sizeof(GDTDescriptors) - 1;

  WriteGDTR(&gdtr_);
  WriteCSSelector(kKernelCSSelector);
  WriteSSSelector(kKernelDSSelector);
  WriteDataAndExtraSegmentSelectors(kKernelDSSelector);
  WriteTaskRegister(kTSS64Selector);
}

void GDT::Print() {
  for (size_t i = 0; i < (gdtr_.limit + 1) / sizeof(uint64_t); i++) {
    PutStringAndHex("ent", gdtr_.base[i]);
  }
}
