#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

void *xmalloc(long size);

const char *str_type(const type_t *type);

#define FNV1A_INIT 0x811C9DC5

unsigned long fnv1a(unsigned long hash, const void *ptr, long len);

int bit_popcount(unsigned long n);
int bit_right_most(unsigned long n);
int bit_left_most(unsigned long n);

#define unreachable()                                                                          \
  do {                                                                                         \
    fprintf(stderr, "internal error: entered unreachable code [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                        \
  } while (0)

typedef int                      hash_map_comp_t(const void *, const void *);
typedef unsigned long            hash_map_hasher_t(const void *);
typedef void                     hash_map_deleter_t(void *);
typedef struct hash_map__entry_s hash_map_entry_t;

struct hash_map__entry_s {
  unsigned long hop;
  void         *key;
  void         *value;
};

typedef struct {
  long               size;
  long               capacity;
  int                load_factor;
  long               bucket_cnt;
  hash_map_entry_t  *buckets;
  hash_map_comp_t   *comparator;
  hash_map_hasher_t *hasher;
} hash_map_t;

hash_map_t             *hash_map_new(hash_map_comp_t *comparator, hash_map_hasher_t *hasher);
void                    hash_map_delete(hash_map_t *table);
const hash_map_entry_t *hash_map_find(hash_map_t *table, const void *key);
void                    hash_map_update(hash_map_t *table, void *key, void *value);
int                     hash_map_remove(hash_map_t *table, const void *key);

#define SGR__FLAG ((unsigned long) 1 << 24)

#define SGR_RESET            (SGR__FLAG | 0)
#define SGR_BOLD             (SGR__FLAG | 1)
#define SGR_FAINT            (SGR__FLAG | 2)
#define SGR_ITALIC           (SGR__FLAG | 3)
#define SGR_UNDERLINE        (SGR__FLAG | 4)
#define SGR_NORMAL_INTENSITY (SGR__FLAG | 22)
#define SGR_NOT_ITALIC       (SGR__FLAG | 23)
#define SGR_NOT_UNDERLINED   (SGR__FLAG | 24)

#define SGR_FG_BLACK   (SGR__FLAG | 30)
#define SGR_FG_RED     (SGR__FLAG | 31)
#define SGR_FG_GREEN   (SGR__FLAG | 32)
#define SGR_FG_YELLOW  (SGR__FLAG | 33)
#define SGR_FG_BLUE    (SGR__FLAG | 34)
#define SGR_FG_MAGENTA (SGR__FLAG | 35)
#define SGR_FG_CYAN    (SGR__FLAG | 36)
#define SGR_FG_WHITE   (SGR__FLAG | 37)
#define SGR_BG_BLACK   (SGR__FLAG | 40)
#define SGR_BG_RED     (SGR__FLAG | 41)
#define SGR_BG_GREEN   (SGR__FLAG | 42)
#define SGR_BG_YELLOW  (SGR__FLAG | 43)
#define SGR_BG_BLUE    (SGR__FLAG | 44)
#define SGR_BG_MAGENTA (SGR__FLAG | 45)
#define SGR_BG_CYAN    (SGR__FLAG | 46)
#define SGR_BG_WHITE   (SGR__FLAG | 47)

#define SGR_FG_BRIGHT_BLACK   (SGR__FLAG | 90)
#define SGR_FG_BRIGHT_RED     (SGR__FLAG | 91)
#define SGR_FG_BRIGHT_GREEN   (SGR__FLAG | 92)
#define SGR_FG_BRIGHT_YELLOW  (SGR__FLAG | 93)
#define SGR_FG_BRIGHT_BLUE    (SGR__FLAG | 94)
#define SGR_FG_BRIGHT_MAGENTA (SGR__FLAG | 95)
#define SGR_FG_BRIGHT_CYAN    (SGR__FLAG | 96)
#define SGR_FG_BRIGHT_WHITE   (SGR__FLAG | 97)
#define SGR_BG_BRIGHT_BLACK   (SGR__FLAG | 100)
#define SGR_BG_BRIGHT_RED     (SGR__FLAG | 101)
#define SGR_BG_BRIGHT_GREEN   (SGR__FLAG | 102)
#define SGR_BG_BRIGHT_YELLOW  (SGR__FLAG | 103)
#define SGR_BG_BRIGHT_BLUE    (SGR__FLAG | 104)
#define SGR_BG_BRIGHT_MAGENTA (SGR__FLAG | 105)
#define SGR_BG_BRIGHT_CYAN    (SGR__FLAG | 106)
#define SGR_BG_BRIGHT_WHITE   (SGR__FLAG | 107)

void term_ansi_stdout(int flag);
void term_ansi_stderr(int flag);
void term_set(unsigned long code);

#endif
