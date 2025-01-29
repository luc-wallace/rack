#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

List* list_init() {
  List* list = (List*)malloc(sizeof(List));
  list->head = NULL;
  list->length = 0;
  return list;
}

Node* list_append(List* list, void* data, size_t data_size) {
  Node* new_node = malloc(sizeof(Node));
  new_node->data = malloc(data_size);
  memcpy(new_node->data, data, data_size);

  if (list->length > 0) {
    new_node->next = list->head;
  }

  list->head = new_node;
  list->length++;

  return new_node;
}

Node* list_index(List* list, size_t index) {
  index = list->length - 1 - index;

  Node* node = list->head;
  size_t i = 0;

  while (1) {
    if (i == index) {
      return node;
    } else if ((node = node->next) == NULL) {
      break;
    }
    i++;
  }

  return NULL;
}

void list_free(List* list) {
  Node* head = list->head;
  Node* node;
  free(list);

  while (head != NULL) {
    node = head;
    head = head->next;
    free(node);
  }
}
