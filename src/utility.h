#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(long size);

uint64_t fnv1(const void *ptr, long len);

int popcount(uint64_t n);
int trailing0(uint64_t n);
int leading0(uint64_t n);

#if defined(__GNUC__) || defined(__clang__)
#define intrinsic_unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
#define intrinsic_unreachable() __assume(0)
#else
#define intrinsic_unreachable()
#endif

#define unreachable()                                                                          \
  do {                                                                                         \
    fprintf(stderr, "internal error: entered unreachable code [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                        \
    intrinsic_unreachable();                                                                   \
  } while (0)

#undef intrinsic_unreachable

typedef uintptr_t            hash_hop_t;
typedef int                  hash_comp_t(const void *, const void *);
typedef uint64_t             hash_hasher_t(const void *);
typedef void                 hash_deleter_t(void *);
typedef struct hash__entry_s hash_entry_t;

struct hash__entry_s {
  hash_hop_t hop;
  void      *key;
  void      *value;
};

typedef struct {
  long           size;
  long           capacity;
  uint8_t        load_factor;
  long           bucket_cnt;
  hash_entry_t  *buckets;
  hash_entry_t   removed;
  hash_comp_t   *comparator;
  hash_hasher_t *hasher;
} hash_t;

hash_t             *hash_new(hash_comp_t *comparator, hash_hasher_t *hasher);
void                hash_delete(hash_t *table, hash_deleter_t *key_deleter, hash_deleter_t *value_deleter);
const hash_entry_t *hash_find(hash_t *table, const void *key);
void                hash_insert_unsafe(hash_t *table, void *key, void *value);
hash_entry_t       *hash_insert(hash_t *table, void *key, void *value);
hash_entry_t       *hash_remove(hash_t *table, const void *key);
int                 hash_default_comp(const void *lhs, const void *rhs);
uint64_t            hash_default_hasher(const void *ptr);

typedef enum {
  SGR_RESET            = 0,
  SGR_BOLD             = 1,
  SGR_FAINT            = 2,
  SGR_ITALIC           = 3,
  SGR_UNDERLINE        = 4,
  SGR_NORMAL_INTENSITY = 22,
  SGR_NOT_ITALIC       = 23,
  SGR_NOT_UNDERLINED   = 24,

  SGR_FG_BLACK   = 30,
  SGR_FG_RED     = 31,
  SGR_FG_GREEN   = 32,
  SGR_FG_YELLOW  = 33,
  SGR_FG_BLUE    = 34,
  SGR_FG_MAGENTA = 35,
  SGR_FG_CYAN    = 36,
  SGR_FG_WHITE   = 37,
  SGR_BG_BLACK   = 40,
  SGR_BG_RED     = 41,
  SGR_BG_GREEN   = 42,
  SGR_BG_YELLOW  = 43,
  SGR_BG_BLUE    = 44,
  SGR_BG_MAGENTA = 45,
  SGR_BG_CYAN    = 46,
  SGR_BG_WHITE   = 47,

  SGR_FG_BRIGHT_BLACK   = 90,
  SGR_FG_BRIGHT_RED     = 91,
  SGR_FG_BRIGHT_GREEN   = 92,
  SGR_FG_BRIGHT_YELLOW  = 93,
  SGR_FG_BRIGHT_BLUE    = 94,
  SGR_FG_BRIGHT_MAGENTA = 95,
  SGR_FG_BRIGHT_CYAN    = 96,
  SGR_FG_BRIGHT_WHITE   = 97,
  SGR_BG_BRIGHT_BLACK   = 100,
  SGR_BG_BRIGHT_RED     = 101,
  SGR_BG_BRIGHT_GREEN   = 102,
  SGR_BG_BRIGHT_YELLOW  = 103,
  SGR_BG_BRIGHT_BLUE    = 104,
  SGR_BG_BRIGHT_MAGENTA = 105,
  SGR_BG_BRIGHT_CYAN    = 106,
  SGR_BG_BRIGHT_WHITE   = 107
} sgr_t;

typedef uint64_t color_t;

void console_ansi(int flag);
void console_set(sgr_t code);
void console_reset(void);
void console_24bit(color_t color);

#endif
