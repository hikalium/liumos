#include "network.h"
#include "liumos.h"
#include "virtio_net.h"

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

void NetworkManager() {
  auto& virtio_net = Virtio::Net::GetInstance();
  while (true) {
    virtio_net.PollRXQueue();
  }
}

void SendARPRequest(const char* ip_addr_str) {
  auto ip_addr = Network::IPv4Addr::CreateFromString(ip_addr_str);
  if (!ip_addr.has_value()) {
    PutString("Invalid IP Addr format: ");
    PutString(ip_addr_str);
    PutString("\n");
  }
  PutString("Sending ARP request to: ");
  ip_addr->Print();
  PutString("\n");
}
