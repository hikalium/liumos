#pragma once
#include "generic.h"
#include "process_lock.h"

class Sheet;
class SerialPort;

class Console {
 public:
  struct CursorPosition {
    int x, y;
  };
  Console()
      : cursor_x_(0), cursor_y_(0), sheet_(nullptr), serial_port_(nullptr) {}
  void SetCursorPosition(int x, int y) {
    cursor_x_ = x;
    cursor_y_ = y;
  }
  void ResetCursorPosition() { SetCursorPosition(0, 0); }
  struct CursorPosition GetCursorPosition() {
    return {cursor_x_, cursor_y_};
  }
  void SetSheet(Sheet* sheet) { sheet_ = sheet; }
  void SetSerial(SerialPort* serial_port) { serial_port_ = serial_port; }
  void PutChar(char c);
  void PutString(const char* s);
  void PutChars(const char* s, int n);
  void PutHex64(uint64_t value);
  void PutHex64ZeroFilled(uint64_t value);
  void PutHex8ZeroFilled(uint8_t value);
  void PutStringAndHex(const char* s, uint64_t value);
  void PutStringAndHex(const char* s, const void* value);
  void PutStringAndBool(const char* s, bool cond);
  void PutAddressRange(uint64_t addr, uint64_t size);
  void PutAddressRange(void* addr, uint64_t size);

#ifndef LIUMOS_LOADER
  uint16_t GetCharWithoutBlocking();
#endif

 private:
  int cursor_x_, cursor_y_;
  Sheet* sheet_;
  SerialPort* serial_port_;
  ProcessLock lock_;
};

void PutChar(char c);
void PutString(const char* s);
void PutStringAndDecimal(const char* s, uint64_t value);
void PutStringAndDecimalWithPointPos(const char* s, uint64_t value, int pos);
void PutStringAndHex(const char* s, uint64_t value);
void PutStringAndHex(const char* s, const void* value);
void PutStringAndBool(const char* s, bool cond);
void PutDecimal64(uint64_t value);
void PutDecimal64WithPointPos(uint64_t value, int point_pos);
void PutHex64(uint64_t value);
void PutAddressRange(uint64_t addr, uint64_t size);
void PutAddressRange(void* addr, uint64_t size);
void PutHex64ZeroFilled(uint64_t value);
void PutHex64ZeroFilled(void* value);
void PutHex8ZeroFilled(uint8_t value);
