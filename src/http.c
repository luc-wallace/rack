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
  if (res->headers == NULL) {
    return;
  }

  size_t header_size = sizeof(HttpHeader);
  HttpHeader* header = malloc(header_size);
  header->name = name;
  header->value = value;

  list_append(res->headers, header, header_size);
}

void set_status_code(HttpResponse* res, HttpStatusCode code) {
  res->status_code = code;
}

char* serialise_response(HttpResponse* res) {
  size_t estimated_size = strlen(HTTP_VERSION) + 64 +
                          strlen(status_code_to_str(res->status_code)) + 1;
  List* headers_list = res->headers;
  Node* node = headers_list->head;
  HttpHeader header;

  for (int i = 0; i < headers_list->length; i++) {
    header = *(HttpHeader*)node->data;
    estimated_size += strlen(header.name) + strlen(header.value) + 4;

    if ((node = node->next) == NULL) {
      break;
    }
  }

  estimated_size += strlen(res->body) + 4;

  char* buffer = malloc(estimated_size);
  if (!buffer) {
    perror("failed to allocate memory for response buffer");
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

  node = headers_list->head;
  int i = 0;

  for (int i = 0; i < headers_list->length; i++) {
    header = *(HttpHeader*)node->data;
    written = snprintf(ptr, estimated_size - (ptr - buffer), "\r\n%s: %s",
                       header.name, header.value);
    if (written < 0) {
      free(buffer);
      return NULL;
    }
    ptr += written;
    if ((node = node->next) == NULL) {
      break;
    }
  }

  snprintf(ptr, estimated_size - (ptr - buffer), "\r\n\r\n");
  ptr += 4;

  snprintf(ptr, estimated_size - (ptr - buffer), "%s", res->body);

  return buffer;
}
