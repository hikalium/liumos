#include "generic.h"

constexpr uint16_t kPortCOM1 = 0x3f8;

class SerialPort {
 public:
  void Init(uint16_t port);
  void SendChar(char c);

 private:
  bool IsTransmitEmpty(void);
  uint16_t port_;
};
