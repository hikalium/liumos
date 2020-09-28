#include "liumos.h"
#include "panic_printer.h"

[[noreturn]] void Panic(const char* s) {
  PutString("!!!! PANIC !!!!\n");
  PutString(s);
  Die();
}

void __assert(const char* expr_str, const char* file, int line) {
  auto& pp = PanicPrinter::BeginPanic();

  pp.PrintLine("Assertion failed:");
  pp.PrintLine(expr_str);
  pp.PrintLine(file);
  pp.PrintLineWithDecimal("line", line);
  pp.EndPanicAndDie("halt...");
}

bool IsEqualString(const char* a, const char* b) {
  while (*a == *b) {
    if (*a == 0)
      return true;
    a++;
    b++;
  }
  return false;
}
