#include "network.h"
#include "liumos.h"

void Network::IPv4Addr::Print() const {
  for (int i = 0; i < 4; i++) {
    PutDecimal64(addr[i]);
    if (i != 3)
      PutChar('.');
  }
}

void Network::EtherAddr::Print() const {
  for (int i = 0; i < 6; i++) {
    PutHex8ZeroFilled(mac[i]);
    if (i != 5)
      PutChar(':');
  }
}

Network* Network::network_;

Network& Network::GetInstance() {
  if (!network_) {
    network_ = liumos->kernel_heap_allocator->Alloc<Network>();
    bzero(network_, sizeof(Network));
    new (network_) Network();
  }
  assert(network_);
  return *network_;
}
