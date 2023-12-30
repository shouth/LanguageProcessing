#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "lexer.h"
#include "map.h"
#include "source.h"
#include "syntax_kind.h"
#include "tasks.h"
#include "token_tree.h"
#include "utility.h"

typedef struct TokenCountEntry TokenCountEntry;
typedef struct TokenCount      TokenCount;

struct TokenCountEntry {
  TokenInfo     token;
  unsigned long count;
};

struct TokenCount {
  Array *token_counts;
  Array *identifer_counts;
};

static void increment_token(Map *counts, TokenInfo *info)
{
  TokenCountEntry *counter;
  MapIndex         entry;

  counter        = xmalloc(sizeof(TokenCountEntry));
  counter->token = *info;
  counter->count = 0;

  if (map_find(counts, &counter->token, &entry)) {
    token_info_deinit(&counter->token);
    free(counter);
    counter = map_value(counts, &entry);
  } else {
    map_update(counts, &entry, &counter->token, counter);
  }

  ++counter->count;
}

static unsigned long token_info_hash(const void *key)
{
  const TokenInfo *info = key;
  unsigned long    hash = FNV1A_INIT;
  hash                  = fnv1a(hash, &info->kind, sizeof(SyntaxKind));
  hash                  = fnv1a(hash, info->text, info->text_length);
  return hash;
}

static int token_info_compare(const void *left, const void *right)
{
  const TokenInfo *l = left;
  const TokenInfo *r = right;

  if (l->kind < r->kind) {
    return -1;
  } else if (l->kind > r->kind) {
    return 1;
  } else {
    return strcmp(l->text, r->text);
  }
}

static int token_info_equal(const void *left, const void *right)
{
  return !token_info_compare(left, right);
}

static Array *list_token(Map *counts)
{
  Array   *list = array_new_with_capacity(sizeof(TokenCountEntry), map_count(counts));
  MapIndex entry;
  map_index(counts, &entry);
  while (map_next(counts, &entry)) {
    array_push(list, map_value(counts, &entry));
    free(map_value(counts, &entry));
  }
  map_free(counts);
  qsort(array_data(list), array_count(list), sizeof(TokenCountEntry), &token_info_compare);
  return list;
}

static TokenStatus token_count_init(TokenCount *count, const Source *source)
{
  TokenInfo   token;
  TokenStatus status;
  Map        *token_counts;
  Map        *identifier_counts;

  unsigned long offset = 0;

  token_counts      = map_new(&token_info_hash, &token_info_equal);
  identifier_counts = map_new(&token_info_hash, &token_info_equal);
  while (1) {
    status = mppl_lex(source, offset, &token);
    if (status != TOKEN_OK) {
      break;
    }

    offset += token.text_length;
    if (syntax_kind_is_trivia(token.kind)) {
      token_info_deinit(&token);
      continue;
    }

    switch (token.kind) {
    case SYNTAX_KIND_IDENTIFIER_TOKEN:
      increment_token(identifier_counts, &token);
      token_info_init(&token, SYNTAX_KIND_IDENTIFIER_TOKEN, "NAME", 4);
      break;
    case SYNTAX_KIND_INTEGER_LITERAL:
      token_info_deinit(&token);
      token_info_init(&token, SYNTAX_KIND_INTEGER_LITERAL, "NUMBER", 6);
      break;
    case SYNTAX_KIND_STRING_LITERAL:
      token_info_deinit(&token);
      token_info_init(&token, SYNTAX_KIND_STRING_LITERAL, "STRING", 6);
      break;
    default:
      /* do nothing */
      break;
    }
    increment_token(token_counts, &token);
  }
  token_info_deinit(&token);
  count->token_counts     = list_token(token_counts);
  count->identifer_counts = list_token(identifier_counts);
  return status;
}

static void token_count_deinit(TokenCount *count)
{
  unsigned long i;

  for (i = 0; i < array_count(count->token_counts); ++i) {
    token_info_deinit(array_at(count->token_counts, i));
  }
  array_free(count->token_counts);

  for (i = 0; i < array_count(count->identifer_counts); ++i) {
    token_info_deinit(array_at(count->identifer_counts, i));
  }
  array_free(count->identifer_counts);
}

