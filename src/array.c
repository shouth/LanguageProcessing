#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

void array_init(Array *array, unsigned long size)
{
  array_init_with_capacity(array, size, 0);
}

void array_init_with_capacity(Array *array, unsigned long size, unsigned long capacity)
{
  array->_data     = NULL;
  array->_size     = size;
  array->_count    = 0;
  array->_capacity = 0;
  array_reserve(array, capacity);
}

void array_deinit(Array *array)
{
  free(array->_data);
}

unsigned long array_count(const Array *array)
{
  return array->_count;
}

unsigned long array_capacity(const Array *array)
{
  return array->_capacity;
}

void *array_data(const Array *array)
{
  return array->_data;
}

void *array_at(const Array *array, unsigned long index)
{
  return (char *) array->_data + array->_size * index;
}

void *array_front(const Array *array)
{
  return array_at(array, 0);
}

void *array_back(const Array *array)
{
  return array_at(array, array->_count - 1);
}

void array_reserve(Array *array, unsigned long capacity)
{
  if (array->_capacity < capacity) {
    void *new = realloc(array->_data, array->_size * capacity);
    if (!new) {
      fprintf(stderr, "Internal Error: Failed to allocate memory\n");
      exit(EXIT_FAILURE);
    }
    array->_data     = new;
    array->_capacity = capacity;
  }
}

void array_fit(Array *array)
{
  void *new = realloc(array->_data, array->_size * array->_count);
  if (new) {
    array->_data     = new;
    array->_capacity = array->_count;
  }
}

void array_push(Array *array, void *value)
{
  array_push_n(array, value, 1);
}

void array_push_n(Array *array, void *value, unsigned long count)
{
  if (array_count(array) + count > array_capacity(array)) {
    unsigned long capacity = array_capacity(array) ? array_capacity(array) : 1;
    while (array_count(array) + count > capacity) {
      capacity *= 2;
    }
    array_reserve(array, capacity);
  }
  memcpy(array_at(array, array_count(array)), value, array->_size * count);
  array->_count += count;
}

void array_pop(Array *array)
{
  array_pop_n(array, 1);
}

void array_pop_n(Array *array, unsigned long count)
{
  if (array->_count > count) {
    array->_count -= count;
  } else {
    array->_count = 0;
  }
}

void array_clear(Array *array)
{
  array->_count = 0;
}

void *array_steal(Array *array)
{
  void *result     = array->_data;
  array->_data     = NULL;
  array->_capacity = 0;
  array->_count    = 0;
  return result;
}
