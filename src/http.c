#include "http.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 2048

char* status_code_to_str(HttpStatusCode code) {
  switch (code) {
  case HTTP_OK:
    return "OK";
  case HTTP_CREATED:
    return "Created";
  case HTTP_BAD_REQUEST:
    return "Bad Request";
  case HTTP_NOT_FOUND:
    return "Not Found";
  case HTTP_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  default:
    return "Internal Server Error";
  }
}

void add_header(HttpResponse* res, char name[], char value[]) {
  if (res->header_count >= res->header_capacity) {
    size_t new_capacity = res->header_capacity * 2; // Double the capacity
    HttpHeader* new_headers =
        realloc(res->headers, sizeof(HttpHeader) * new_capacity);
    res->headers = new_headers;
    res->header_capacity = new_capacity;
  }

  res->headers[res->header_count] = (HttpHeader){name, value};
  res->header_count += 1;
}

void set_status_code(HttpResponse* res, HttpStatusCode code) {
  res->status_code = code;
}

char* serialise_response(HttpResponse* res) {
  size_t estimated_size = strlen(HTTP_VERSION) + 64 +
                          strlen(status_code_to_str(res->status_code)) + 1;

  for (size_t i = 0; i < res->header_count; i++) {
    estimated_size += strlen(res->headers[i].name) +
                      strlen(res->headers[i].value) + 4; // ": " + "\r\n"
  }

  estimated_size += strlen(res->body) + 4;

  char* buffer = malloc(estimated_size);
  if (!buffer) {
    perror("Memory allocation failed for response buffer");
    return NULL;
  }

  char* ptr = buffer;

  int written =
      snprintf(ptr, estimated_size, "%s %d %s", HTTP_VERSION, res->status_code,
               status_code_to_str(res->status_code));
  if (written < 0) {
    free(buffer);
    return NULL;
  }
  ptr += written;

  for (size_t i = 0; i < res->header_count; i++) {
    written = snprintf(ptr, estimated_size - (ptr - buffer), "\r\n%s: %s",
                       res->headers[i].name, res->headers[i].value);
    if (written < 0) {
      free(buffer);
      return NULL;
    }
    ptr += written;
  }

  snprintf(ptr, estimated_size - (ptr - buffer), "\r\n\r\n");
  ptr += 4;

  snprintf(ptr, estimated_size - (ptr - buffer), "%s", res->body);

  return buffer;
}