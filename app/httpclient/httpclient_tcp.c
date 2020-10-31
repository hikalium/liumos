// HTTP client with TCP protocol.

#include "../liumlib/liumlib.h"

char *host = NULL;
char *path = NULL;
char *ip = NULL;
uint16_t port = 0;

void RequestLine(char *request) {
  strcpy(request, "GET ");
  strcat(request, path);
  strcat(request, " HTTP/1.1\n");
}

void Headers(char *request) {
  strcat(request, "Host: ");
  strcat(request, host);
  strcat(request, "\n");
}

void Crlf(char *request) {
  strcat(request, "\n");
}

void Body(char *request) {}

void SendRequest(char *request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char response[SIZE_RESPONSE];

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    Println("Error: Fail to create socket");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }

  struct sockaddr_in dst_address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(port);

  if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
    Println("Error: Fail to connect socket");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }

  if (sendto(socket_fd, request, strlen(request), MSG_CONFIRM,
             (struct sockaddr*)&address, addrlen) == -1) {
    Println("Error: Failed to send a request.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  Println("Request sent. Waiting for a response...");

  if (read(socket_fd, response, SIZE_RESPONSE)< 0) {
    Println("Error: Failed to receiver a response.");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  Println(response);

  close(socket_fd);
}

int main(int argc, char** argv) {
  if (argc != 1 && argc != 5) {
    Println("Usage: httpclient.bin [ IP PORT HOST PATH ]");
    Println("       IP, PORT, and URL are optional.");
    Println("       Default values are: IP=127.0.0.1, PORT=8888, HOST=Ø, PATH=/");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  if (argc == 5) {
    ip = argv[1];
    port = StrToNum16(argv[2], NULL);
    host = argv[3];
    path = argv[4];
  } else {
    ip = "127.0.0.1";
    port = 8888;
    host = "";
    path = "/";
  }

  char *request = (char *) malloc(SIZE_REQUEST);

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
