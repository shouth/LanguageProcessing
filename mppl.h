#ifndef MPPL_H
#define MPPL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* utility */

static uint64_t fnv1(const void *ptr, size_t len)
{
    uint64_t ret = 0xcbf29ce484222325;
    uint64_t prime = 0x00000100000001b3;
    const char *p = ptr;
    size_t i;

    for (i = 0; i < len; i++) {
        ret *= prime;
        ret ^= p[i];
    }
    return ret;
}

static uint64_t fnv1_int(uint64_t value)
{ return fnv1(&value, sizeof(value)); }

static uint64_t fnv1_ptr(const void *ptr)
{ return fnv1(&ptr, sizeof(ptr)); }

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

/* utility.c */

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
{ return lhs == rhs; }

static uint64_t hash_table_default_hasher(const void *ptr)
{ return fnv1_ptr(ptr); }

typedef enum {
    SGR_RESET             = 0,
    SGR_BOLD              = 1,
    SGR_FAINT             = 2,
    SGR_ITALIC            = 3,
    SGR_UNDERLINE         = 4,
    SGR_NORMAL_INTENSITY  = 22,
    SGR_NOT_ITALIC        = 23,
    SGR_NOT_UNDERLINED    = 24,

    SGR_FG_BLACK          = 30,
    SGR_FG_RED            = 31,
    SGR_FG_GREEN          = 32,
    SGR_FG_YELLOW         = 33,
    SGR_FG_BLUE           = 34,
    SGR_FG_MAGENTA        = 35,
    SGR_FG_CYAN           = 36,
    SGR_FG_WHITE          = 37,
    SGR_BG_BLACK          = 40,
    SGR_BG_RED            = 41,
    SGR_BG_GREEN          = 42,
    SGR_BG_YELLOW         = 43,
    SGR_BG_BLUE           = 44,
    SGR_BG_MAGENTA        = 45,
    SGR_BG_CYAN           = 46,
    SGR_BG_WHITE          = 47,

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
void console_reset();
void console_24bit(color_t color);

/* source.c */

typedef struct {
    char *input_filename;
    char *output_filename;
    char *src_ptr;
    size_t src_size;
    size_t *lines_ptr;
    size_t lines_size;
} source_t;

source_t *new_source(const char *filename, const char *output);
void delete_source(source_t *src);

typedef struct {
    size_t line;
    size_t col;
} location_t;

location_t location_from(size_t line, size_t col);
location_t source_location(const source_t *src, size_t index);

typedef struct {
    size_t pos;
    size_t len;
} region_t;

region_t region_from(size_t pos, size_t len);
region_t region_unite(region_t a, region_t b);
int region_compare(region_t a, region_t b);

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

typedef struct {
    const char *ptr;
    size_t len;
} symbol_t;

typedef struct {
    hash_table_t *table;
} symbol_storage_t;

int symbol_compare(const void *lhs, const void *rhs);
symbol_storage_t *new_symbol_storage();
void delete_symbol_storage(symbol_storage_t *storage);
const symbol_t *symbol_intern(symbol_storage_t *storage, const char *ptr, size_t len);

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
    region_t region;
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

/* crossref.c */

void print_crossref(const ir_t *ir);

/* codegen.c */

void codegen_casl2(const ir_t *ir);

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
    region_t region;
    msg_inline_entry_t *next;
};

typedef struct {
    const source_t *src;
    char msg[256];
    region_t region;
    msg_level_t level;
    msg_inline_entry_t *inline_entries;
    msg_entry_t *entries;
} msg_t;

msg_t *new_msg(const source_t *src, region_t region, msg_level_t level, const char *fmt, ...);
void delete_msg(msg_t *msg);
void msg_add_entry(msg_t *msg, msg_level_t level, const char *fmt, ...);
void msg_add_inline_entry(msg_t *msg, region_t region, const char *fmt, ...);
void msg_emit(msg_t *msg);

#endif
