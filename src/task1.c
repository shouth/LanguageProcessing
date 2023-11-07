#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "module.h"
#include "syntax_kind.h"
#include "tasks.h"
#include "token.h"
#include "token_cursor.h"
#include "vector.h"

typedef struct TokenCountEntry TokenCountEntry;

struct TokenCountEntry {
  const Token  *token;
  unsigned long count;
};

static const Token TOKEN_NAME   = { { SYNTAX_KIND_IDENTIFIER }, "NAME", 4 };
static const Token TOKEN_NUMBER = { { SYNTAX_KIND_INTEGER }, "NUMBER", 6 };
static const Token TOKEN_STRING = { { SYNTAX_KIND_STRING }, "STRING", 6 };

static void count_token(TokenCursor *cursor, Map *token_counts, Map *identifier_counts)
{
  const Token *token;
  while ((token = token_cursor_next(cursor))) {
    MapEntry      entry;
    unsigned long count;

    switch (token->node.kind) {
    case SYNTAX_KIND_IDENTIFIER: {
      count = 0;
      if (map_entry(identifier_counts, (void *) token, &entry)) {
        count = (unsigned long) map_entry_value(&entry);
      }
      map_entry_update(&entry, (void *) (count + 1));
      token = &TOKEN_NAME;
      break;
    }

    case SYNTAX_KIND_INTEGER:
      token = &TOKEN_NUMBER;
      break;

    case SYNTAX_KIND_STRING:
      token = &TOKEN_STRING;
      break;

    case SYNTAX_KIND_SPACE:
    case SYNTAX_KIND_NEWLINE:
    case SYNTAX_KIND_BRACES_COMMENT:
    case SYNTAX_KIND_C_COMMENT:
      continue;

    default:
      break;
    }

    count = 0;
    if (map_entry(token_counts, (void *) token, &entry)) {
      count = (unsigned long) map_entry_value(&entry);
    }
    map_entry_update(&entry, (void *) (count + 1));
  }
}

static int token_count_entry_comparator(const void *left, const void *right)
{
  const TokenCountEntry *l = left;
  const TokenCountEntry *r = right;

  if (l->count != r->count) {
    return r->count < l->count ? -1 : 1;
  } else if (l->token->node.kind != r->token->node.kind) {
    return l->token->node.kind < r->token->node.kind ? -1 : 1;
  } else {
    return strcmp(l->token->string, r->token->string);
  }
}

static void token_count_list_init(Vector *list, Map *counts)
{
  MapIterator iterator;
  map_iterator(&iterator, counts);
  vector_init_with_capacity(list, sizeof(TokenCountEntry), map_size(counts));
  while (map_iterator_next(&iterator)) {
    TokenCountEntry entry;
    entry.token = map_iterator_key(&iterator);
    entry.count = (unsigned long) map_iterator_value(&iterator);
    vector_push(list, &entry);
  }
  qsort(vector_data(list), vector_length(list), sizeof(TokenCountEntry), &token_count_entry_comparator);
}

static void token_count_list_deinit(Vector *list)
{
  vector_deinit(list);
}

static void get_display_width(Vector *list, int *name_width, int *count_width)
{
  unsigned long i;
  *name_width  = 0;
  *count_width = 0;
  for (i = 0; i < vector_length(list); ++i) {
    TokenCountEntry *entry           = vector_at(list, i);
    int              new_name_width  = entry->token->size;
    int              new_count_width = 1;
    unsigned long    count;
    for (count = entry->count; count > 9; count /= 10) {
      ++new_count_width;
    }
    *name_width  = *name_width > new_name_width ? *name_width : new_name_width;
    *count_width = *count_width > new_count_width ? *count_width : new_count_width;
  }
}

static void token_counts_print_identifier(Map *counts)
{
  unsigned long i;
  int           name_width, count_width;
  Vector        identifier_count_list;

  token_count_list_init(&identifier_count_list, counts);
  get_display_width(&identifier_count_list, &name_width, &count_width);
  for (i = 0; i < vector_length(&identifier_count_list); ++i) {
    TokenCountEntry *entry       = vector_at(&identifier_count_list, i);
    int              space_width = name_width - (int) entry->token->size + 2;
    printf("    \"Identifier\" \"%s\"%*c%*ld\n", entry->token->string, space_width, ' ', count_width, entry->count);
  }
  token_count_list_deinit(&identifier_count_list);
}

static void token_counts_print(Map *token_counts, Map *identifier_counts)
{
  unsigned long i;
  int           name_width, count_width;
  Vector        token_count_list;

  token_count_list_init(&token_count_list, token_counts);
  get_display_width(&token_count_list, &name_width, &count_width);
  for (i = 0; i < vector_length(&token_count_list); ++i) {
    TokenCountEntry *entry       = vector_at(&token_count_list, i);
    int              space_width = name_width - (int) entry->token->size + 2;
    printf("\"%s\"%*c%*ld\n", entry->token->string, space_width, ' ', count_width, entry->count);
    if (entry->token->node.kind == SYNTAX_KIND_IDENTIFIER) {
      token_counts_print_identifier(identifier_counts);
    }
  }
  token_count_list_deinit(&token_count_list);
}

void task1(int argc, const char **argv)
{
  Module      module;
  TokenCursor cursor;
  Map         token_counts, identifier_counts;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  module_init(&module, argv[1]);
  module_token_cursor(&module, &cursor);

  map_init(&token_counts, NULL, NULL);
  map_init(&identifier_counts, NULL, NULL);
  count_token(&cursor, &token_counts, &identifier_counts);
  token_counts_print(&token_counts, &identifier_counts);
  map_deinit(&token_counts);
  map_deinit(&identifier_counts);

  module_deinit(&module);
}
