#ifndef MPPL_H
#define MPPL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* utility */

static uint64_t fnv1(const uint8_t *ptr, size_t len)
{
    uint64_t ret   = 0xcbf29ce484222325;
    uint64_t prime = 0x00000100000001b3;
    size_t i;

    for (i = 0; i < len; i++) {
        ret *= prime;
        ret ^= ptr[i];
    }
    return ret;
}

static uint64_t fnv1_int(uint64_t value)
{
    return fnv1((uint8_t *) &value, sizeof(value));
}

static uint64_t fnv1_ptr(const void *ptr)
{
    return fnv1((uint8_t *) &ptr, sizeof(ptr));
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

static uint8_t lsb(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
    /* If compiler is GCC or Clang, use builtin functions. */
    return __builtin_ctzll(n);

#else
    /* Otherwise, use bitwise operatons. */
    return popcount((n & (~n + 1)) - 1);
#endif
}

static uint8_t msb(uint64_t n)
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

/* hash.c */

typedef uintptr_t hash_table_hop_t;
typedef int hash_table_comparator_t(const void *, const void *);
typedef uint64_t hash_table_hasher_t(const void *);
typedef void hash_table_deleter_t(void *);

typedef struct impl_hash_table_entry hash_table_entry_t;
struct impl_hash_table_entry {
    hash_table_hop_t hop;
    void *key;
    void *value;
};

typedef struct {
    size_t size;
    size_t capacity;
    uint8_t load_factor;
    size_t bucket_cnt;
    hash_table_entry_t *buckets;
    hash_table_entry_t removed;
    hash_table_comparator_t *comparator;
    hash_table_hasher_t *hasher;
} hash_table_t;

hash_table_t *new_hash_table(hash_table_comparator_t *comparator, hash_table_hasher_t *hasher);
void delete_hash_table(hash_table_t *table, hash_table_deleter_t *key_deleter, hash_table_deleter_t *value_deleter);
const hash_table_entry_t *hash_table_find(hash_table_t *table, const void *key);
void hash_table_insert_unchecked(hash_table_t *table, void *key, void *value);
hash_table_entry_t *hash_table_insert(hash_table_t *table, void *key, void *value);
hash_table_entry_t *hash_table_remove(hash_table_t *table, const void *key);

static int hash_table_default_comparator(const void *lhs, const void *rhs)
{
    return lhs == rhs;
}

static uint64_t hash_table_default_hasher(const void *ptr)
{
    return fnv1_ptr(ptr);
}

/* source.c */

typedef struct {
    char *filename;
    char *src_ptr;
    size_t src_size;
    size_t *lines_ptr;
    size_t lines_size;
} source_t;

typedef struct {
    size_t line;
    size_t col;
    const source_t *src;
} location_t;

source_t *new_source(const char *filename);
void delete_source(source_t *src);
void source_location(const source_t *src, size_t index, location_t *loc);

/* symbol.c */

typedef uintptr_t symbol_t;

typedef struct {
    const char *ptr;
    size_t len;
} symbol_instance_t;

typedef struct {
    hash_table_t *table;
} symbol_storage_t;

symbol_storage_t *new_symbol_storage();
void delete_symbol_storage(symbol_storage_t *storage);
symbol_t symbol_intern(symbol_storage_t *storage, const char *ptr, size_t len);
const symbol_instance_t *symbol_get_instance(symbol_t symbol);

/* cursol.c */

typedef struct {
    size_t init_len;
    const char *ptr;
    size_t len;
    const source_t *src;
} cursol_t;

void cursol_init(cursol_t *cur, const source_t *src, const char *ptr, size_t len);
int cursol_nth(const cursol_t *cur, size_t index);
int cursol_first(const cursol_t *cur);
int cursol_second(const cursol_t *cur);
int cursol_eof(const cursol_t *cur);
void cursol_next(cursol_t *cur);
size_t cursol_position(const cursol_t *cur);

/* lexer.c */

typedef enum {
    TOKEN_NAME,
    TOKEN_PROGRAM,
    TOKEN_VAR,
    TOKEN_ARRAY,
    TOKEN_OF,
    TOKEN_BEGIN,
    TOKEN_END,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_PROCEDURE,
    TOKEN_RETURN,
    TOKEN_CALL,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_NOT,
    TOKEN_OR,
    TOKEN_DIV,
    TOKEN_AND,
    TOKEN_CHAR,
    TOKEN_INTEGER,
    TOKEN_BOOLEAN,
    TOKEN_READLN,
    TOKEN_WRITELN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_EQUAL,
    TOKEN_NOTEQ,
    TOKEN_LE,
    TOKEN_LEEQ,
    TOKEN_GR,
    TOKEN_GREQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LSQPAREN,
    TOKEN_RSQPAREN,
    TOKEN_ASSIGN,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMI,
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_BREAK,
    TOKEN_WHITESPACE,
    TOKEN_BRACES_COMMENT,
    TOKEN_CSTYLE_COMMENT,
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_ERROR
} token_kind_t;

typedef union {
    struct {
        unsigned long value;
    } number;
    struct {
        const char *ptr;
        size_t len;
        size_t str_len;
    } string;
} token_data_t;

typedef struct {
    const char *ptr;
    size_t len;
    size_t pos;
    token_kind_t type;
    token_data_t data;
} token_t;

void lex_token(cursol_t *cursol, token_t *ret);
const char *token_to_str(token_kind_t type);

/* ast.c */

#include "ast.h"

/* parser.c */

ast_t *parse_source(const source_t *src);

/* pretty_print.c */

void pretty_print(const ast_t *ast);

/* ir.c */

#include "ir.h"

/* analyzer.c */

ir_t *analyze_ast(ast_t *ast);

/* msg.c */

typedef enum {
    MSG_HELP,
    MSG_NOTE,
    MSG_WARN,
    MSG_ERROR,
    MSG_FATAL
} msg_level_t;

typedef struct msg_entry msg_entry_t;
struct msg_entry {
    char msg[256];
    msg_level_t level;
    msg_entry_t *next;
};

typedef struct msg_inline_entry msg_inline_entry_t;
struct msg_inline_entry {
    char msg[256];
    size_t pos;
    size_t len;
    msg_inline_entry_t *next;
};

typedef struct {
    const source_t *src;
    char msg[256];
    size_t pos;
    size_t len;
    msg_level_t level;
    msg_inline_entry_t *inline_entries;
    msg_entry_t *entries;
} msg_t;

msg_t *new_msg(const source_t *src, size_t pos, size_t len, msg_level_t level, const char *fmt, ...);
void delete_msg(msg_t *msg);
void msg_add_entry(msg_t *msg, msg_level_t level, const char *fmt, ...);
void msg_add_inline_entry(msg_t *msg, size_t pos, size_t len, const char *fmt, ...);
void msg_emit(msg_t *msg);

#endif
