#include "../liumlib/liumlib.h"

static void Print(const char* s) {
  write(1, s, strlen(s));
}
static void PrintNum(int v) {
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

static char NumToHexChar(char v) {
  if (v < 10)
    return v + '0';
  if (v < 16)
    return v - 10 + 'A';
  return '?';
}
static uint8_t StrToByte(const char* s, const char** next) {
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
static uint16_t StrToNum16(const char* s, const char** next) {
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
static in_addr_t MakeIPv4AddrFromString(const char* s) {
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

int main(int argc, char** argv) {
  int socket_fd;

  if(argc < 4) {
    Print("Usage: udpclient.bin <ip addr> <port> <message>\n");
    return EXIT_FAILURE;
  }

  // Create a stream socket
  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    return EXIT_FAILURE;
  }

  struct sockaddr_in dst_address;
  dst_address.sin_family = AF_INET; /* IP */
  dst_address.sin_addr.s_addr = MakeIPv4AddrFromString(argv[1]);
  dst_address.sin_port = htons(StrToNum16(argv[2], NULL));

  char* buf = argv[3];
  ssize_t sent_size;

  sent_size = sendto(socket_fd, buf, strlen(buf), 0,
                       (struct sockaddr*)&dst_address, sizeof(dst_address));
  Print("Sent size: ");
  PrintNum(sent_size);
  Print("\n");
  return 0;
}
