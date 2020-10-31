// HTTP client with UDP protocol.

#include "../liumlib/liumlib.h"
#include "lib.h"

char* host = NULL;
char* path = NULL;
char* ip = NULL;
uint16_t port = 0;

void request_line(char* request) {
  strcpy(request, "GET ");
  strcat(request, path);
  strcat(request, " HTTP/1.1\n");
}

void headers(char* request) {
  strcat(request, "Host: ");
  strcat(request, host);
  strcat(request, "\n");
}

void crlf(char* request) {
  strcat(request, "\n");
}

void body(char* request) {}

void send_request(char* request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char response[SIZE_RESPONSE];

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    println("Error: Fail to create socket");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }

  struct sockaddr_in dst_address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(port);

  if (sendto(socket_fd, request, strlen(request), MSG_CONFIRM,
             (struct sockaddr*)&address, addrlen) == -1) {
    println("Error: Failed to send a request.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  println("Request sent. Waiting for a response...");

  unsigned int len = sizeof(address);
  if (recvfrom(socket_fd, response, SIZE_RESPONSE, MSG_WAITALL,
               (struct sockaddr*)&address, &len) < 0) {
    println("Error: Failed to receiver a response.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  println(response);

  close(socket_fd);
}

int main(int argc, char* argv[]) {
  if (argc != 1 && argc != 5) {
    println("Usage: httpclient.bin [ IP PORT HOST PATH ]");
    println("       IP, PORT, and URL are optional.");
    println("       Default values are: IP=127.0.0.1, PORT=8888, HOST=Ø, PATH=/");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  if (argc == 5) {
    ip = argv[1];
    port = str_to_num16(argv[2], NULL);
    host = argv[3];
    path = argv[4];
  } else {
    ip = "127.0.0.1";
    port = 8888;
    host = "";
    path = "/";
  }

  char* request = (char*) malloc(SIZE_REQUEST);

  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  request_line(request);
  headers(request);
  crlf(request);
  body(request);

  println("----- request -----");
  println(request);
  println("----- response -----");

  send_request(request);

  exit(0);
  return 0;
}
