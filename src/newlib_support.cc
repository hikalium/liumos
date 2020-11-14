#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include "generic.h"
#include "kernel.h"
#include "liumos.h"
#include "panic_printer.h"

extern "C" {

caddr_t sbrk(int diff) {
  Process& proc = liumos->scheduler->GetCurrentProcess();
  ExecutionContext& ctx = proc.GetExecutionContext();
  ctx.ExpandHeap(diff);
  return (caddr_t)ctx.GetHeapEndVirtAddr();
}

void _exit(int) {
  Panic("_exit");
}

void kill(int) {
  Panic("kill");
}

void getpid(int) {
  Panic("getpid");
}

void close(int) {
  Panic("close");
}

void lseek(int) {
  Panic("lseek");
}

void read(int) {
  Panic("lseek");
}

size_t write(int, char* s, size_t len) {
  for (size_t i = 0; i < len; i++) {
    kprintf("%c", s[i]);
  }
  return len;
}
void fstat(int) {
  Panic("fstat");
}
void isatty(int) {
  Panic("isatty");
}
}
