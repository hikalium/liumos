#include <stdint.h>

typedef uint32_t in_addr_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef size_t socklen_t;

#define NULL (0)
#define EXIT_FAILURE 1
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
struct sockaddr_in {
  uint16_t sin_family;
  uint16_t sin_port;
  struct in_addr sin_addr;
  uint8_t padding[8];
  // c.f.
  // https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L231
};
struct sockaddr;

int socket(int domain, int type, int protocol);
ssize_t sendto(int sockfd,
               const void* buf,
               size_t len,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t addrlen);
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
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = v % 10 + '0';
    v /= 10;
    if (!v)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
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

int main(int argc, char** argv) {
  Test();
  if (argc != 2) {
    Print("Usage: ");
    Print(argv[0]);
    Print(" <ip addr>\n");
    exit(EXIT_FAILURE);
  }
  int n;
  struct sockaddr_in addr;
  const char* str = "Hello, raw socket!";
  in_addr_t ping_target_ip_addr = MakeIPv4AddrFromString(argv[1]);
  Print("Ping to ");
  PrintIPv4Addr(ping_target_ip_addr);
  Print("...\n");

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ping_target_ip_addr;
  int soc = socket(AF_INET, SOCK_RAW, kIPTypeICMP);
  if (soc < 0) {
    Print("socket() failed\n");
    exit(EXIT_FAILURE);
  }
  n = sendto(soc, str, strlen(str), 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n < 1) {
    Print("snedto() failed\n");
    exit(EXIT_FAILURE);
  }
  close(soc);
}
