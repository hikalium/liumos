#include "liumos.h"

int counter;

extern "C" void KernelEntry(Sheet* vram_sheet) {
  vram_sheet->DrawRect(0, 0, 100, 100, 0xffffff);
  for (;;) {
    counter++;
  }
}
