#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vector Vector;

struct Vector {
  void        **_data;
  unsigned long _size;
  unsigned long _capacity;
};

void vector_init(Vector *vector);
void vector_init_with_capacity(Vector *vector, unsigned long capacity);
void vector_deinit(Vector *vector);

unsigned long vector_size(const Vector *vector);
unsigned long vector_capacity(const Vector *vector);
void        **vector_data(const Vector *vector);
void         *vector_back(Vector *vector);
void          vector_reserve(Vector *vector, unsigned long capacity);
void          vector_push_back(Vector *vector, void *value);
void          vector_pop_back(Vector *vector);
void          vector_clear(Vector *vector);

#endif
