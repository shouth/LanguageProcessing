#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

void vector_init(Vector *vector)
{
  vector_init_with_capacity(vector, 1 << 4);
}

void vector_init_with_capacity(Vector *vector, unsigned long capacity)
{
  vector->_size = 0;
  vector->_data = NULL;
  vector_reserve(vector, capacity);
}

void vector_deinit(Vector *vector)
{
  free(vector->_data);
}

unsigned long vector_size(const Vector *vector)
{
  return vector->_size;
}

unsigned long vector_capacity(const Vector *vector)
{
  return vector->_capacity;
}

void **vector_data(const Vector *vector)
{
  return vector->_data;
}

void *vector_back(Vector *vector)
{
  return vector->_data[vector->_size - 1];
}

void vector_reserve(Vector *vector, unsigned long capacity)
{
  void **new = realloc(vector->_data, sizeof(void *) * capacity);
  if (!new) {
    fprintf(stderr, "Internal Error: Failed to allocate memory\n");
    exit(EXIT_FAILURE);
  }
  vector->_data     = new;
  vector->_capacity = capacity;
}

void vector_push_back(Vector *vector, void *value)
{
  if (vector->_size == vector->_capacity) {
    vector_reserve(vector, vector->_capacity * 2);
  }
  vector->_data[vector->_size] = value;
  ++vector->_size;
}

void vector_pop_back(Vector *vector)
{
  --vector->_size;
}

void vector_clear(Vector *vector)
{
  vector->_size = 0;
}
