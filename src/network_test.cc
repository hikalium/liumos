#include "network.h"

#ifdef LIUMOS_TEST

#include <stdio.h>

#include <cassert>

int main() {
  auto ip_addr_actual = Network::IPv4Addr::CreateFromString("12.34.56.78");
  Network::IPv4Addr ip_addr_expected = {12, 34, 56, 78};
  assert(ip_addr_actual.has_value());
  assert(ip_addr_actual->IsEqualTo(ip_addr_expected));

  assert(Network::IPv4Addr::CreateFromString("0.0.0.0").has_value());
  assert(Network::IPv4Addr::CreateFromString("255.255.255.255").has_value());

  assert(!Network::IPv4Addr::CreateFromString("").has_value());
  assert(!Network::IPv4Addr::CreateFromString("123.56.78").has_value());

  puts("PASS");
  return 0;
}

#endif
