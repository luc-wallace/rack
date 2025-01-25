#ifndef http_h
#define http_h

typedef enum HttpMethod {
  HTTP_GET,
  HTTP_HEAD,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_CONNECT,
  HTTP_OPTIONS,
  HTTP_TRACE,
  HTTP_PATCH
} HttpMethod;

typedef struct HttpHeader {
  const char* name;
  const char* value;
} HttpHeader;

typedef struct HttpRequest {
  HttpMethod method;
  const char* path;
  const char* body;
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
  HttpHeader* headers;
  size_t header_count;

  char* body;
} HttpResponse;

#endif
