// HTTP client with UDP/TCP protocol.

#include "../liumlib/liumlib.h"

char *host;
char *path;
char *ip;
uint16_t port;
bool tcp;

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

  // In TCP, the second argument of socket() should be `SOCK_STREAM`.
  // In UDP, the second argument of socket() should be `SOCK_DGRAM`.
  if (tcp) {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  } else {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  }
  if (socket_fd < 0) {
    Println("Error: Failed to create a socket");
    exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(port);

  // In TCP, connect() should be called.
  if (tcp) {
    if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
      Println("Error: Fail to connect a socket");
      exit(EXIT_FAILURE);
    }
  }

  if (sendto(socket_fd, request, strlen(request), 0,
             (struct sockaddr*)&address, addrlen) < 0) {
    Println("Error: Failed to send a request.");
    exit(EXIT_FAILURE);
  }
  Println("Request sent. Waiting for a response...");

  unsigned int len = sizeof(address);
  if (recvfrom(socket_fd, response, SIZE_RESPONSE, 0,
               (struct sockaddr*)&address, &len) < 0) {
    Println("Error: Failed to receive a response.");
    exit(EXIT_FAILURE);
  }
  Println("----- response -----");
  Println(response);

  close(socket_fd);
}

// Return true when parse succeeded, otherwise return false.
bool ParseArgs(int argc, char **argv) {
  // Set default values.
  ip = "127.0.0.1";
  port = 8888;
  host = "";
  path = "/";
  tcp = false;

  while (argc > 0) {
    if (strcmp("--ip", argv[0]) == 0 || strcmp("-i", argv[0]) == 0) {
      ip = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("--port", argv[0]) == 0 || strcmp("-p", argv[0]) == 0) {
      port = StrToNum16(argv[1], NULL);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("--host", argv[0]) == 0 || strcmp("-h", argv[0]) == 0) {
      host = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("--path", argv[0]) == 0 || strcmp("-P", argv[0]) == 0) {
      path = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("--tcp", argv[0]) == 0) {
      tcp = true;
      argc -= 1;
      argv += 1;
      continue;
    }

    return false;
  }
  return true;
}

int main(int argc, char **argv) {
  if (!ParseArgs(argc - 1, argv + 1)) {
    Println("Usage: httpclient.bin [ OPTIONS ]");
    Println("       -i, --ip      IP address. Default: 127.0.0.1");
    Println("       -p, --port    Port number. Default: 8888");
    Println("       -h, --host    Host property of the URL. Default: Ø");
    Println("       -P, --path    Path property of the URL. Default: /");
    Println("           --tcp     Flag to use TCP. Use UDP when it doesn't exist.");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  if (tcp)
    Println("Log: Using protocol: TCP");
  else
    Println("Log: Using protocol: UDP");

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

  SendRequest(request);

  exit(0);
  return 0;
}
