#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "map.h"
#include "report.h"
#include "source.h"
#include "syntax_kind.h"
#include "tasks.h"
#include "token.h"
#include "utility.h"
#include "vector.h"

typedef struct TokenCountEntry TokenCountEntry;
typedef struct TokenCount      TokenCount;

struct TokenCountEntry {
  TokenInfo     token;
  unsigned long count;
};

struct TokenCount {
  Vector token_counts;
  Vector identifer_counts;
};

static void increment_token(Map *counts, TokenInfo *info)
{
  TokenCountEntry *counter;
  MapEntry         entry;

  counter        = xmalloc(sizeof(TokenCountEntry));
  counter->token = *info;
  counter->count = 0;

  if (map_entry(counts, &counter->token, &entry)) {
    token_info_deinit(&counter->token);
    free(counter);
    counter = map_entry_value(&entry);
  } else {
    map_entry_update(&entry, counter);
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

static void list_token(Map *counts, Vector *list)
{
  MapIterator iterator;
  map_iterator(&iterator, counts);
  vector_init_with_capacity(list, sizeof(TokenCountEntry), map_count(counts));
  while (map_iterator_next(&iterator)) {
    vector_push(list, map_iterator_value(&iterator));
    free(map_iterator_value(&iterator));
  }
  map_deinit(counts);
  qsort(vector_data(list), vector_count(list), sizeof(TokenCountEntry), &token_info_compare);
}

static void token_count_init(TokenCount *count, const Source *source)
{
  TokenInfo token;
  Report    report;
  Map       token_counts;
  Map       identifier_counts;

  unsigned long offset = 0;

  map_init(&token_counts, &token_info_hash, &token_info_equal);
  map_init(&identifier_counts, &token_info_hash, &token_info_equal);
  while (mppl_lex(source, offset, &token, &report) && token.kind != SYNTAX_KIND_EOF_TOKEN) {
    offset += token.text_length;
    if (syntax_kind_is_trivia(token.kind)) {
      token_info_deinit(&token);
      continue;
    }

    switch (token.kind) {
    case SYNTAX_KIND_IDENTIFIER_TOKEN:
      increment_token(&identifier_counts, &token);
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
    increment_token(&token_counts, &token);
  }
  token_info_deinit(&token);
  list_token(&token_counts, &count->token_counts);
  list_token(&identifier_counts, &count->identifer_counts);
}

static void token_count_deinit(TokenCount *count)
{
  unsigned long i;

  for (i = 0; i < vector_count(&count->token_counts); ++i) {
    token_info_deinit(vector_at(&count->token_counts, i));
  }
  vector_deinit(&count->token_counts);

  for (i = 0; i < vector_count(&count->identifer_counts); ++i) {
    token_info_deinit(vector_at(&count->identifer_counts, i));
  }
  vector_deinit(&count->identifer_counts);
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
    unsigned long max_token_width      = get_max_token_display_width(vector_data(&count->token_counts), vector_count(&count->token_counts));
    unsigned long max_identifier_width = identifier_prefix_width + get_max_token_display_width(vector_data(&count->identifer_counts), vector_count(&count->identifer_counts));

    max_token_display_width = max_token_width > max_identifier_width ? max_token_width : max_identifier_width;
  }

  {
    unsigned long max_token_count_width      = get_max_count_display_width(vector_data(&count->token_counts), vector_count(&count->token_counts));
    unsigned long max_identifier_count_width = get_max_count_display_width(vector_data(&count->identifer_counts), vector_count(&count->identifer_counts));

    max_count_display_width = max_token_count_width > max_identifier_count_width ? max_token_count_width : max_identifier_count_width;
  }

  for (i = 0; i < vector_count(&count->token_counts); ++i) {
    TokenCountEntry *token_entry       = vector_at(&count->token_counts, i);
    unsigned long    token_space_width = (max_token_display_width - get_token_display_width(token_entry))
      + (max_count_display_width - get_count_display_width(token_entry)) + 2;
    printf("\"%s\"%*c%lu\n", token_entry->token.text, (int) token_space_width, ' ', token_entry->count);

    if (token_entry->token.kind == SYNTAX_KIND_IDENTIFIER_TOKEN) {
      for (j = 0; j < vector_count(&count->identifer_counts); ++j) {
        TokenCountEntry *identifier_entry       = vector_at(&count->identifer_counts, j);
        unsigned long    identifier_space_width = (max_token_display_width - identifier_prefix_width - get_token_display_width(identifier_entry))
          + (max_count_display_width - get_count_display_width(identifier_entry)) + 2;
        printf("%s\"%s\"%*c%lu\n", identifier_prefix, identifier_entry->token.text, (int) identifier_space_width, ' ', identifier_entry->count);
      }
    }
  }
}

void task1(int argc, const char **argv)
{
  Source     source;
  TokenCount counter;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  source_init(&source, argv[1], strlen(argv[1]));
  token_count_init(&counter, &source);
  token_count_print(&counter);
  token_count_deinit(&counter);
  source_deinit(&source);
}
