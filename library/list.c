#include "list.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * The factor by which capacity will increase when being resized.
 * Must be at least 1. A value of 1 means the capacity increases by 1 upon a
 * resize.
 */
const size_t GROW_FACTOR = 2;

struct list {
  void **objects;
  size_t capacity;
  size_t size;
  free_func_t freer;
};

list_t *list_init(size_t initial_size, free_func_t freer) {
  assert(initial_size >= 0);

  list_t *list = malloc(sizeof(list_t));
  list->objects = malloc(sizeof(void *) * initial_size);
  list->capacity = initial_size;
  list->size = 0;
  list->freer = freer;

  return list;
}

void list_free(list_t *list) {
  size_t size = list->size;
  if (list->freer != NULL) {
    for (size_t i = 0; i < size; ++i) {
      list->freer(list->objects[i]);
    }
  }
  free(list->objects);
  free(list);
}

size_t list_size(list_t *list) { return list->size; }

void *list_get(list_t *list, size_t index) {
  assert(index < list->size);

  return list->objects[index];
}

/**
 * Increases the capacity of a list by a factor of GROW_FACTOR if the list is
 * full. A list is considered full iff its size equals its capacity. If the list
 * is not full, this function will do nothing. Otherwise, the capacity will
 * increase.
 */
void list_resize_if_full(list_t *list) {
  // Return if the list is not full.
  if (list->size != list->capacity) {
    return;
  }

  // Calculate the new capacity
  size_t new_capacity = GROW_FACTOR * list->capacity;
  if (new_capacity == list->capacity) {
    new_capacity += 1;
  }

  // Update the list
  list->objects = realloc(list->objects, sizeof(void *) * new_capacity);
  list->capacity = new_capacity;
}

void *list_remove(list_t *list, size_t index) {
  assert(list->size != 0);
  assert(index < list->size);

  void *removed = list->objects[index];
  if (index < list->size - 1) {
    memmove(list->objects + index, list->objects + index + 1,
            sizeof(void *) * ((list->size - 1) - index));
  }
  --list->size;

  return removed;
}

void list_add(list_t *list, void *value) {
  assert(value != NULL);

  list_resize_if_full(list);

  list->objects[list->size] = value;
  ++list->size;
}
