#include "../liumlib/liumlib.h"

int main(int argc, char** argv) {
  if(argc < 4) {
    Print("Usage: udpclient.bin <ip addr> <port> <message>\n");
    return EXIT_FAILURE;
  }

  int socket_fd;
  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
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
