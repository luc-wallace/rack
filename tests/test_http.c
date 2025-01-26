#include "rack.h"
#include <stdio.h>
#include <stdlib.h>

void request_handler(HttpRequest* req, HttpResponse* res) {
  res->body = "<h1>Rack test</h1><p>Welcome to my website!</p>";

  add_header(res, "Content-Type", "text/html");
  add_header(res, "Connection", "keep-alive");

  res->status_code = HTTP_OK;
}

int main(int argc, char* argv[]) {
  int port;

  if (argc < 2) {
    port = 80;
  } else {
    port = atoi(argv[1]);
  }

  HttpServer server = new_server(port);
  set_handler(&server, &request_handler);
  launch(&server);

  return 0;
}
