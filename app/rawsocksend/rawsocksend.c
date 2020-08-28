#include <stdint.h>

typedef uint32_t in_addr_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef size_t socklen_t;

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

int main(int argc, char** argv) {
  int n;
  struct sockaddr_in addr;
  const char* str = "Hello, raw socket!";
  in_addr_t ping_target_ip_addr = MakeIPv4Addr(10, 10, 10, 90);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ping_target_ip_addr;
  int soc = socket(AF_INET, SOCK_RAW, kIPTypeICMP);
  if (soc < 0) {
    exit(EXIT_FAILURE);
  }
  n = sendto(soc, str, strlen(str), 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n < 1) {
    exit(EXIT_FAILURE);
  }
  close(soc);
  exit(0);
}
