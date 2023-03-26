#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t fnv1(const char *ptr, size_t len)
{
  uint64_t ret   = 0xcbf29ce484222325;
  uint64_t prime = 0x00000100000001b3;
  size_t   i;

  for (i = 0; i < len; i++) {
    ret *= prime;
    ret ^= ptr[i];
  }
  return ret;
}

static uint64_t fnv1_int(uint64_t value)
{
  char buf[sizeof(value)];
  memcpy(buf, &value, sizeof(value));
  return fnv1(buf, sizeof(value));
}

static uint64_t fnv1_ptr(const void *ptr)
{
  char buf[sizeof(ptr)];
  memcpy(buf, &ptr, sizeof(ptr));
  return fnv1(buf, sizeof(ptr));
}

static uint8_t popcount(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
  /* If compiler is GCC or Clang, use builtin functions. */
  return __builtin_popcountll(n);

#else
  /* Otherwise, use bitwise operatons. */
  n -= (n >> 1) & 0x5555555555555555;
  n = (n & 0x3333333333333333) + ((n >> 2) & 0x3333333333333333);
  n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0f;
  n += n >> 8;
  n += n >> 16;
  n += n >> 32;
  return n & 0x000000000000007f;
#endif
}

static uint8_t trailing0(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
  /* If compiler is GCC or Clang, use builtin functions. */
  return __builtin_ctzll(n);

#else
  /* Otherwise, use bitwise operatons. */
  return popcount((n & (~n + 1)) - 1);
#endif
}

static uint8_t leading0(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
  /* If compiler is GCC or Clang, use builtin functions. */
  return 63 - __builtin_clzll(n);

#else
  /* Otherwise, use bitwise operatons. */
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return popcount(n) - 1;
#endif
}

static void *xmalloc(size_t size)
{
  void *ret = malloc(size);
  if (ret == NULL) {
    fprintf(stderr, "memory allocation failed!");
    exit(1);
  }
  return ret;
}

#define new(type) ((type *) xmalloc(sizeof(type)))
#define new_arr(type, size) ((type *) xmalloc(sizeof(type) * (size)))

#define unreachable() \
  (fprintf(stderr, "internal error: entered unreachable code [%s:%d]\n", __FILE__, __LINE__), exit(1))

typedef uintptr_t hash_table_hop_t;
typedef int       hash_table_comparator_t(const void *, const void *);
typedef uint64_t  hash_table_hasher_t(const void *);
typedef void      hash_table_deleter_t(void *);

typedef struct impl_hash_table_entry hash_table_entry_t;
struct impl_hash_table_entry {
  hash_table_hop_t hop;
  void            *key;
  void            *value;
};

typedef struct {
  size_t                   size;
  size_t                   capacity;
  uint8_t                  load_factor;
  size_t                   bucket_cnt;
  hash_table_entry_t      *buckets;
  hash_table_entry_t       removed;
  hash_table_comparator_t *comparator;
  hash_table_hasher_t     *hasher;
} hash_table_t;

hash_table_t             *new_hash_table(hash_table_comparator_t *comparator, hash_table_hasher_t *hasher);
void                      delete_hash_table(hash_table_t *table, hash_table_deleter_t *key_deleter, hash_table_deleter_t *value_deleter);
const hash_table_entry_t *hash_table_find(hash_table_t *table, const void *key);
void                      hash_table_insert_unchecked(hash_table_t *table, void *key, void *value);
hash_table_entry_t       *hash_table_insert(hash_table_t *table, void *key, void *value);
hash_table_entry_t       *hash_table_remove(hash_table_t *table, const void *key);

static int hash_table_default_comparator(const void *lhs, const void *rhs)
{
  return lhs == rhs;
}

static uint64_t hash_table_default_hasher(const void *ptr)
{
  return fnv1_ptr(ptr);
}

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
