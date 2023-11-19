#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

void vector_init(Vector *vector, unsigned long size)
{
  vector_init_with_capacity(vector, size, 0);
}

void vector_init_with_capacity(Vector *vector, unsigned long size, unsigned long capacity)
{
  vector->_data     = NULL;
  vector->_size     = size;
  vector->_count    = 0;
  vector->_capacity = 0;
  vector_reserve(vector, capacity);
}

void vector_deinit(Vector *vector)
{
  free(vector->_data);
}

unsigned long vector_count(const Vector *vector)
{
  return vector->_count;
}

unsigned long vector_capacity(const Vector *vector)
{
  return vector->_capacity;
}

void *vector_data(const Vector *vector)
{
  return vector->_data;
}

void *vector_at(const Vector *vector, unsigned long index)
{
  return (char *) vector->_data + vector->_size * index;
}

void *vector_back(Vector *vector)
{
  return vector_at(vector, vector->_count - 1);
}

void vector_reserve(Vector *vector, unsigned long capacity)
{
  if (vector->_capacity < capacity) {
    void *new = realloc(vector->_data, vector->_size * capacity);
    if (!new) {
      fprintf(stderr, "Internal Error: Failed to allocate memory\n");
      exit(EXIT_FAILURE);
    }
    vector->_data     = new;
    vector->_capacity = capacity;
  }
}

void vector_fit(Vector *vector)
{
  void *new = realloc(vector->_data, vector->_size * vector->_count);
  if (new) {
    vector->_data     = new;
    vector->_capacity = vector->_count;
  }
}

void vector_push(Vector *vector, void *value)
{
  vector_push_n(vector, value, 1);
}

void vector_push_n(Vector *vector, void *value, unsigned long count)
{
  if (vector_count(vector) + count > vector_capacity(vector)) {
    unsigned long capacity = vector_capacity(vector) ? vector_capacity(vector) : 1;
    while (vector_count(vector) + count > capacity) {
      capacity *= 2;
    }
    vector_reserve(vector, capacity);
  }
  memcpy(vector_at(vector, vector_count(vector)), value, vector->_size * count);
  vector->_count += count;
}

void vector_pop(Vector *vector)
{
  vector_pop_n(vector, 1);
}

void vector_pop_n(Vector *vector, unsigned long count)
{
  if (vector->_count > count) {
    vector->_count -= count;
  } else {
    vector->_count = 0;
  }
}

void vector_clear(Vector *vector)
{
  vector->_count = 0;
}

void *vector_steal(Vector *vector)
{
  void *result      = vector->_data;
  vector->_data     = NULL;
  vector->_capacity = 0;
  vector->_count    = 0;
  return result;
}
