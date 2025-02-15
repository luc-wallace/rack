#include "server.h"
#include "http.h"

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

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  getsockname(server.socket, (struct sockaddr*)&sin, &len);
  
  printf("server address: http://localhost:%d\n", ntohs(sin.sin_port));

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
    HttpRequest req;
    req.headers = list_init();

    ParseStage parse_stage = STAGE_METHOD;

    char* method;
    char* path;

    HttpHeader* cur_header;

    // index being addressed in buffer
    size_t buf_index = 0;
    size_t cursor_tail = 0;
    for (size_t cursor_head = 0; cursor_head < bytes_read; cursor_head++) {
      switch (parse_stage) {
      case STAGE_METHOD:
        if (cursor_head > 7) {
          goto malformed_request; // invalid http methods
        }
        if (buffer[cursor_head] == ' ') {
          size_t size = cursor_head - cursor_tail;

          method = malloc((size + 1) * sizeof(char));
          memcpy(method, &buffer[cursor_tail], size);

          method[size] = '\0';
          parse_stage = STAGE_PATH;
          cursor_tail = cursor_head + 1;
          buf_index = 0;

          continue;
        }
        break;

      case STAGE_PATH:
        if (buffer[cursor_head] == ' ' || buffer[cursor_head] == '\n') {
          size_t size = cursor_head - cursor_tail;
          path = malloc((size + 1) * sizeof(char));
          memcpy(path, &buffer[cursor_tail], size);
          path[size] = '\0';
          parse_stage = STAGE_HEADER_NAME;
          cursor_tail = cursor_head + 1;
          continue;
        }
        break;

      case STAGE_HEADER_NAME:
        break;
      }

      if (parse_stage == STAGE_HEADER_NAME) {
        break;
      }
      buf_index++;
    }

    printf("%s %s\n", method, path);

    HttpResponse res;

    res.headers = list_init();
    res.body = "";

    // call server handler
    // TODO: check if handler is NULL
    server->handler(&req, &res);

    char content_length[20];
    sprintf(content_length, "%ld", strlen(res.body));
    add_header(&res, "Content-Length", content_length);

    char* res_str = serialise_response(&res);
    write(client_socket, res_str, strlen(res_str));

    free(res_str);
    list_free(res.headers);

    if (0) {
    malformed_request: // unified error handler for malformed HTTP requests
      HttpResponse err_res;

      err_res.status_code = HTTP_BAD_REQUEST;
      err_res.body = "http error: malformed request";
      add_header(&err_res, "Content-Type", "text");
      add_header(&err_res, "Connection", "close");

      char* res_str = serialise_response(&err_res);
      write(client_socket, res_str, strlen(res_str));
      free(res_str);
      list_free(err_res.headers);
    }

    list_free(req.headers);
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

  printf("server running\n");

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