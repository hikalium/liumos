#include "generic.h"
#include "kernel.h"

void* operator new(unsigned long size) {
  return AllocKernelMemory<void*>(size);
}

void operator delete(void*) {
  kprintf("warning: delete called but not implemented yet\n");
}

void* operator new(unsigned long, std::align_val_t) {
  Panic("void * operator new(unsigned long, std::align_val_t)");
}

void operator delete(void*, std::align_val_t) {
  Panic("void operator delete(void*, std::align_val_t)");
}
