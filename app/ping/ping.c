#include "../liumlib/liumlib.h"

void PrintHex8ZeroFilled(uint8_t v) {
  char s[2];
  s[0] = NumToHexChar((v >> 4) & 0xF);
  s[1] = NumToHexChar(v & 0xF);
  write(1, s, 2);
}

void PrintIPv4Addr(in_addr_t addr) {
  uint8_t buf[4];
  *(uint32_t*)buf = addr;
  for (int i = 0;; i++) {
    PrintNum(buf[i]);
    if (i == 3)
      break;
    Print(".");
  }
}

void Test() {
  assert(StrToByte("123", NULL) == 123);
  assert(MakeIPv4AddrFromString("12.34.56.78") == MakeIPv4Addr(12, 34, 56, 78));
}

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

#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ECHO_REPLY 0

int main(int argc, char** argv) {
  Test();
  if (argc != 2) {
    Print("Usage: ");
    Print(argv[0]);
    Print(" <ip addr>\n");
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in addr;
  const char* str = "Hello, raw socket!";
  in_addr_t ping_target_ip_addr = MakeIPv4AddrFromString(argv[1]);
  Print("Ping to ");
  PrintIPv4Addr(ping_target_ip_addr);
  Print("...\n");

  // Create sockaddr_in
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ping_target_ip_addr;

  // Create ICMP socket
  // https://lwn.net/Articles/422330/
  int soc = socket(AF_INET, SOCK_DGRAM, PROT_ICMP);
  if (soc < 0) {
    Print("socket() failed\n");
    exit(EXIT_FAILURE);
  }

  // Send ICMP
  struct ICMPMessage icmp;
  icmp.type = ICMP_TYPE_ECHO_REQUEST;
  icmp.code = 0;
  icmp.checksum = 0;
  icmp.identifier = 0;
  icmp.sequence = 0;
  icmp.checksum = CalcChecksum(&icmp, 0, sizeof(icmp));
  int n = sendto(soc, &icmp, sizeof(icmp), 0, (struct sockaddr*)&addr,
                 sizeof(addr));
  if (n < 1) {
    Print("sendto() failed\n");
    exit(EXIT_FAILURE);
  }

  // Recieve reply
  uint8_t recv_buf[256];
  socklen_t addr_size;
  int recv_len = recvfrom(soc, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&addr, &addr_size);
  Print("recvfrom returns: ");
  PrintNum(recv_len);
  Print("\n");

  if (recv_len < 1) {
    Print("recvfrom() failed\n");
    exit(EXIT_FAILURE);
  }

  // Show recieved ICMP packet
  for (int i = 0; i < recv_len; i++) {
    PrintHex8ZeroFilled(recv_buf[i]);
    Print((i & 0xF) == 0xF ? "\n" : " ");
  }
  Print("\n");

  struct ICMPMessage* recv_icmp = (struct ICMPMessage *)(recv_buf);
  Print("ICMP packet recieved from ");
  PrintIPv4Addr(addr.sin_addr.s_addr);
  Print(" ICMP Type = ");
  PrintNum(recv_icmp->type);
  Print("\n");

  close(soc);
}
