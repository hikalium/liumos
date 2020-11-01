#include "network.h"
#include "kernel.h"
#include "liumos.h"
#include "virtio_net.h"

void Network::IPv4Addr::Print() const {
  for (int i = 0; i < 4; i++) {
    PutDecimal64(addr[i]);
    if (i != 3)
      PutChar('.');
  }
}

void Network::IPv4NetMask::Print() const {
  for (int i = 0; i < 4; i++) {
    PutDecimal64(mask[i]);
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
    ClearIntFlag();
    virtio_net.PollRXQueue();
    StoreIntFlag();
    Sleep();
  }
}

void SendARPRequest(Network::IPv4Addr ip_addr) {
  using Net = Virtio::Net;
  using ARPPacket = Virtio::Net::ARPPacket;
  Net& virtio_net = Net::GetInstance();
  ARPPacket& arp =
      *virtio_net.GetNextTXPacketBuf<ARPPacket*>(sizeof(ARPPacket));
  arp.SetupRequest(ip_addr, virtio_net.GetSelfIPv4Addr(),
                   virtio_net.GetSelfEtherAddr());
  // send
  virtio_net.SendPacket();
}

void SendARPRequest(const char* ip_addr_str) {
  auto ip_addr = Network::IPv4Addr::CreateFromString(ip_addr_str);
  if (!ip_addr.has_value()) {
    PutString("Invalid IP Addr format: ");
    PutString(ip_addr_str);
    PutString("\n");
    return;
  }
  SendARPRequest(*ip_addr);
}

void SendDHCPRequest() {
  using DHCPPacket = Network::DHCPPacket;
  auto& virtio_net = Virtio::Net::GetInstance();
  // Send DHCP
  DHCPPacket& request =
      *virtio_net.GetNextTXPacketBuf<DHCPPacket*>(sizeof(DHCPPacket));
  request.SetupRequest(virtio_net.GetSelfEtherAddr());
  virtio_net.SendPacket();
}
