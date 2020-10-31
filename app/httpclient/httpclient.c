// HTTP client with UDP protocol.

#include "../liumlib/liumlib.h"

char* host;
char* path;
char* ip;
uint16_t port;

void RequestLine(char* request) {
  strcpy(request, "GET ");
  strcat(request, path);
  strcat(request, " HTTP/1.1\n");
}

void Headers(char* request) {
  strcat(request, "Host: ");
  strcat(request, host);
  strcat(request, "\n");
}

void Crlf(char* request) {
  strcat(request, "\n");
}

void Body(char* request) {}

void SendRequest(char* request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char response[SIZE_RESPONSE];

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    Println("Error: Fail to create socket");
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
    Println("Error: Failed to send a request.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  Println("Request sent. Waiting for a response...");

  unsigned int len = sizeof(address);
  if (recvfrom(socket_fd, response, SIZE_RESPONSE, MSG_WAITALL,
               (struct sockaddr*)&address, &len) < 0) {
    Println("Error: Failed to receiver a response.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  Println(response);

  close(socket_fd);
}

// Return 1 when parse succeeded, otherwise return 0.
int ParseArgs(int argc, char** argv) {
  // Set default values.
  ip = "127.0.0.1";
  port = 8888;
  host = "";
  path = "/";

  while (argc > 0) {
    if (strcmp("-ip", argv[0]) == 0) {
      ip = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("-port", argv[0]) == 0) {
      port = StrToNum16(argv[1], NULL);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("-host", argv[0]) == 0) {
      host = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("-path", argv[0]) == 0) {
      path = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    return 0;
  }
  return 1;
}

int main(int argc, char** argv) {
  if (ParseArgs(argc-1, argv+1) == 0) {
    Println("Usage: httpclient.bin [ OPTIONS ]");
    Println("       -ip      IP address. Default: 127.0.0.1");
    Println("       -port    Port number. Default: 8888");
    Println("       -host    Host property of the URL. Default: Ø");
    Println("       -path    Path property of the URL. Default: /");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  char* request = (char*) malloc(SIZE_REQUEST);

  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  RequestLine(request);
  Headers(request);
  Crlf(request);
  Body(request);

  Println("----- request -----");
  Println(request);
  Println("----- response -----");

  SendRequest(request);

  exit(0);
  return 0;
}
