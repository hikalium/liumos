#include "adlib.h"

void TestAdlib() {
  if (!Adlib::DetectAndInit()) {
    return;
  }

  Adlib::NoteOn(0, 60);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(0);
  Adlib::NoteOn(1, 64);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(1);
  Adlib::NoteOn(2, 67);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(2);

  liumos->hpet->BusyWait(500);

  Adlib::NoteOn(0, 60);
  Adlib::NoteOn(1, 64);
  Adlib::NoteOn(2, 67);
  liumos->hpet->BusyWait(500);

  Adlib::NoteOff(0);
  Adlib::NoteOff(1);
  Adlib::NoteOff(2);
}
