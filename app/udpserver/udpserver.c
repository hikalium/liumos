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
static char NumToHexChar(char v) {
  if (v < 10)
    return v + '0';
  if (v < 16)
    return v - 10 + 'A';
  return '?';
}
static void panic(const char* s) {
  write(1, s, strlen(s));
  exit(EXIT_FAILURE);
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

int main(int argc, char** argv) {
  if (argc < 2) {
    Print("Usage: udpserver.bin <port>\n");
    return EXIT_FAILURE;
  }
  uint16_t port = StrToNum16(argv[1], NULL);

  int socket_fd;
  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    panic("error: failed to create socket\n");
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET; /* IP */
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  // Bind address to the socket
  if (bind(socket_fd, (struct sockaddr*)&server_address,
           sizeof(server_address)) == -1) {
    close(socket_fd);
    panic("error: failed to bind socket\n");
  }
  Print("Listening port: ");
  PrintNum(port);
  Print("\n");

  // Recieve loop
  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  char buf[4096];
  ssize_t recieved_size;
  for (;;) {
    recieved_size =
        recvfrom(socket_fd, (char*)buf, sizeof(buf), 0,
                 (struct sockaddr*)&client_address, &client_addr_len);
    if (recieved_size == -1) {
      panic("error: recvfrom returned -1\n");
    }
    Print("Recieved size: ");
    PrintNum(recieved_size);
    Print("\n");
    write(1, buf, recieved_size);
    Print("\n");
  }
}
