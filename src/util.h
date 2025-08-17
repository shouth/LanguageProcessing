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

/* BitSet */

void bitset_init(unsigned long *bitset, size_t size);
void bitset_set(unsigned long *bitset, size_t index, int value);
int bitset_get(unsigned long const *bitset, size_t index);
size_t bitset_count(unsigned long const *bitset, size_t size);

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

void fenwick_build(size_t *fenwick, size_t count);
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
