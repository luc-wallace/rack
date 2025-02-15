#ifndef server_h
#define server_h

#include "http.h"
#include <netinet/in.h>
#include <sys/socket.h>

typedef void (*HttpHandler)(HttpRequest* req, HttpResponse* res);

typedef struct HttpServer {
  int port;
  struct sockaddr_in address;
  HttpHandler handler;
  int socket;
} HttpServer;

typedef enum ParseStage {
  STAGE_METHOD,
  STAGE_PATH,
  STAGE_HEADER_NAME,
  STAGE_HEADER_VALUE,
  STAGE_BODY
} ParseStage;

HttpServer new_server(int port);

void launch(HttpServer* server);

void set_handler(HttpServer* server, HttpHandler handler);

#endif
