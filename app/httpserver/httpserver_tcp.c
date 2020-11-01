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
    default:
      strcpy(response, "HTTP/1.1 500 Internal Server Error\n");
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
  unsigned int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Println("Error: Failed to create a socket");
    exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) == -1) {
    Println("Error: Failed to bind a socket");
    exit(1);
  }

  if (listen(socket_fd, 3) < 0) {
    Println("Error: Failed to listen a socket");
    exit(1);
  }

  Print("Listening port: ");
  PrintNum(port);
  Print("\n");

  while (1) {
    Println("Log: Waiting for a request...\n");

    if ((accepted_socket = accept(socket_fd, (struct sockaddr *)&address,
                                  (socklen_t*)&addrlen)) < 0) {
      Println("Error: Failed to accept a socket.");
      exit(1);
    }

    char request[SIZE_REQUEST];
    int size = read(accepted_socket, request, SIZE_REQUEST);

    Println("----- request -----");
    Println(request);

    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");

    char* response = (char *) malloc(SIZE_RESPONSE);

    if (strcmp(method, "GET") == 0) {
      Route(response, path);
    } else {
      BuildResponse(response, 500, "Only GET method is supported.");
    }

    if (sendto(accepted_socket, response, strlen(response), 0,
               (struct sockaddr *) &address, addrlen) < 0) {
      Println("Error: Failed to send a response.");
      exit(EXIT_FAILURE);
    }

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
