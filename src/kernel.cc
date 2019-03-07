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

void SubTask() {
  constexpr int map_ysize_shift = 4;
  constexpr int map_xsize_shift = 5;
  constexpr int pixel_size = 8;

  constexpr int ysize_mask = (1 << map_ysize_shift) - 1;
  constexpr int xsize_mask = (1 << map_xsize_shift) - 1;
  constexpr int ysize = 1 << map_ysize_shift;
  constexpr int xsize = 1 << map_xsize_shift;
  static char map[ysize * xsize];
  constexpr int canvas_ysize = ysize * pixel_size;
  constexpr int canvas_xsize = xsize * pixel_size;

  map[(ysize / 2 - 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 - 1) * xsize + (xsize / 2 + 2)] = 1;

  map[(ysize / 2) * xsize + (xsize / 2 - 4)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 2)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 3)] = 1;

  map[(ysize / 2 + 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 + 1) * xsize + (xsize / 2 + 2)] = 1;

  while (1) {
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int count = 0;
        for (int p = -1; p <= 1; p++)
          for (int q = -1; q <= 1; q++)
            count +=
                map[((y + p) & ysize_mask) * xsize + ((x + q) & xsize_mask)] &
                1;
        count -= map[y * xsize + x] & 1;
        if ((map[y * xsize + x] && (count == 2 || count == 3)) ||
            (!map[y * xsize + x] && count == 3))
          map[y * xsize + x] |= 2;
      }
    }
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int p = map[y * xsize + x];
        int col = 0x000000;
        if (p & 1)
          col = 0xff0088 * (p & 2) + 0x00cc00 * (p & 1);
        map[y * xsize + x] >>= 1;
        liumos->screen_sheet->DrawRectWithoutFlush(
            liumos->screen_sheet->GetXSize() - canvas_xsize + x * pixel_size,
            y * pixel_size, pixel_size, pixel_size, col);
      }
    }
    liumos->screen_sheet->Flush(liumos->screen_sheet->GetXSize() - canvas_xsize,
                                0, canvas_xsize, canvas_ysize);
    liumos->hpet->BusyWait(200);
  }
}

GDT gdt_;
IDT idt_;
KeyboardController keyboard_ctrl_;

extern "C" void KernelEntry(LiumOS* liumos_) {
  liumos = liumos_;
  PutString("Hello from kernel!\n");

  ClearIntFlag();

  gdt_.Init();
  idt_.Init();
  keyboard_ctrl_.Init();

  StoreIntFlag();

  const int kNumOfStackPages = 3;
  void* sub_context_stack_base =
      liumos->dram_allocator->AllocPages<void*>(kNumOfStackPages);
  void* sub_context_rsp = reinterpret_cast<void*>(
      reinterpret_cast<uint64_t>(sub_context_stack_base) +
      kNumOfStackPages * (1 << 12));
  PutStringAndHex("SubTask stack base", sub_context_stack_base);

  ExecutionContext* sub_context = liumos->exec_ctx_ctrl->Create(
      SubTask, GDT::kKernelCSSelector, sub_context_rsp, GDT::kKernelDSSelector,
      reinterpret_cast<uint64_t>(&GetKernelPML4()));
  liumos->scheduler->RegisterExecutionContext(sub_context);

  // EnableSyscall();

  TextBox console_text_box;
  while (1) {
    WaitAndProcessCommand(console_text_box);
  }
}
