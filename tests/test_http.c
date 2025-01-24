#include "rack.h"
#include <stdio.h>
#include <stdlib.h>

char *get_home(struct Request *req) {
  return "<h1>Rack test</h1><p>Welcome to my website!</p>";
}

int main(int argc, char *argv[]) {
  int port;

  if (argc < 2) {
    port = 80;
  } else {
    port = atoi(argv[1]);
  }

  struct Server server =
      new_server(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 128);

  set_handler(&server, &get_home);

  launch(&server);
  return 0;
}
