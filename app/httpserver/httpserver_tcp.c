// HTTP server with TCP.

#include "../liumlib/liumlib.h"

void status_line(char *response, int status) {
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

void headers(char *response) {
  strcat(response, "Content-Type: text/html; charset=UTF-8\n");
}

void crlf(char *response) {
  strcat(response, "\n");
}

void body(char *response, char *message) {
  strcat(response, message);
}

void build_response(char *response, int status, char *message) {
  // c.f.
  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  status_line(response, status);
  headers(response);
  crlf(response);
  body(response, message);
}

void route(char *response, char *path) {
  if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
    char *body =
        "<html>"
        "  <body>"
        "    <h1>Hello World</h1>"
        "    <div>"
        "       <p>サンプルパラグラフです。</p>"
        "       <ul>"
        "           <li>リスト1</li>"
        "           <li>リスト2</li>"
        "       </ul>"
        "   </div>"
        " </body>"
        "</html>";
    build_response(response, 200, body);
    return;
  }
  if (strcmp(path, "/example.html") == 0) {
    build_response(response, 200, "<html><body><h1>example page</h1><ul><li>abc</li><li>def</li></ul></body></html>");
    return;
  }
  build_response(response, 404, "<body><p>Page is not found.</p></body>");
}

void start() {
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
    write(1, "LOG: wait a message from client\n", 32);
    if ((accepted_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1) {
      write(1, "error: fail to accept socket\n", 29);
      close(socket_fd);
      close(accepted_socket);
      exit(1);
      return;
    }

    char request[SIZE_REQUEST];
    int size = read(accepted_socket, request, SIZE_REQUEST);
    write(1, request, size);

    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, " ");

    char* response = (char *) malloc(SIZE_RESPONSE);

    if (strcmp(method, "GET") == 0) {
      route(response, path);
    } else {
      build_response(response, 501, "Methods not GET are not supported.");
    }

    sendto(accepted_socket, response, strlen(response), 0, (struct sockaddr *) &address, addrlen);

    close(accepted_socket);
  }
}

int main(int argc, char *argv[]) {
  start();
  exit(0);
  return 0;
}
