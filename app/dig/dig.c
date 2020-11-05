#include "../liumlib/liumlib.h"

struct DNSMessage {
  uint16_t transaction_id;
  uint16_t flags;
  uint16_t num_questions;
  uint16_t num_answers;
  uint16_t num_authority_rr;
  uint16_t num_additional_rr;
};

char packet_bytes[] = {0x00, 0x00, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x08, 0x68, 0x69, 0x6b,
                       0x61, 0x6c, 0x69, 0x75, 0x6d, 0x03, 0x63, 0x6f,
                       0x6d, 0x00, 0x00, 0x01, 0x00, 0x01};

int main(int argc, char** argv) {
  if (argc != 2) {
    Print("Usage: dig.bin <DNS server ip>\n");
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
  dst_address.sin_port = htons(53);

  ssize_t sent_size;

  sent_size = sendto(socket_fd, packet_bytes, sizeof(packet_bytes), 0,
                     (struct sockaddr*)&dst_address, sizeof(dst_address));
  Print("Sent size: ");
  PrintNum(sent_size);
  Print("\n");

  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  uint8_t buf[4096];
  ssize_t recv_size =
      recvfrom(socket_fd, (char*)buf, sizeof(buf), 0,
               (struct sockaddr*)&client_address, &client_addr_len);
  if (recv_size == -1) {
    panic("error: recvfrom returned -1\n");
  }
  Print("Recieved size: ");
  PrintNum(recv_size);
  Print("\n");

  for (int i = 0; i < recv_size; i++) {
    PrintHex8ZeroFilled(buf[i]);
    Print((i & 0xF) == 0xF ? "\n" : " ");
  }
  Print("\n");

  write(1, buf, recv_size);
  Print("\n");

  struct DNSMessage* dns = (struct DNSMessage*)buf;
  Print("Num of answer RRs: ");
  PrintNum(htons(dns->num_answers));
  Print("\n");

  uint8_t* p = buf + sizeof(struct DNSMessage);
  for (int i = 0; i < htons(dns->num_questions); i++) {
    Print("Query: ");
    while (*p) {
      write(1, &p[1], *p);
      p += *p + 1;
      if (*p) {
        write(1, ".", 1);
      }
    }
    p++;
    p += 2;  // Type
    p += 2;  // Class
    Print("\n");
  }

  for (int i = 0; i < htons(dns->num_answers); i++){
    p += 2; // pointer
    p += 2;  // Type
    p += 2;  // Class
    p += 4;  // TTL
    p += 2;  // len (4)
  PrintIPv4Addr(*(uint32_t *)p);
    p += 4;
    Print("\n");
  }

  return 0;
}
