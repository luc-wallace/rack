#ifndef list_h
#define list_h

#include <stdlib.h>

typedef struct List {
  struct Node* head;
  size_t length;
} List;

typedef struct Node {
  void* data;
  struct Node* next;
} Node;

List* list_init();

Node* list_append(List* list, void* data, size_t data_size);

Node* list_index(List* list, size_t index);

void list_free(List* list);

#endif
