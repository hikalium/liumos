#include "liumos.h"

void GDT::Print() {
  for (int i = 0; i < 10; i++) {
    PutStringAndHex("ent", gdtr_.base[i]);
  }
}