unsigned long get_token_display_width(TokenCountEntry *entry)
{
  return entry->token.text_length;
}

unsigned long get_max_token_display_width(TokenCountEntry *entries, unsigned long length)
{
  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < length; ++i) {
    unsigned long width = get_token_display_width(entries + i);
    if (result < width) {
      result = width;
    }
  }
  return result;
}

unsigned long get_count_display_width(TokenCountEntry *entry)
{
  unsigned long result = 1;
  unsigned long count  = entry->count;
  while (count > 9) {
    ++result;
    count /= 10;
  }
  return result;
}

unsigned long get_max_count_display_width(TokenCountEntry *entries, unsigned long length)
{
  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < length; ++i) {
    unsigned long width = get_count_display_width(entries + i);
    if (result < width) {
      result = width;
    }
  }
  return result;
}

static void token_count_print(TokenCount *count)
{
  unsigned long i, j;
  const char   *identifier_prefix       = "    \"Identifier\" ";
  unsigned long identifier_prefix_width = strlen(identifier_prefix);

  unsigned long max_token_display_width;
  unsigned long max_count_display_width;

  {
    unsigned long max_token_width
      = get_max_token_display_width(array_data(count->token_counts), array_count(count->token_counts));
    unsigned long max_identifier_width
      = get_max_token_display_width(array_data(count->identifer_counts), array_count(count->identifer_counts))
      + identifier_prefix_width;

    max_token_display_width
      = max_token_width > max_identifier_width ? max_token_width : max_identifier_width;
  }

  {
    unsigned long max_token_count_width
      = get_max_count_display_width(array_data(count->token_counts), array_count(count->token_counts));
    unsigned long max_identifier_count_width
      = get_max_count_display_width(array_data(count->identifer_counts), array_count(count->identifer_counts));

    max_count_display_width
      = max_token_count_width > max_identifier_count_width ? max_token_count_width : max_identifier_count_width;
  }

  for (i = 0; i < array_count(count->token_counts); ++i) {
    TokenCountEntry *token_entry       = array_at(count->token_counts, i);
    unsigned long    token_space_width = (max_token_display_width - get_token_display_width(token_entry))
      + (max_count_display_width - get_count_display_width(token_entry)) + 2;
    printf("\"%s\"%*c%lu\n", token_entry->token.text, (int) token_space_width, ' ', token_entry->count);

    if (token_entry->token.kind == SYNTAX_KIND_IDENTIFIER_TOKEN) {
      for (j = 0; j < array_count(count->identifer_counts); ++j) {
        TokenCountEntry *identifier_entry = array_at(count->identifer_counts, j);
        unsigned long    identifier_space_width
          = (max_token_display_width - identifier_prefix_width - get_token_display_width(identifier_entry))
          + (max_count_display_width - get_count_display_width(identifier_entry)) + 2;
        printf("%s\"%s\"%*c%lu\n",
          identifier_prefix, identifier_entry->token.text, (int) identifier_space_width, ' ', identifier_entry->count);
      }
    }
  }
}

void task1(int argc, const char **argv)
{
  Source     *source;
  TokenCount counter;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  source = source_new(argv[1], strlen(argv[1]));
  switch (token_count_init(&counter, source)) {
  case TOKEN_EOF:
    token_count_print(&counter);
    break;
  case TOKEN_ERROR_STRAY_CHAR:
    fprintf(stderr, "Error: Stray character in program\n");
    break;
  case TOKEN_ERROR_NONGRAPHIC_CHAR:
    fprintf(stderr, "Error: Non-graphic character in string\n");
    break;
  case TOKEN_ERROR_UNTERMINATED_STRING:
    fprintf(stderr, "Error: String is unterminated\n");
    break;
  case TOKEN_ERROR_UNTERMINATED_COMMENT:
    fprintf(stderr, "Error: Comment is unterminated\n");
    break;
  default:
    unreachable();
  }
  token_count_deinit(&counter);
  source_free(source);
}
