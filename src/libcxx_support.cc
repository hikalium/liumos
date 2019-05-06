#include "generic.h"

void* operator new(unsigned long, std::align_val_t) {
  Panic("void * operator new(unsigned long, std::align_val_t)");
}

void operator delete(void*, std::align_val_t) {
  Panic("void operator delete(void*, std::align_val_t)");
}

extern "C" void __cxa_throw() {
  Panic("libcxx_support");
}

extern "C" void __cxa_begin_catch() {
  Panic("libcxx_support");
}

extern "C" void __cxa_end_catch() {
  Panic("libcxx_support");
}

extern "C" void __cxa_allocate_exception() {
  Panic("libcxx_support:__cxa_allocate_exception");
}

extern "C" void __cxa_free_exception() {
  Panic("libcxx_support");
}

void* __eh_frame_end;
void* __eh_frame_start;
void* __eh_frame_hdr_end;
void* __eh_frame_hdr_start;
void* __gxx_personality_v0;
