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
#include "utility.h"
#include "vector.h"

#define TOKEN_COUNT_ENTRY_LENGTH (SYNTAX_KIND_KEYWORD_BREAK + 1)

typedef struct TokenCountEntry TokenCountEntry;
typedef struct TokenCount      TokenCount;

struct TokenCountEntry {
  Token        *tokens;
  unsigned long count;
};

struct TokenCount {
  TokenCountEntry  entries[TOKEN_COUNT_ENTRY_LENGTH];
  TokenCountEntry *identifiers;
  unsigned long    identifiers_length;
};

static unsigned long token_hasher(const void *token)
{
  return fnv1a(FNV1A_INIT, token, strlen(token));
}

static int token_comparator(const void *left, const void *right)
{
  return !strcmp(left, right);
}

static int identifier_comparator(const void *left, const void *right)
{
  const TokenCountEntry *l = left;
  const TokenCountEntry *r = right;
  return strcmp(l->tokens->info.token, r->tokens->info.token);
}

static void token_count_init(TokenCount *token_count, TokenCursor *cursor)
{
  unsigned long i;
  {
    Vector entries[TOKEN_COUNT_ENTRY_LENGTH];

    for (i = 0; i < TOKEN_COUNT_ENTRY_LENGTH; ++i) {
      vector_init(entries + i, sizeof(Token));
    }

    {
      Token token;
      while (token_cursor_next(cursor, &token)) {
        if (token.info.kind < TOKEN_COUNT_ENTRY_LENGTH) {
          vector_push(entries + token.info.kind, &token);
        } else {
          token_deinit(&token);
        }
      }
    }

    for (i = 0; i < TOKEN_COUNT_ENTRY_LENGTH; ++i) {
      TokenCountEntry *entry = token_count->entries + i;
      vector_fit(entries + i);
      entry->count  = vector_length(entries + i);
      entry->tokens = vector_steal(entries + i);
    }
  }

  {
    TokenCountEntry *identifier = token_count->entries + SYNTAX_KIND_IDENTIFIER;
    Map              identifiers;
    map_init(&identifiers, &token_hasher, &token_comparator);
    for (i = 0; i < identifier->count; ++i) {
      Token  *token = identifier->tokens + i;
      Vector *vector;
      {
        MapEntry entry;
        if (map_entry(&identifiers, token->info.token, &entry)) {
          vector = map_entry_value(&entry);
        } else {
          vector = xmalloc(sizeof(Vector));
          vector_init(vector, sizeof(Token));
          map_entry_update(&entry, vector);
        }
      }
      vector_push(vector, token);
    }

    token_count->identifiers_length = map_size(&identifiers);
    token_count->identifiers        = xmalloc(sizeof(TokenCountEntry) * token_count->identifiers_length);
    {
      MapIterator iterator;
      map_iterator(&iterator, &identifiers);
      for (i = 0; map_iterator_next(&iterator); ++i) {
        TokenCountEntry *entry = token_count->identifiers + i;
        vector_fit(map_iterator_value(&iterator));
        entry->count  = vector_length(map_iterator_value(&iterator));
        entry->tokens = vector_steal(map_iterator_value(&iterator));
        free(map_iterator_value(&iterator));
      }
    }
    qsort(token_count->identifiers, token_count->identifiers_length, sizeof(TokenCountEntry), &identifier_comparator);
    map_deinit(&identifiers);
  }
}

static const char *token_kind_string(Token *token)
{
  switch (token->info.kind) {
  case SYNTAX_KIND_IDENTIFIER:
    return "NAME";
  case SYNTAX_KIND_INTEGER:
    return "NUMBER";
  case SYNTAX_KIND_STRING:
    return "STRING";
  default:
    return token->info.token;
  }
}

static unsigned long get_name_display_width(TokenCountEntry *entries, unsigned long length, int direct)
{
  unsigned long i;
  unsigned long result = 0;

  for (i = 0; i < length; ++i) {
    TokenCountEntry *entry = entries + i;
    if (entry->count > 0) {
      unsigned long width = direct ? entry->tokens->info.length : strlen(token_kind_string(entry->tokens));
      if (result < width) {
        result = width;
      }
    }
  }
  return result;
}

static unsigned long get_count_display_width(TokenCountEntry *entries, unsigned long length)
{
  unsigned long i;
  unsigned long result = 0;

  for (i = 0; i < length; ++i) {
    TokenCountEntry *entry = entries + i;
    if (entry->count > 0) {
      char          buffer[32];
      unsigned long width = sprintf(buffer, "%ld", entry->count);
      if (result < width) {
        result = width;
      }
    }
  }
  return result;
}

static void token_count_print(TokenCount *token_count)
{
  SyntaxKind    kind;
  unsigned long name_display_width;
  unsigned long count_display_width;
  unsigned long identifier_padding = sizeof("    \"Identifier\" ") - 1;

  {
    unsigned long token_display_width      = get_name_display_width(token_count->entries, TOKEN_COUNT_ENTRY_LENGTH, 0);
    unsigned long identifier_display_width = identifier_padding + get_name_display_width(token_count->identifiers, token_count->identifiers_length, 1);

    name_display_width = token_display_width > identifier_display_width ? token_display_width : identifier_display_width;
  }

  {
    unsigned long token_count_display_width      = get_count_display_width(token_count->entries, TOKEN_COUNT_ENTRY_LENGTH);
    unsigned long identifier_count_display_width = get_count_display_width(token_count->identifiers, token_count->identifiers_length);

    count_display_width = token_count_display_width > identifier_count_display_width ? token_count_display_width : identifier_count_display_width;
  }

  for (kind = 0; kind < TOKEN_COUNT_ENTRY_LENGTH; ++kind) {
    TokenCountEntry *entry = token_count->entries + kind;
    if (entry->count > 0) {
      char buffer[32];

      unsigned long space_width = 2 + (name_display_width - strlen(token_kind_string(entry->tokens)))
        + (count_display_width - sprintf(buffer, "%ld", entry->count));
      printf("\"%s\"%*c%lu\n", token_kind_string(entry->tokens), (int) space_width, ' ', entry->count);

      if (entry->tokens->info.kind == SYNTAX_KIND_IDENTIFIER) {
        unsigned long i;
        for (i = 0; i < token_count->identifiers_length; ++i) {
          TokenCountEntry *identifier = token_count->identifiers + i;
          space_width                 = 2 + (name_display_width - identifier_padding - identifier->tokens->info.length)
            + (count_display_width - sprintf(buffer, "%ld", identifier->count));
          printf("    \"Identifier\" \"%s\"%*c%lu\n", identifier->tokens->info.token, (int) space_width, ' ', identifier->count);
        }
      }
    }
  }
}

static void token_count_deinit(TokenCount *token_count)
{
  unsigned long i;
  for (i = 0; i < TOKEN_COUNT_ENTRY_LENGTH; ++i) {
    TokenCountEntry *entry = token_count->entries + i;
    unsigned long    i;
    for (i = 0; i < entry->count; ++i) {
      token_deinit(entry->tokens + i);
    }
    free(entry->tokens);
  }

  for (i = 0; i < token_count->identifiers_length; ++i) {
    TokenCountEntry *entry = token_count->identifiers + i;
    free(entry->tokens);
  }
  free(token_count->identifiers);
}

void task1(int argc, const char **argv)
{
  Module      module;
  TokenCursor cursor;
  TokenCount  token_count;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  module_init(&module, argv[1]);
  module_token_cursor(&module, &cursor);

  token_count_init(&token_count, &cursor);
  token_count_print(&token_count);
  token_count_deinit(&token_count);

  module_deinit(&module);
}
