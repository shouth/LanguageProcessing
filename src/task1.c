#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "module.h"
#include "symbol.h"
#include "syntax_kind.h"
#include "tasks.h"
#include "token_cursor.h"
#include "utility.h"
#include "vector.h"

typedef struct TokenCountEntry TokenCountEntry;
typedef struct TokenCounts     TokenCounts;

struct TokenCountEntry {
  Symbol        symbol;
  unsigned long count;
};

struct TokenCounts {
  Map token;
  Map identifier;

  struct {
    Symbol name;
    Symbol number;
    Symbol string;
  } symbol;
};

static void count_token(TokenCounts *counts, TokenCursor *cursor)
{
  Token token;
  while (token_cursor_next(cursor, &token)) {
    MapEntry      entry;
    unsigned long count;
    Symbol        symbol;

    switch (token.kind) {
    case SYNTAX_KIND_IDENTIFIER: {
      count = 0;
      if (map_entry(&counts->identifier, (void *) token.symbol, &entry)) {
        count = (unsigned long) map_entry_value(&entry);
      }
      map_entry_update(&entry, (void *) (count + 1));
      symbol = counts->symbol.name;
      break;
    }

    case SYNTAX_KIND_INTEGER:
      symbol = counts->symbol.number;
      break;

    case SYNTAX_KIND_STRING:
      symbol = counts->symbol.string;
      break;

    case SYNTAX_KIND_SPACE:
    case SYNTAX_KIND_NEWLINE:
    case SYNTAX_KIND_BRACES_COMMENT:
    case SYNTAX_KIND_C_COMMENT:
      continue;

    default:
      symbol = token.symbol;
      break;
    }

    count = 0;
    if (map_entry(&counts->token, (void *) symbol, &entry)) {
      count = (unsigned long) map_entry_value(&entry);
    }
    map_entry_update(&entry, (void *) (count + 1));
  }
}

static int token_count_entry_comparator(const void *left, const void *right)
{
  const TokenCountEntry *l = *(void **) left;
  const TokenCountEntry *r = *(void **) right;

  if (l->count != r->count) {
    return r->count < l->count ? -1 : 1;
  } else {
    return strcmp(symbol_string(l->symbol), symbol_string(r->symbol));
  }
}

static void token_count_list_init(Vector *list, Map *counts)
{
  MapIterator iterator;
  map_iterator(&iterator, counts);
  vector_init_with_capacity(list, map_size(counts));
  while (map_iterator_next(&iterator)) {
    TokenCountEntry *entry = xmalloc(sizeof(TokenCountEntry));
    entry->symbol          = (Symbol) map_iterator_key(&iterator);
    entry->count           = (unsigned long) map_iterator_value(&iterator);
    vector_push_back(list, entry);
  }
  qsort(vector_data(list), vector_size(list), sizeof(void *), &token_count_entry_comparator);
}

static void token_count_list_deinit(Vector *list)
{
  while (vector_size(list) > 0) {
    free(vector_back(list));
    vector_pop_back(list);
  }
  vector_deinit(list);
}

static void get_display_width(Vector *list, int *name_width, int *count_width)
{
  unsigned long i;
  *name_width  = 0;
  *count_width = 0;
  for (i = 0; i < vector_size(list); ++i) {
    TokenCountEntry *entry           = vector_data(list)[i];
    int              new_name_width  = symbol_size(entry->symbol);
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
  Vector        count_list;

  token_count_list_init(&count_list, counts);
  get_display_width(&count_list, &name_width, &count_width);
  for (i = 0; i < vector_size(&count_list); ++i) {
    TokenCountEntry *entry       = vector_data(&count_list)[i];
    int              space_width = name_width - (int) symbol_size(entry->symbol) + 2;
    printf("    \"Identifier\" \"%s\"%*c%*ld\n", symbol_string(entry->symbol), space_width, ' ', count_width, entry->count);
  }
  token_count_list_deinit(&count_list);
}

static void token_counts_print(TokenCounts *counts)
{
  unsigned long i;
  int           name_width, count_width;
  Vector        token_count_list;
  Vector        identifier_count_list;

  token_count_list_init(&token_count_list, &counts->token);
  token_count_list_init(&identifier_count_list, &counts->identifier);
  get_display_width(&token_count_list, &name_width, &count_width);
  for (i = 0; i < vector_size(&token_count_list); ++i) {
    TokenCountEntry *entry       = vector_data(&token_count_list)[i];
    int              space_width = name_width - (int) symbol_size(entry->symbol) + 2;
    printf("\"%s\"%*c%*ld\n", symbol_string(entry->symbol), space_width, ' ', count_width, entry->count);
    if (entry->symbol == counts->symbol.name) {
      token_counts_print_identifier(&counts->identifier);
    }
  }
  token_count_list_deinit(&token_count_list);
  token_count_list_deinit(&identifier_count_list);
}

static void token_counts_init(TokenCounts *counts, TokenCursor *cursor)
{
  map_init(&counts->token, NULL, NULL);
  map_init(&counts->identifier, NULL, NULL);
  counts->symbol.name   = symbol_from("NAME", sizeof("NAME") - 1);
  counts->symbol.number = symbol_from("NUMBER", sizeof("NUMBER") - 1);
  counts->symbol.string = symbol_from("STRING", sizeof("STRING") - 1);
  count_token(counts, cursor);
}

static void token_counts_deinit(TokenCounts *counts)
{
  map_deinit(&counts->token);
  map_deinit(&counts->identifier);
}

void task1(int argc, const char **argv)
{
  Module      module;
  TokenCursor cursor;
  TokenCounts counts;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  module_init(&module, argv[1]);
  module_token_cursor_init(&module, &cursor);
  token_counts_init(&counts, &cursor);
  token_counts_print(&counts);
  token_counts_deinit(&counts);
  module_deinit(&module);
}
