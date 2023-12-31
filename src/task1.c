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

typedef struct CounterToken CounterToken;
typedef struct CounterEntry CounterEntry;
typedef struct Counter      Counter;

struct CounterToken {
  SyntaxKind    kind;
  char         *text;
  unsigned long text_length;
};

struct CounterEntry {
  CounterToken  token;
  unsigned long count;
};

struct Counter {
  Array *token_counts;
  Array *identifer_counts;
};

static void counter_entry_init(CounterEntry *entry, SyntaxKind kind, const char *text, unsigned long text_length)
{
  entry->count      = 0;
  entry->token.kind = kind;
  entry->token.text = xmalloc(sizeof(char) * (text_length + 1));
  strncpy(entry->token.text, text, text_length);
  entry->token.text[text_length] = '\0';
  entry->token.text_length       = text_length;
}

static void counter_entry_deinit(CounterEntry *entry)
{
  free(entry->token.text);
}

static void increment_token(Map *counts, SyntaxKind kind, const char *text, unsigned long text_length)
{
  CounterEntry *counter = xmalloc(sizeof(CounterEntry));
  MapIndex      index;

  counter_entry_init(counter, kind, text, text_length);
  if (map_find(counts, &counter->token, &index)) {
    counter_entry_deinit(counter);
    free(counter);
    counter = map_value(counts, &index);
  } else {
    map_update(counts, &index, &counter->token, counter);
  }
  ++counter->count;
}

static unsigned long counter_token_hash(const void *key)
{
  const CounterToken *token = key;
  unsigned long       hash  = FNV1A_INIT;
  hash                      = fnv1a(hash, &token->kind, sizeof(SyntaxKind));
  hash                      = fnv1a(hash, token->text, token->text_length);
  return hash;
}

static int counter_token_compare(const void *left, const void *right)
{
  const CounterToken *l = left;
  const CounterToken *r = right;

  if (l->kind < r->kind) {
    return -1;
  } else if (l->kind > r->kind) {
    return 1;
  } else {
    return strcmp(l->text, r->text);
  }
}

static int counter_token_equal(const void *left, const void *right)
{
  return !counter_token_compare(left, right);
}

static Array *list_token(Map *counts)
{
  Array   *list = array_new_with_capacity(sizeof(CounterEntry), map_count(counts));
  MapIndex entry;
  map_index(counts, &entry);
  while (map_next(counts, &entry)) {
    array_push(list, map_value(counts, &entry));
    free(map_value(counts, &entry));
  }
  map_free(counts);
  qsort(array_data(list), array_count(list), sizeof(CounterEntry), &counter_token_compare);
  return list;
}

static TokenStatus token_count_init(Counter *count, const Source *source)
{
  LexedToken  token;
  TokenStatus status;
  Map        *token_counts;
  Map        *identifier_counts;

  unsigned long offset = 0;

  token_counts      = map_new(&counter_token_hash, &counter_token_equal);
  identifier_counts = map_new(&counter_token_hash, &counter_token_equal);
  while (1) {
    status = mppl_lex(source, offset, &token);
    if (status != TOKEN_OK) {
      break;
    }

    offset += token.length;
    if (syntax_kind_is_trivia(token.kind)) {
      continue;
    }

    switch (token.kind) {
    case SYNTAX_KIND_IDENTIFIER_TOKEN:
      increment_token(identifier_counts, token.kind, source->text + token.offset, token.length);
      increment_token(token_counts, SYNTAX_KIND_IDENTIFIER_TOKEN, "NAME", 4);
      break;
    case SYNTAX_KIND_INTEGER_LITERAL:
      increment_token(token_counts, SYNTAX_KIND_INTEGER_LITERAL, "NUMBER", 6);
      break;
    case SYNTAX_KIND_STRING_LITERAL:
      increment_token(token_counts, SYNTAX_KIND_STRING_LITERAL, "STRING", 6);
      break;
    default:
      increment_token(token_counts, token.kind, source->text + token.offset, token.length);
      break;
    }
  }
  count->token_counts     = list_token(token_counts);
  count->identifer_counts = list_token(identifier_counts);
  return status;
}

static void token_count_deinit(Counter *count)
{
  unsigned long i;

  for (i = 0; i < array_count(count->token_counts); ++i) {
    CounterEntry *entry = array_at(count->token_counts, i);
    counter_entry_deinit(entry);
  }
  array_free(count->token_counts);

  for (i = 0; i < array_count(count->identifer_counts); ++i) {
    CounterEntry *entry = array_at(count->identifer_counts, i);
    counter_entry_deinit(entry);
  }
  array_free(count->identifer_counts);
}

unsigned long get_token_display_width(CounterEntry *entry)
{
  return entry->token.text_length;
}

unsigned long get_max_token_display_width(CounterEntry *entries, unsigned long length)
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

unsigned long get_count_display_width(CounterEntry *entry)
{
  unsigned long result = 1;
  unsigned long count  = entry->count;
  while (count > 9) {
    ++result;
    count /= 10;
  }
  return result;
}

unsigned long get_max_count_display_width(CounterEntry *entries, unsigned long length)
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

static void token_count_print(Counter *count)
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
    CounterEntry *token_entry       = array_at(count->token_counts, i);
    unsigned long token_space_width = (max_token_display_width - get_token_display_width(token_entry))
      + (max_count_display_width - get_count_display_width(token_entry)) + 2;
    printf("\"%s\"%*c%lu\n", token_entry->token.text, (int) token_space_width, ' ', token_entry->count);

    if (token_entry->token.kind == SYNTAX_KIND_IDENTIFIER_TOKEN) {
      for (j = 0; j < array_count(count->identifer_counts); ++j) {
        CounterEntry *identifier_entry = array_at(count->identifer_counts, j);
        unsigned long identifier_space_width
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
  Source *source;
  Counter counter;

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
