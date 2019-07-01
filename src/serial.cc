#include "liumos.h"

void SerialPort::Init(uint16_t port) {
  // https://wiki.osdev.org/Serial_Ports
  port_ = port;
  WriteIOPort8(port_ + 1, 0x00);  // Disable all interrupts
  WriteIOPort8(port_ + 3, 0x80);  // Enable DLAB (set baud rate divisor)
  constexpr uint16_t baud_divisor =
      0x0001;  // baud rate = (115200 / baud_divisor)
  WriteIOPort8(port_ + 0, baud_divisor & 0xff);
  WriteIOPort8(port_ + 1, baud_divisor >> 8);
  WriteIOPort8(port_ + 3, 0x03);  // 8 bits, no parity, one stop bit
  WriteIOPort8(port_ + 2,
               0xC7);  // Enable FIFO, clear them, with 14-byte threshold
  WriteIOPort8(port_ + 4, 0x0B);  // IRQs enabled, RTS/DSR set
}

bool SerialPort::IsTransmitEmpty(void) {
  return ReadIOPort8(port_ + 5) & 0x20;
}

void SerialPort::SendChar(char c) {
  while (!IsTransmitEmpty())
    ;
  WriteIOPort8(port_, c);
}

bool SerialPort::IsReceived(void) {
  return ReadIOPort8(port_ + 5) & 1;
}

char SerialPort::ReadCharReceived(void) {
  if (!IsReceived())
    return 0;
  return ReadIOPort8(port_);
}
