#include "adlib.h"

int Adlib::note_on_count_[0x100];
int Adlib::ch_of_note_[0x100];
int Adlib::note_of_ch_[9];

void TestAdlib() {
  if (!Adlib::DetectAndInit()) {
    return;
  }

  Adlib::NoteOn(60);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(60);

  Adlib::NoteOn(64);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(64);

  Adlib::NoteOn(67);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(67);

  liumos->hpet->BusyWait(500);

  Adlib::NoteOn(60);
  Adlib::NoteOn(64);
  Adlib::NoteOn(67);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(60);
  Adlib::NoteOff(64);
  Adlib::NoteOff(67);

  liumos->hpet->BusyWait(500);

  for (int i = 0; i < 10; i++) {
    Adlib::NoteOn(i + 60);
    liumos->hpet->BusyWait(200);
  }
  for (int i = 0; i < 10; i++) {
    liumos->hpet->BusyWait(200);
    Adlib::NoteOff(i + 60);
  }

  Adlib::TurnOffAllNotes();
}
