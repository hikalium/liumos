#pragma once

#include "asm.h"
#include "console.h"
#include "serial.h"
#include "sheet.h"
#include "sheet_painter.h"
#include "string_buffer.h"

class PanicPrinter {
 public:
  static void Init(void* buf, Sheet& sheet, SerialPort& serial) {
    pp_ = reinterpret_cast<PanicPrinter*>(buf);
    new (pp_) PanicPrinter();
    pp_->sheet_ = &sheet;
    pp_->serial_ = &serial;
  }
  static PanicPrinter& BeginPanic() {
    ClearIntFlag();
    if (!pp_ || !pp_->sheet_ || !pp_->serial_) {
      Die();
    }
    if (pp_->in_progress_) {
      pp_->PrintLine(
          "BeginPanic() called during panic printing. Double fault?");
      pp_->Flush();
      for (;;) {
        asm volatile("hlt; pause;");
      };
    }
    pp_->in_progress_ = true;
    pp_->PrintLine("---- Begin Panic ----");
    return *pp_;
  }
  [[noreturn]] void EndPanicAndDie(const char* s) {
    PrintLine(s);
    pp_->PrintLine("----  End  Panic ----");
    pp_->Flush();
    Die();
  }
  void PrintLineWithHex(const char* s, uint64_t v) {
    StringBuffer<128> line;
    line.WriteString(s);
    line.WriteString(": 0x");
    line.WriteHex64(v);
    PrintLine(line.GetString());
  }
  void PrintLineWithDecimal(const char* s, uint64_t v) {
    StringBuffer<128> line;
    line.WriteString(s);
    line.WriteString(": ");
    line.WriteDecimal64(v);
    PrintLine(line.GetString());
  }
  void PrintLine(const char* s) {
    str_buf_.WriteString(s);
    str_buf_.WriteString("\n");
  }

 private:
  void Flush() {
    const char* s = str_buf_.GetString();
    if (serial_) {
      for (int i = 0; s[i]; i++) {
        serial_->SendChar(s[i]);
      }
    }
    if (!sheet_) {
      Die();
    }
    int cursor_y = 0;
    while (*s) {
      SheetPainter::DrawRect(*sheet_, 0, cursor_y, sheet_->GetXSize(), 16,
                             0x800000, false);
      for (int x = 0; x < sheet_->GetXSize() / 8; x++) {
        if (!*s)
          break;
        if (*s == '\n') {
          s++;
          break;
        }
        SheetPainter::DrawCharacterForeground(*sheet_, *s, x * 8, cursor_y,
                                              0xFFFFFF, false);
        s++;
      }

      cursor_y += 16;
      if (cursor_y + 16 > sheet_->GetYSize()) {
        break;
      }
    }
  }
  static PanicPrinter* pp_;
  Sheet* sheet_;
  SerialPort* serial_;
  bool in_progress_;
  static constexpr int kBufSize = 1024;
  StringBuffer<kBufSize> str_buf_;

  PanicPrinter(){};
};
