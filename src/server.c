#include "server.h"
#include "http.h"
#include "string.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define BACKLOG 10
#define MAX_EVENTS 10
#define BUFFER_SIZE 4096

// set socket to non-blocking
int set_non_blocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

HttpServer new_server(int port) {
  HttpServer server;
  ;
  server.port = port;

  server.address.sin_family = AF_INET;
  server.address.sin_port = htons(port);
  server.address.sin_addr.s_addr = htonl(INADDR_ANY);

  server.socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server.socket < 0) {
    perror("failed to connect to socket");
    exit(EXIT_FAILURE);
  }

  if (bind(server.socket, (struct sockaddr*)&server.address,
           sizeof(server.address)) < 0) {
    perror("failed to bind to socket");
    exit(EXIT_FAILURE);
  }

  if (listen(server.socket, BACKLOG) < 0) {
    perror("failed to listen to server");
    exit(EXIT_FAILURE);
  }

  return server;
}

void set_handler(HttpServer* server, HttpHandler handler) {
  server->handler = handler;
}

void handle_conn(HttpServer* server, int client_socket) {
  char* buffer = malloc(BUFFER_SIZE);
  size_t buffer_capacity = BUFFER_SIZE;
  unsigned int bytes_read = 0;
  int read_size;

  while (((read_size = read(client_socket, buffer + bytes_read,
                            BUFFER_SIZE - 1)) > 0)) {

    if (read_size < 0) {
      perror("read failed");
      free(buffer);
      close(client_socket);
      return;
    }

    bytes_read += read_size;
    if (bytes_read < buffer_capacity) {
      break;
    }

    char* new_buffer = realloc(buffer, bytes_read + BUFFER_SIZE);
    if (!new_buffer) {
      perror("realloc failed");
      free(buffer);
      close(client_socket);
      return;
    }
    buffer = new_buffer;
    buffer_capacity += BUFFER_SIZE;
  }

  if (bytes_read < 1) {
    close(client_socket);
    return;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    perror("read error");
    close(client_socket);
    return;
  }

  buffer[bytes_read] = '\0';

  if (bytes_read > 0) {
    printf("received:\n%s\n", buffer);
    free(buffer);

    HttpRequest* req = malloc(sizeof(HttpRequest));
    HttpResponse* res = malloc(sizeof(HttpResponse));

    res->header_count = 0;
    res->header_capacity = 16;
    res->headers = malloc(sizeof(HttpHeader) * res->header_capacity);
    res->body = "";

    // call server handler
    // TODO: check if handler is NULL
    server->handler(req, res);

    char content_length[20];
    sprintf(content_length, "%ld", strlen(res->body));
    add_header(res, "Content-Length", content_length);

    char* res_str = serialise_response(res);

    free(req);
    free(res->headers);
    free(res);

    write(client_socket, res_str, strlen(res_str));

    free(res_str);

    close(client_socket);
  } else if (bytes_read == 0) { // client disconnected
    close(client_socket);
  } else {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("read error");
      close(client_socket);
    }
  }
}

void launch(HttpServer* server) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("failed to create epoll instance");
    exit(EXIT_FAILURE);
  }

  if (set_non_blocking(server->socket) == -1) {
    perror("failed to set server socket to non-blocking");
    exit(EXIT_FAILURE);
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server->socket;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->socket, &event) == -1) {
    perror("failed to add server socket to epoll");
    exit(EXIT_FAILURE);
  }

  printf("server running and listening on port %d\n", server->port);

  struct epoll_event events[MAX_EVENTS];

  while (true) {
    int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (num_events < 0) {
      perror("epoll wait failed");
      break;
    }

    for (int i = 0; i < num_events; i++) {
      if (events[i].data.fd == server->socket) {
        while (true) {
          int client_socket = accept(server->socket, NULL, NULL);
          if (client_socket == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            perror("accept failed");
            break;
          }

          if (set_non_blocking(client_socket) == -1) {
            perror("failed to set client socket to non-blocking");
            close(client_socket);
            continue;
          }

          event.events = EPOLLIN | EPOLLET;
          event.data.fd = client_socket;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
            perror("failed to add client socket to epoll");
            close(client_socket);
          }
        }
      } else {
        int client_socket = events[i].data.fd;
        handle_conn(server, client_socket);
      }
    }
  }

  close(epoll_fd);
}