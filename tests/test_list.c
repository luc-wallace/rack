#include "list.h"
#include <stdio.h>

int main() {
  List* list = list_init();

  int vals[] = {1, 2, 3, 4};
  size_t val_size = sizeof(int);

  // O(1) insertions
  list_append(list, vals, val_size);
  list_append(list, vals + 1, val_size);
  list_append(list, vals + 2, val_size);
  list_append(list, vals + 3, val_size);

  // O(1) length lookup
  printf("length: %d\n", list->length);

  // O(n) lookups
  int first = *(int*)list_index(list, 0)->data;
  int third = *(int*)list_index(list, 2)->data;
  printf("first: %d\nthird: %d\n", first, third);

  // iteration in reverse order as insertions are at the head
  Node* node = list->head;
  int i = 0;
  for (int i = 0; i < list->length; i++) {
    printf("%d\n", i, *(int*)node->data);
    if ((node = node->next) == NULL) {
      break;
    }
  }

  list_free(list);

  return 0;
}
