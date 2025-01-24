#include "server.h"
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

#define MAX_EVENTS 10
#define BUFFER_SIZE 4096

// set socket to non-blocking
int set_non_blocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

struct Server new_server(int domain, int service, int protocol,
                         unsigned long interface, int port, int backlog) {
  struct Server server;

  server.domain = domain;
  server.service = service;
  server.protocol = protocol;
  server.interface = interface;
  server.port = port;
  server.backlog = backlog;

  server.address.sin_family = domain;
  server.address.sin_port = htons(port);
  server.address.sin_addr.s_addr = htonl(interface);

  server.socket = socket(domain, service, protocol);
  if (server.socket < 0) {
    perror("failed to connect to socket");
    exit(EXIT_FAILURE);
  }

  if (bind(server.socket, (struct sockaddr*)&server.address,
           sizeof(server.address)) < 0) {
    perror("failed to bind to socket");
    exit(EXIT_FAILURE);
  }

  if (listen(server.socket, server.backlog) < 0) {
    perror("failed to listen to server");
    exit(EXIT_FAILURE);
  }

  return server;
}

void set_handler(struct Server* server, char* (*handler)(struct Request* req)) {
  server->handler = handler;
}

void handle_conn(struct Server* server, int client_socket) {
  char* buffer = malloc(BUFFER_SIZE);
  size_t buffer_capacity = BUFFER_SIZE;
  int bytes_read = 0;
  int read_size;

  while (((read_size = read(client_socket, buffer + bytes_read,
                            BUFFER_SIZE - 1)) > 0)) {
    bytes_read += read_size;

    if (read_size < 0) {
      perror("read failed");
      free(buffer);
      close(client_socket);
      return;
    } else if (bytes_read < sizeof(buffer)) {
      break;
    }

    char* new_buffer = realloc(buffer, bytes_read + BUFFER_SIZE + 1);
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
    buffer[bytes_read] = '\0'; // null terminate received data
    printf("received:\n%s\n", buffer);

    // Process the request using the server's handler
    struct Request req; // Request is an empty struct
    char* body = server->handler(&req);

    // Build the HTTP response
    char res[300 + strlen(body)];
    strcpy(res, "HTTP/1.1 200 OK\n"
                "Date: Mon, 21 Jan 2025 12:00:00 GMT\n"
                "Server: rack/1.0\n"
                "Content-Type: text/html\n"
                "Connection: keep-alive\n"
                "Content-Length: ");

    char content_length[20];
    sprintf(content_length, "%ld\n\n", strlen(body));

    strcat(strcat(res, content_length), body);

    write(client_socket, res, strlen(res));
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

void launch(struct Server* server) {
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