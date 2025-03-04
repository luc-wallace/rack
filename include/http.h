#ifndef http_h
#define http_h

#include "list.h"
#include <stddef.h>

#define HTTP_VERSION "HTTP/1.1"

typedef struct HttpHeader {
  const char* name;
  const char* value;
} HttpHeader;

typedef struct HttpRequest {
  const char* method;
  const char* path;
  const char* body;
  List* headers;
} HttpRequest;

typedef enum HttpStatusCode {
  HTTP_OK = 200,
  HTTP_CREATED = 201,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
  HTTP_INTERNAL_SERVER_ERROR = 500
  // TODO: add all status codes
} HttpStatusCode;

typedef struct HttpResponse {
  HttpStatusCode status_code;
  List* headers;
  char* body;
} HttpResponse;

char* serialise_response(HttpResponse* res);

void add_header(HttpResponse* res, char name[], char value[]);

void set_status_code(HttpResponse* res, HttpStatusCode code);

#endif
