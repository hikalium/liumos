#include "generic.h"

void* operator new(unsigned long, std::align_val_t) {
  Panic("void * operator new(unsigned long, std::align_val_t)");
}

void operator delete(void*, std::align_val_t) {
  Panic("void operator delete(void*, std::align_val_t)");
}

void* __eh_frame_end;
void* __eh_frame_start;
void* __eh_frame_hdr_end;
void* __eh_frame_hdr_start;
