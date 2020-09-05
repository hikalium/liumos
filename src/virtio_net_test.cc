#include "virtio_net.h"

#ifdef LIUMOS_TEST

#include <stdio.h>

#include <cassert>

int main() {
  using Virtio::Net;

  constexpr Net::EtherAddr test_eth_addr = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  assert(test_eth_addr.IsEqualTo(test_eth_addr));
  assert(Net::kBroadcastEtherAddr.IsEqualTo(Net::kBroadcastEtherAddr));
  assert(!test_eth_addr.IsEqualTo(Net::kBroadcastEtherAddr));

  assert(Net::kBroadcastIPv4Addr.IsEqualTo(Net::kBroadcastIPv4Addr));
  assert(!Net::kBroadcastIPv4Addr.IsEqualTo(Net::kWildcardIPv4Addr));
  assert(!Net::kWildcardIPv4Addr.IsEqualTo(Net::kBroadcastIPv4Addr));
  assert(Net::kWildcardIPv4Addr.IsEqualTo(Net::kWildcardIPv4Addr));

  Net::DHCPPacket request;
  uint8_t* request_raw = reinterpret_cast<uint8_t*>(&request);

  request.SetupRequest(test_eth_addr);

  for (size_t i = 0; i < sizeof(Net::DHCPPacket); i++) {
    printf("%02X%c", request_raw[i], ((i & 0xF) == 0xF) ? '\n' : ' ');
  }
  putchar('\n');

  constexpr Net::InternetChecksum expected_ip_csum = {0x4F, 0xA3};
  constexpr Net::InternetChecksum expected_udp_csum = {0xBF, 0x03};
  assert(request.udp.ip.csum.IsEqualTo(expected_ip_csum));
  assert(request.udp.csum.IsEqualTo(expected_udp_csum));

  puts("PASS");
  return 0;
}

#endif
