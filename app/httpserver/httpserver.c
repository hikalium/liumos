// HTTP server with UDP protocol.

#include "../liumlib/liumlib.h"

uint16_t port;

void StatusLine(char *response, int status) {
  switch (status) {
    case 200:
      strcpy(response, "HTTP/1.1 200 OK\n");
      break;
    case 404:
      strcpy(response, "HTTP/1.1 404 Not Found\n");
      break;
    case 500:
      strcpy(response, "HTTP/1.1 500 Internal Server Error\n");
      break;
    case 501:
      strcpy(response, "HTTP/1.1 501 Not Implemented\n");
      break;
    default:
      strcpy(response, "HTTP/1.1 404 Not Found\n");
  }
}

void Headers(char *response) {
  strcat(response, "Content-Type: text/html; charset=UTF-8\n");
}

void Crlf(char *response) {
  strcat(response, "\n");
}

void Body(char *response, char *message) {
  strcat(response, message);
}

void BuildResponse(char *response, int status, char *message) {
  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  StatusLine(response, status);
  Headers(response);
  Crlf(response);
  Body(response, message);
}

void Route(char *response, char *path) {
  if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
    char *body =
        "<html>\n"
        "  <body>\n"
        "    <h1>Hello World</h1>\n"
        "    <div>\n"
        "       <p>サンプルパラグラフです。</p>\n"
        "       <ul>\n"
        "           <li>リスト1</li>\n"
        "           <li>リスト2</li>\n"
        "       </ul>\n"
        "   </div>\n"
        " </body>\n"
        "</html>\n";
    BuildResponse(response, 200, body);
    return;
  }
  if (strcmp(path, "/example.html") == 0) {
    char *body =
        "<html>\n"
        "  <body>\n"
        "    <h1>Example Page</h1>\n"
        "    <div>\n"
        "       <p>サンプルパラグラフです。</p>\n"
        "       <ul>\n"
        "           <li>リスト1</li>\n"
        "           <li>リスト2</li>\n"
        "       </ul>\n"
        "   </div>\n"
        " </body>\n"
        "</html>\n";
    BuildResponse(response, 200, body);
    return;
  }
  char *body =
      "<html>\n"
      "  <body>\n"
      "    <p>Page is not found.</p>\n"
      " </body>\n"
      "</html>\n";
  BuildResponse(response, 404, body);
}

void Start(uint16_t port) {
  int socket_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) != 0) {
    Println("Error: Failed to bind socket");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  Print("Listening port: ");
  PrintNum(port);
  Print("\n");

  while (1) {
    Println("Log: Waiting for a request...\n");

    char request[SIZE_REQUEST];
    unsigned int len = sizeof(address);
    if (recvfrom(socket_fd, request, SIZE_REQUEST, MSG_WAITALL,
                 (struct sockaddr*) &address, &len) < 0) {
      Println("Error: Failed to receive a request.");
      close(socket_fd);
      exit(EXIT_FAILURE);
      return;
    }
    Println(request);

    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, " ");

    char* response = (char *) malloc(SIZE_RESPONSE);

    if (strcmp(method, "GET") == 0) {
      Route(response, path);
    } else {
      BuildResponse(response, 501, "Methods not GET are not supported.");
    }

    if (sendto(socket_fd, response, strlen(response), MSG_CONFIRM,
               (struct sockaddr *) &address, addrlen) == -1) {
      Println("Error: Failed to send a response.");
      close(socket_fd);
      exit(EXIT_FAILURE);
      return;
    }
  }

  close(socket_fd);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Println("Usage: httpserver.bin [ PORT ]");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  uint16_t port = StrToNum16(argv[1], NULL);
  Start(port);
  exit(0);
  return 0;
}
