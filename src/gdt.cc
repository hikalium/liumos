#include "liumos.h"

void GDT::Init() {
  descriptors_[0] = 0;
  descriptors_[kKernelCSIndex] = kDescBitTypeCode | kDescBitPresent |
                                 kCSDescBitLongMode | kCSDescBitReadable;
  descriptors_[kKernelDSIndex] =
      kDescBitTypeData | kDescBitPresent | kDSDescBitWritable;
  gdtr_.base = descriptors_;
  gdtr_.limit = sizeof(descriptors_) - 1;
  WriteGDTR(&gdtr_);
  WriteCSSelector(kKernelCSSelector);
  WriteSSSelector(kKernelDSSelector);
  WriteDataAndExtraSegmentSelectors(kKernelDSSelector);
  PutStringAndHex("GDT base", gdtr_.base);
  PutStringAndHex("GDT limit", gdtr_.limit);
  Print();
}

void GDT::Print() {
  for (size_t i = 0; i < (gdtr_.limit + 1) / sizeof(uint64_t); i++) {
    PutStringAndHex("ent", gdtr_.base[i]);
  }
}
