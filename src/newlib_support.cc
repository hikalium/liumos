#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include "generic.h"

extern "C" {

caddr_t sbrk(int) {
  errno = ENOMEM;
  return 0;
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
