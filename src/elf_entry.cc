#include "liumos.h"

int counter;

LiumOS* liumos;

extern "C" void KernelEntry(LiumOS* liumos_) {
  liumos = liumos_;
  liumos->screen_sheet->DrawRect(0, 0, 100, 100, 0xffffff);
  for (;;) {
    counter++;
  }
}
