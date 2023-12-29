#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

struct Array {
  void         *data;
  unsigned long size;
  unsigned long count;
  unsigned long capacity;
};

Array *array_new(unsigned long size)
{
  return array_new_with_capacity(size, 0);
}

Array *array_new_with_capacity(unsigned long size, unsigned long capacity)
{
  Array *array    = malloc(sizeof(Array));
  array->data     = NULL;
  array->size     = size;
  array->count    = 0;
  array->capacity = 0;
  array_reserve(array, capacity);
  return array;
}

void array_free(Array *array)
{
  if (array) {
    free(array->data);
    free(array);
  }
}

unsigned long array_count(const Array *array)
{
  return array->count;
}

unsigned long array_capacity(const Array *array)
{
  return array->capacity;
}

void *array_data(const Array *array)
{
  return array->data;
}

void *array_at(const Array *array, unsigned long index)
{
  return (char *) array->data + array->size * index;
}

void *array_front(const Array *array)
{
  return array_at(array, 0);
}

void *array_back(const Array *array)
{
  return array_at(array, array->count - 1);
}

void array_reserve(Array *array, unsigned long capacity)
{
  if (array->capacity < capacity) {
    void *new = realloc(array->data, array->size * capacity);
    if (!new) {
      fprintf(stderr, "Internal Error: Failed to allocate memory\n");
      exit(EXIT_FAILURE);
    }
    array->data     = new;
    array->capacity = capacity;
  }
}

void array_fit(Array *array)
{
  void *new = realloc(array->data, array->size * array->count);
  if (new) {
    array->data     = new;
    array->capacity = array->count;
  }
}

void array_push(Array *array, void *value)
{
  array_push_count(array, value, 1);
}

void array_push_count(Array *array, void *value, unsigned long count)
{
  if (array_count(array) + count > array_capacity(array)) {
    unsigned long capacity = array_capacity(array) ? array_capacity(array) : 1;
    while (array_count(array) + count > capacity) {
      capacity *= 2;
    }
    array_reserve(array, capacity);
  }
  memcpy(array_at(array, array_count(array)), value, array->size * count);
  array->count += count;
}

void array_pop(Array *array)
{
  array_pop_count(array, 1);
}

void array_pop_count(Array *array, unsigned long count)
{
  if (array->count > count) {
    array->count -= count;
  } else {
    array->count = 0;
  }
}

void array_clear(Array *array)
{
  array->count = 0;
}

void *array_steal(Array *array)
{
  void *result    = array->data;
  array->data     = NULL;
  array->capacity = 0;
  array->count    = 0;
  array_free(array);
  return result;
}
