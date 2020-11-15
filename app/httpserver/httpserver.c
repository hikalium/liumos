// HTTP server with UDP protocol.

#include "../liumlib/liumlib.h"

uint16_t port;
bool tcp;

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
        "      <p>This is a sample paragraph.</p>\n"
        "      <ul>\n"
        "        <li>List 1</li>\n"
        "        <li>List 2</li>\n"
        "        <li>List 3</li>\n"
        "      </ul>\n"
        "    </div>\n"
        "  </body>\n"
        "</html>\n";
    BuildResponse(response, 200, body);
    return;
  }
  if (strcmp(path, "/page1.html") == 0) {
    char *body =
        "<html>\n"
        "  <body>\n"
        "    <h1>Example Page 1</h1>\n"
        "    <div>\n"
        "      <p>This is a sample paragraph.</p>\n"
        "      <ul>\n"
        "        <li>List 1</li>\n"
        "        <li>List 2</li>\n"
        "      </ul>\n"
        "      <ul>\n"
        "        <li>List 3</li>\n"
        "        <li>List 4</li>\n"
        "      </ul>\n"
        "    </div>\n"
        "  </body>\n"
        "</html>\n";
    BuildResponse(response, 200, body);
    return;
  }
  if (strcmp(path, "/page2.html") == 0) {
    char *body =
        "<html>\n"
        "  <body>\n"
        "    <h1>Example Page 2</h1>\n"
        "    <div>\n"
        "      <p>This is the first paragraph.</p>\n"
        "    </div>\n"
        "    <ul>\n"
        "      <li>List 1</li>\n"
        "      <li>List 2</li>\n"
        "    </ul>\n"
        "    <div>\n"
        "      <p>This is the second paragraph.</p>\n"
        "    </div>\n"
        "  </body>\n"
        "</html>\n";
    BuildResponse(response, 200, body);
    return;
  }
  char *body =
      "<html>\n"
      "  <body>\n"
      "    <h1>Error!</h1>\n"
      "    <p>Page is not found.</p>\n"
      " </body>\n"
      "</html>\n";
  BuildResponse(response, 404, body);
}

void StartServer() {
  int socket_fd, accepted_socket;
  struct sockaddr_in address;
  unsigned int addrlen = sizeof(address);

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
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) < 0) {
    Println("Error: Failed to bind a socket");
    exit(EXIT_FAILURE);
  }

  // In TCP, listen() should be called.
  if (tcp) {
    if (listen(socket_fd, 3) < 0) {
      Println("Error: Failed to listen a socket");
      exit(1);
    }
  }

  while (1) {
    Println("Log: Waiting for a request...\n");

    char request[SIZE_REQUEST];
    int size = -1;

    // In TCP, a request is received by accept() and read().
    // In UDP, a request is received by recvfrom().
    if (tcp) {
      if ((accepted_socket = accept(socket_fd, (struct sockaddr *)&address,
                                    (socklen_t*)&addrlen)) < 0) {
        Println("Error: Failed to accept a socket.");
        exit(1);
      }

      size = read(accepted_socket, request, SIZE_REQUEST);
    } else {
      size = recvfrom(socket_fd, request, SIZE_REQUEST, 0,
                      (struct sockaddr*) &address, &addrlen);
    }
    if (size < 0) {
      Println("Error: Failed to receive a request.");
      exit(EXIT_FAILURE);
    }

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

    size = -1;
    // In TCP, a response is sent to `accepted_socket`.
    // In UDP, a response is sent to `socket_fd`.
    if (tcp) {
      size = sendto(accepted_socket, response, strlen(response), 0,
                    (struct sockaddr *) &address, addrlen);
    } else {
      size = sendto(socket_fd, response, strlen(response), 0,
                    (struct sockaddr *) &address, addrlen);
    }
    if (size < 0) {
      Println("Error: Failed to send a response.");
      exit(EXIT_FAILURE);
    }

    // In TCP, an accepted socket should be closed.
    if (tcp) {
      close(accepted_socket);
    }
  }

  close(socket_fd);
}

// Return true when parse succeeded, otherwise return false.
bool ParseArgs(int argc, char **argv) {
  // Set default values.
  port = 8888;
  tcp = false;

  while (argc > 0) {
    if (strcmp("--port", argv[0]) == 0 || strcmp("-p", argv[0]) == 0) {
      port = StrToNum16(argv[1], NULL);
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
    Println("Usage: httpserver.bin [ OPTION ]");
    Println("       -p, --port    Port number. Default: 8888");
    Println("           --tcp     Flag to use TCP. Use UDP when it doesn't exist.");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  if (tcp)
    Println("Log: Using protocol: TCP");
  else
    Println("Log: Using protocol: UDP");
  Print("Log: Listening port: ");
  PrintNum(port);
  Print("\n");

  StartServer();

  exit(0);
  return 0;
}
