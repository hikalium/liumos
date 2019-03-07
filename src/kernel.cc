#include "liumos.h"

int counter;

LiumOS* liumos;

void WaitAndProcessCommand(TextBox& tbox) {
  PutString("> ");
  tbox.StartRecording();
  while (1) {
    StoreIntFlagAndHalt();
    while (1) {
      uint16_t keyid;
      if ((keyid = liumos->keyboard_ctrl->ReadKeyID())) {
        if (!keyid && keyid & KeyID::kMaskBreak)
          continue;
        if (keyid == KeyID::kEnter) {
          keyid = '\n';
        }
      } else if (liumos->com1->IsReceived()) {
        keyid = liumos->com1->ReadCharReceived();
        if (keyid == '\n') {
          continue;
        }
        if (keyid == '\r') {
          keyid = '\n';
        }
      } else {
        break;
      }
      if (keyid == '\n') {
        tbox.StopRecording();
        tbox.putc('\n');
        ConsoleCommand::Process(tbox);
        return;
      }
      tbox.putc(keyid);
    }
  }
}

extern "C" void KernelEntry(LiumOS* liumos_) {
  liumos = liumos_;
  liumos->screen_sheet->DrawRect(0, 0, 100, 100, 0xffffff);
  PutString("Hello from kernel!\n");
  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
