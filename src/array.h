#ifndef VECTOR_H
#define VECTOR_H

typedef struct Array Array;

struct Array {
  void         *_data;
  unsigned long _size;
  unsigned long _count;
  unsigned long _capacity;
};

void array_init(Array *array, unsigned long size);
void array_init_with_capacity(Array *array, unsigned long size, unsigned long capacity);
void array_deinit(Array *array);

unsigned long array_count(const Array *array);
unsigned long array_capacity(const Array *array);
void         *array_data(const Array *array);
void         *array_at(const Array *array, unsigned long index);
void         *array_front(const Array *array);
void         *array_back(const Array *array);
void          array_reserve(Array *array, unsigned long capacity);
void          array_fit(Array *array);
void          array_push(Array *array, void *value);
void          array_push_count(Array *array, void *value, unsigned long count);
void          array_pop(Array *array);
void          array_pop_count(Array *array, unsigned long count);
void          array_clear(Array *array);
void         *array_steal(Array *array);

#endif
