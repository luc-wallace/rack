#include <netinet/in.h>
#include <sys/socket.h>
#include "http.h"

#ifndef server_h
#define server_h

struct Server {
  int domain;
  int service;
  int protocol;
  unsigned long interface;
  int port;
  int backlog;

  struct sockaddr_in address;

  char* (*handler)(struct Request* req);

  int socket;
};

struct Server new_server(int domain, int service, int protocol,
                         unsigned long interface, int port, int backlog);

void launch(struct Server *server);

void set_handler(struct Server *server, char *(*handler)(struct Request *req));

#endif
