#include "liumos.h"

void SerialPort::Init(uint16_t port) {
  port_ = port;
  WriteIOPort8(port_ + 1, 0x00);  // Disable all interrupts
  WriteIOPort8(port_ + 3, 0x80);  // Enable DLAB (set baud rate divisor)
  WriteIOPort8(port_ + 0, 0x01);  // Set divisor (lo byte) of baud rate
  WriteIOPort8(port_ + 1, 0x00);  //                  (hi byte)
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
