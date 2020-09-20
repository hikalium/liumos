#include <stdint.h>

typedef uint32_t in_addr_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef size_t socklen_t;

#define NULL (0)
#define EXIT_FAILURE 1
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L31
#define kIPTypeICMP 1
// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/linux/socket.h#L164
#define AF_INET 2
// c.f. https://elixir.bootlin.com/linux/v4.15/source/include/linux/net.h#L66
#define SOCK_RAW 3

// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L85
struct in_addr {
  uint32_t s_addr;
};
// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L232
// sockaddr_in means sockaddr for InterNet protocol(IP)
struct sockaddr_in {
  uint16_t sin_family;
  uint16_t sin_port;
  struct in_addr sin_addr;
  uint8_t padding[8];
  // c.f.
  // https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L231
};
// See https://elixir.bootlin.com/linux/v4.15/source/include/linux/socket.h#L30
struct sockaddr;

// Error Numbers:
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/asm-generic/errno-base.h#L26

int socket(int domain, int type, int protocol);
ssize_t sendto(int sockfd,
               const void* buf,
               size_t len,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t addrlen);
ssize_t recvfrom(int sockfd,
                 void* buf,
                 size_t len,
                 int flags,
                 struct sockaddr* dest_addr,
                 socklen_t* addrlen);
int close(int fd);
int write(int fd, const void*, size_t);
void exit(int);

size_t strlen(const char* s) {
  size_t len = 0;
  while (*s) {
    len++;
    s++;
  }
  return len;
}

in_addr_t MakeIPv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  // a.b.c.d -> in_addr_t (=uint32_t)
  uint8_t buf[4];
  buf[0] = a;
  buf[1] = b;
  buf[2] = c;
  buf[3] = d;
  return *(uint32_t*)buf;
}

uint8_t StrToByte(const char* s, const char** next) {
  uint32_t v = 0;
  while ('0' <= *s && *s <= '9') {
    v = v * 10 + *s - '0';
    s++;
  }
  if (next) {
    *next = s;
  }
  return v;
}

void Print(const char* s) {
  write(1, s, strlen(s));
}

void PrintNum(int v) {
  char s[16];
  int i;
  if (v < 0) {
    write(1, "-", 1);
    v = -v;
  }
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = v % 10 + '0';
    v /= 10;
    if (!v)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
}
char NumToHexChar(char v) {
  if (v < 10)
    return v + '0';
  if (v < 16)
    return v - 10 + 'A';
  return '?';
}
void PrintHex8ZeroFilled(uint8_t v) {
  char s[2];
  s[0] = NumToHexChar((v >> 4) & 0xF);
  s[1] = NumToHexChar(v & 0xF);
  write(1, s, 2);
}

void __assert(const char* expr_str, const char* file, int line) {
  Print("\nAssertion failed: ");
  Print(expr_str);
  Print(" at ");
  Print(file);
  Print(":");
  PrintNum(line);
  Print("\n");
  exit(EXIT_FAILURE);
}

#undef assert
#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))

in_addr_t MakeIPv4AddrFromString(const char* s) {
  // "a.b.c.d" -> in_addr_t (=uint32_t)
  uint8_t buf[4];
  for (int i = 0;; i++) {
    buf[i] = StrToByte(s, &s);
    if (i == 3)
      break;
    assert(*s == '.');
    s++;
  }
  return *(uint32_t*)buf;
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

struct __attribute__((packed)) IPHeader {
  uint16_t version;
  uint16_t total_length;
  uint16_t ident;
  uint16_t flags;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t csum;
  in_addr_t src_ip_addr;
  in_addr_t dst_ip_addr;
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

  // Create socket
  int soc = socket(AF_INET, SOCK_RAW, kIPTypeICMP);
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
  int recv_len = recvfrom(soc, &recv_buf, sizeof(recv_buf), 0, NULL, NULL);
  Print("recvfrom returns: ");
  PrintNum(recv_len);
  Print("\n");

  if (recv_len < 1) {
    Print("recvfrom() failed\n");
    exit(EXIT_FAILURE);
  }

  // Recieved data contains IP header
  for (int i = 0; i < recv_len; i++) {
    PrintHex8ZeroFilled(recv_buf[i]);
    Print((i & 0xF) == 0xF ? "\n" : " ");
  }
  Print("\n");

  struct IPHeader *ip = (struct IPHeader *)recv_buf;
  struct ICMPMessage *recv_icmp = (struct ICMPMessage *)(recv_buf + sizeof(struct IPHeader));
  Print("from ");
  PrintIPv4Addr(ip->src_ip_addr);
  Print(" to ");
  PrintIPv4Addr(ip->dst_ip_addr);
  Print(" ICMP Type = ");
  PrintNum(recv_icmp->type);
  Print("\n");

  close(soc);
}
