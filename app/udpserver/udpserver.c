#include "../liumlib/liumlib.h"

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

  // Receive loop
  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  char buf[4096];
  ssize_t received_size;
  for (;;) {
    received_size =
        recvfrom(socket_fd, (char*)buf, sizeof(buf), 0,
                 (struct sockaddr*)&client_address, &client_addr_len);
    if (received_size == -1) {
      panic("error: recvfrom returned -1\n");
    }
    Print("Received size: ");
    PrintNum(received_size);
    Print("\n");
    write(1, buf, received_size);
    Print("\n");
  }
}
