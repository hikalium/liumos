#pragma once
#include "generic.h"

constexpr uint16_t kPortCOM1 = 0x3f8;
constexpr uint16_t kPortCOM2 = 0x2f8;

class SerialPort {
 public:
  void Init(uint16_t port);
  void SendChar(char c);
  bool IsReceived(void);
  char ReadCharReceived(void);

 private:
  bool IsTransmitEmpty(void);
  uint16_t port_;
};
