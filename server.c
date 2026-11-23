#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

static int SendAll(int client_socket, const char *data, size_t length) {
  size_t sent = 0;

  while (sent < length) {
    ssize_t result =
        send(client_socket, data + sent, length - sent, MSG_NOSIGNAL);
    if (result <= 0) {
      return -1;
    }
    sent += (size_t)result;
  }

  return 0;
}

static void SendResponse(int client_socket, const char *status,
                         const char *content_type, const char *body) {
  char header[BUFFER_SIZE];
  size_t body_length = strlen(body);
  int header_length =
      snprintf(header, sizeof(header),
               "HTTP/1.1 %s\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %zu\r\n"
               "Connection: close\r\n"
               "\r\n",
               status, content_type, body_length);

  if (header_length > 0) {
    SendAll(client_socket, header, (size_t)header_length);
  }
  SendAll(client_socket, body, body_length);
}

static void SendHtmlFile(int client_socket, const char *file_path) {
  FILE *html_file = fopen(file_path, "rb");
  if (html_file == NULL) {
    SendResponse(client_socket, "404 Not Found", "text/plain",
                 "404 Not Found\n");
    return;
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  char header[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n";

  SendAll(client_socket, header, strlen(header));
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, html_file)) > 0) {
    if (SendAll(client_socket, buffer, bytes_read) < 0) {
      break;
    }
  }
  fclose(html_file);
}

int main(int argc, char *argv[]) {
  int server_socket;
  int client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  int port = PORT;

  if (argc == 2) {
    char *end_pointer = NULL;
    errno = 0;
    long parsed_port = strtol(argv[1], &end_pointer, 10);
    if (errno != 0 || end_pointer == argv[1] || *end_pointer != '\0' ||
        parsed_port < 1 || parsed_port > 65535) {
      fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
      return EXIT_FAILURE;
    }
    port = (int)parsed_port;
  } else if (argc > 2) {
    fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
    return EXIT_FAILURE;
  }

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("No se pudo crear el socket");
    exit(EXIT_FAILURE);
  }

  int reuse_address = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
             sizeof(reuse_address));

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons((uint16_t)port);

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("No se pudo enlazar el puerto");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 5) < 0) {
    perror("No se pudo escuchar conexiones");
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  printf("Servidor escuchando en el puerto %d...\n", port);

  while (1) {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket < 0) {
      perror("No se pudo aceptar la conexion");
      continue;
    }

    char request[BUFFER_SIZE] = {0};
    ssize_t received = recv(client_socket, request, sizeof(request) - 1, 0);
    if (received > 0) {
      char method[16] = {0};
      char path[256] = {0};
      if (sscanf(request, "%15s %255s", method, path) == 2 &&
          strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0) {
          SendHtmlFile(client_socket, "index.html");
        } else {
          SendResponse(client_socket, "404 Not Found", "text/plain",
                       "404 Not Found\n");
        }
      } else {
        SendResponse(client_socket, "405 Method Not Allowed", "text/plain",
                     "405 Method Not Allowed\n");
      }
    }

    close(client_socket);
  }

  close(server_socket);
  return EXIT_SUCCESS;
}
