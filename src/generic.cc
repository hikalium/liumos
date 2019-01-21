#include "liumos.h"

[[noreturn]] void Panic(const char* s) {
  PutString("!!!! PANIC !!!!\n");
  PutString(s);
  Die();
}

void __assert(const char* expr_str, const char* file, int line) {
  PutString("Assertion failed: ");
  PutString(expr_str);
  PutString(" at ");
  PutString(file);
  PutString(":0x");
  PutHex64(line);
  PutString("\n");
  Panic("halt...");
}
