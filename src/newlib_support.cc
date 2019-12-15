#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include "generic.h"
#include "liumos.h"

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

void write(int) {
  Panic("write");
}
void fstat(int) {
  Panic("fstat");
}
void isatty(int) {
  Panic("isatty");
}
}
