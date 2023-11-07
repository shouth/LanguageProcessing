#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vector Vector;

struct Vector {
  void         *_data;
  unsigned long _size;
  unsigned long _count;
  unsigned long _capacity;
};

void vector_init(Vector *vector, unsigned long size);
void vector_init_with_capacity(Vector *vector, unsigned long size, unsigned long capacity);
void vector_deinit(Vector *vector);

unsigned long vector_count(const Vector *vector);
unsigned long vector_capacity(const Vector *vector);
void         *vector_data(const Vector *vector);
void         *vector_at(const Vector *vector, unsigned long index);
void         *vector_back(Vector *vector);
void          vector_reserve(Vector *vector, unsigned long capacity);
void          vector_fit(Vector *vector);
void          vector_push(Vector *vector, void *value);
void          vector_pop(Vector *vector);
void          vector_clear(Vector *vector);
void         *vector_steal(Vector *vector);

#endif
