// HTTP server with TCP protocol.

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

void StartServer() {
  int socket_fd, accepted_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) == -1) {
    write(1, "error: fail to bind socket\n", 27);
    close(socket_fd);
    exit(1);
    return;
  }

  if (listen(socket_fd, 3) < 0) {
    write(1, "error: fail to listen socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);
  while (1) {
    Println("Log: Waiting for a request...\n");

    if ((accepted_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1) {
      write(1, "error: fail to accept socket\n", 29);
      close(socket_fd);
      close(accepted_socket);
      exit(1);
      return;
    }

    char request[SIZE_REQUEST];
    int size = read(accepted_socket, request, SIZE_REQUEST);
    Println("----- request -----");
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

    sendto(accepted_socket, response, strlen(response), 0,
           (struct sockaddr *) &address, addrlen);

    close(accepted_socket);
  }
}

// Return 1 when parse succeeded, otherwise return 0.
int ParseArgs(int argc, char** argv) {
  // Set default values.
  port = 8888;

  while (argc > 0) {
    if (strcmp("-port", argv[0]) == 0) {
      port = StrToNum16(argv[1], NULL);
      argc -= 2;
      argv += 2;
      continue;
    }

    return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  if (ParseArgs(argc-1, argv+1) == 0) {
    Println("Usage: httpserver.bin [ OPTION ]");
    Println("       -port    Port number. Default: 8888");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  StartServer();

  exit(0);
  return 0;
}
