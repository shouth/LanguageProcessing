/* SPDX-License-Identifier: Apache-2.0 */

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* Memory */

void *xmalloc(unsigned long size);

unsigned long popcount(const void *data, unsigned long size);

/* BitSet */

#define BitSet(bits)                                      \
  struct {                                                \
    unsigned char data[(bits + CHAR_BIT - 1) / CHAR_BIT]; \
  }

#define bitset_zero() \
  { 0 }

#define bitset_set(self, index) \
  ((self)->data[(index) / CHAR_BIT] |= 1ul << ((index) % CHAR_BIT))

#define bitset_reset(self, index) \
  ((self)->data[(index) / CHAR_BIT] &= ~(1ul << ((index) % CHAR_BIT)))

#define bitset_get(self, index) \
  ((self)->data[(index) / CHAR_BIT] & (1ul << ((index) % CHAR_BIT)))

#define bitset_clear(self) \
  memset((self)->data, 0, sizeof((self)->data))

#define bitset_count(self) \
  popcount((self)->data, sizeof((self)->data))

#define bitset_insert(self, other)                   \
  do {                                               \
    unsigned long        i;                          \
    unsigned char       *self_data  = (self)->data;  \
    const unsigned char *other_data = (other)->data; \
    for (i = 0; i < count_of((self)->data); ++i) {   \
      self_data[i] |= other_data[i];                 \
    }                                                \
  } while (0)

#define bitset_erase(self, other)                    \
  do {                                               \
    unsigned long        i;                          \
    unsigned char       *self_data  = (self)->data;  \
    const unsigned char *other_data = (other)->data; \
    for (i = 0; i < count_of((self)->data); ++i) {   \
      self_data[i] &= ~other_data[i];                \
    }                                                \
  } while (0)

/* List */

typedef struct ListNode ListNode;
typedef int ListNodeCompare(ListNode const *a, ListNode const *b);

struct ListNode {
  struct ListNode *next;
  struct ListNode *prev;
};

void list_init(ListNode *node);
void list_push_back(ListNode *head, ListNode *node);
void list_erase(ListNode *node);
void list_sort(ListNode *head, ListNodeCompare *compare);

/* Fenwick */

void fenwick_construct(size_t *fenwick, size_t count);
void fenwick_add(size_t *fenwick, size_t count, size_t index, size_t value);
size_t fenwick_query(size_t const *fenwick, size_t count, size_t index);
size_t fenwick_upper_bound(size_t const *fenwick, size_t count, size_t value);

/* Character */

int is_alphabet(int c);
int is_number(int c);
int is_space(int c);
int is_graphic(int c);

long utf8_len(const char *str, unsigned long len);

/* Color */

#define MONOKAI_RED    0xFF6188
#define MONOKAI_GREEN  0xA9DC76
#define MONOKAI_YELLOW 0xFFD866
#define MONOKAI_BLUE   0x78DCE8
#define MONOKAI_PURPLE 0xAB9DF2

/* Misc */

char *load_file(char const *filename, size_t *out_length);

#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))

#define count_of(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t) (!(sizeof(x) % sizeof(0 [x])))))

#if defined(__GNUC__) || defined(__clang__)

#define format(archetype, string_index, first_to_check) \
  __attribute__((format(archetype, string_index, first_to_check)))

#else

#define format(archetype, string_index, first_to_check)

#endif

#define unreachable()                                                                            \
  do {                                                                                           \
    fprintf(stderr, "Internal Error: Entered unreachable region [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                          \
  } while (0)

#endif /* UTIL_H */
