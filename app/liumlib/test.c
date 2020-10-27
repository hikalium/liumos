#include "liumlib.h"

void Test() {
  assert(StrToByte("123", NULL) == 123);
  assert(MakeIPv4AddrFromString("12.34.56.78") == MakeIPv4Addr(12, 34, 56, 78));
}

int main(int argc, char** argv) {
  Test();
  Print("PASS\n");
  return 0;
}
