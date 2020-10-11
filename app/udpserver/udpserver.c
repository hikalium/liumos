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

int main(int argc, char** argv) {
  int socket_fd;

  // Create a stream socket
  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    exit(1);
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET; /* IP */
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(12345);

  if (bind(socket_fd, (struct sockaddr*)&server_address,
           sizeof(server_address)) == -1) {
    write(1, "error: fail to bind socket\n", 27);
    close(socket_fd);
    exit(1);
    return EXIT_FAILURE;
  }

  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  char buf[4096];
  ssize_t recieved_size;
  for (;;) {
    recieved_size =
        recvfrom(socket_fd, (char*)buf, sizeof(buf), 0,
                 (struct sockaddr*)&client_address, &client_addr_len);
    Print("Recieved size: ");
    PrintNum(recieved_size);
    Print("\n");
    write(1, buf, recieved_size);
    Print("\n");
  }
}
