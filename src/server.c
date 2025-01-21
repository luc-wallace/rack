#include "server.h"
#include "string.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

struct ThreadArgs {
  struct Server* server;
  int socket;
};

void* handle_conn(void* arg) {
  struct ThreadArgs* args = (struct ThreadArgs*)arg;
  struct Server* server = args->server;
  int socket = args->socket;

  char buffer[30000];
  read(socket, buffer, 30000);
  puts(buffer);

  struct Request req;

  char* body = server->handler(&req);

  char res[300 + strlen(body)];
  strcat(res, "HTTP/1.1 200 OK\n"
              "Date: Mon, 21 Jan 2025 12:00:00 GMT\n"
              "Server: MySimpleServer/1.0\n"
              "Content-Type: text/html\n"
              "Connection: close\n"
              "Content-Length: ");

  char content_length[20];
  sprintf(content_length, "%ld\n\n", strlen(body));

  strcat(strcat(res, content_length), body);

  write(socket, res, strlen(res));
  close(socket);

  return NULL;
}

void launch(struct Server* server) {
  while (true) {
    int address_length = sizeof(server->address);
    int new_socket = accept(server->socket, (struct sockaddr*)&server->address,
                            (socklen_t*)&address_length);

    struct ThreadArgs* args = malloc(sizeof(struct ThreadArgs));
    args->server = server;
    args->socket = new_socket;

    pthread_t thread;
    pthread_create(&thread, NULL, handle_conn, args);
    pthread_join(thread, NULL);
  }
}
