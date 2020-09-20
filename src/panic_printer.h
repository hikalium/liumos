#pragma once

#include "asm.h"
#include "console.h"
#include "sheet.h"
#include "sheet_painter.h"
#include "string_buffer.h"

class PanicPrinter {
 public:
  static void Init(void* buf, Sheet& sheet, SerialPort& serial) {
    pp_ = reinterpret_cast<PanicPrinter*>(buf);
    pp_->sheet_ = &sheet;
    pp_->serial_ = &serial;
  }
  static PanicPrinter& BeginPanic() {
    ClearIntFlag();
    if (!pp_->sheet_ || !pp_->serial_) {
      Die();
    }
    pp_->PrintLine("---- Begin Panic ----");
    return *pp_;
  }
  [[noreturn]] void EndPanicAndDie(const char* s) {
    PrintLine(s);
    pp_->PrintLine("----  End  Panic ----");
    Die();
  } void PrintLineWithHex(const char* s, uint64_t v) {
    StringBuffer<128> line;
    line.WriteString(s);
    line.WriteString(": 0x");
    line.WriteHex64(v);
    PrintLine(line.GetString());
  }
  void PrintLine(const char* s) {
    if (!sheet_) {
      Die();
    }
    SheetPainter::DrawRect(*sheet_, 0, cursor_y_, sheet_->GetXSize(), 16,
                           0x800000, false);
    for (int x = 0; x < sheet_->GetXSize() / 8; x++) {
      if (!s[x])
        break;
      SheetPainter::DrawCharacterForeground(*sheet_, s[x], x * 8, cursor_y_,
                                            0xFFFFFF, false);
    }

    cursor_y_ += 16;
    if (cursor_y_ + 16 > sheet_->GetYSize()) {
      cursor_y_ -= 16;
    }
  }

 private:
  static PanicPrinter* pp_;
  Sheet* sheet_;
  SerialPort* serial_;
  int cursor_y_;

  PanicPrinter(){};
};
