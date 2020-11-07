#include "../liumlib/liumlib.h"

struct __attribute__((packed)) ICMPMessage {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
};

uint16_t CalcChecksum(void* buf, size_t start, size_t end) {
  // https://tools.ietf.org/html/rfc1071
  uint8_t* p = buf;
  uint32_t sum = 0;
  for (size_t i = start; i < end; i += 2) {
    sum += ((uint16_t)p[i + 0]) << 8 | p[i + 1];
  }
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  sum = ~sum;
  return ((sum >> 8) & 0xFF) | ((sum & 0xFF) << 8);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    Print("Usage: ");
    Print(argv[0]);
    Print(" <ip addr>\n");
    exit(EXIT_FAILURE);
  }

  in_addr_t ping_target_ip_addr = MakeIPv4AddrFromString(argv[1]);
  Print("Ping to ");
  PrintIPv4Addr(ping_target_ip_addr);
  Print("...\n");

  // Create ICMP socket
  // https://lwn.net/Articles/422330/
  int soc = socket(AF_INET, SOCK_DGRAM, PROT_ICMP);
  if (soc < 0) {
    panic("socket() failed\n");
  }

  // Create sockaddr_in
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ping_target_ip_addr;

  // Send ICMP
  struct ICMPMessage icmp;
  memset(&icmp, 0, sizeof(icmp));
  icmp.type = 8; /* Echo Request */
  icmp.checksum = CalcChecksum(&icmp, 0, sizeof(icmp));
  int n = sendto(soc, &icmp, sizeof(icmp), 0, (struct sockaddr*)&addr,
                 sizeof(addr));
  if (n < 1) {
    panic("sendto() failed\n");
  }

  // Recieve reply
  uint8_t recv_buf[256];
  socklen_t addr_size;
  int recv_len = recvfrom(soc, &recv_buf, sizeof(recv_buf), 0,
                          (struct sockaddr*)&addr, &addr_size);
  if (recv_len < 1) {
    panic("recvfrom() failed\n");
  }

  Print("recvfrom returned: ");
  PrintNum(recv_len);
  Print("\n");
  // Show recieved ICMP packet
  for (int i = 0; i < recv_len; i++) {
    PrintHex8ZeroFilled(recv_buf[i]);
    Print((i & 0xF) == 0xF ? "\n" : " ");
  }
  Print("\n");

  struct ICMPMessage* recv_icmp = (struct ICMPMessage*)(recv_buf);
  Print("ICMP packet recieved from ");
  PrintIPv4Addr(addr.sin_addr.s_addr);
  Print(" ICMP Type = ");
  PrintNum(recv_icmp->type);
  Print("\n");

  close(soc);
}
