#include "../liumlib/liumlib.h"

int main(int argc, char** argv) {
  Print("argc: ");
  PrintNum(argc);
  Print("\n");
  for (int i = 0; i < argc; i++) {
    Print("argv[");
    PrintNum(i);
    Print("]: ");
    Print(argv[i]);
    Print("\n");
  }
}

