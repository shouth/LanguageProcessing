#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "compiler.h"
#include "context.h"
#include "map.h"
#include "source.h"
#include "syntax_kind.h"
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

static CounterEntry *counter_entry_new(SyntaxKind kind, const char *text, unsigned long text_length)
{
  CounterEntry *entry = xmalloc(sizeof(CounterEntry));
  entry->count        = 0;
  entry->token.kind   = kind;
  entry->token.text   = xmalloc(sizeof(char) * (text_length + 1));
  strncpy(entry->token.text, text, text_length);
  entry->token.text[text_length] = '\0';
  entry->token.text_length       = text_length;
  return entry;
}

static void counter_entry_free(CounterEntry *entry)
{
  if (entry) {
    free(entry->token.text);
    free(entry);
  }
}

static void increment_token(Map *counts, SyntaxKind kind, const char *text, unsigned long text_length)
{
  CounterEntry *counter = counter_entry_new(kind, text, text_length);
  MapIndex      index;
  if (map_entry(counts, &counter->token, &index)) {
    counter_entry_free(counter);
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

  return l->kind == r->kind && l->text_length == r->text_length && strcmp(l->text, r->text) == 0;
}

static int counter_entry_ptr_compare(const void *left, const void *right)
{
  const CounterEntry *l = *(const CounterEntry **) left;
  const CounterEntry *r = *(const CounterEntry **) right;

  if (l->token.kind != r->token.kind) {
    return l->token.kind < r->token.kind ? -1 : 1;
  } else {
    return strcmp(l->token.text, r->token.text);
  }
}

static Array *list_token(Map *counts)
{
  Array   *list = array_new_with_capacity(sizeof(CounterEntry *), map_count(counts));
  MapIndex entry;
  for (map_iterator(counts, &entry); map_next(counts, &entry);) {
    CounterEntry *value = map_value(counts, &entry);
    array_push(list, &value);
  }
  map_free(counts);
  qsort(array_data(list), array_count(list), sizeof(CounterEntry *), &counter_entry_ptr_compare);
  return list;
}

static LexStatus token_count_init(Counter *count, const Source *source)
{
  LexedToken token;
  LexStatus  status;
  Map       *token_counts;
  Map       *identifier_counts;

  unsigned long offset = 0;

  token_counts      = map_new(&counter_token_hash, &counter_token_compare);
  identifier_counts = map_new(&counter_token_hash, &counter_token_compare);
  while (1) {
    status = mpplc_lex(source, offset, &token);
    if (status != LEX_OK) {
      break;
    }

    offset += token.length;
    if (syntax_kind_is_trivia(token.kind)) {
      continue;
    }

    switch (token.kind) {
    case SYNTAX_IDENT_TOKEN:
      increment_token(identifier_counts, token.kind, source->text + token.offset, token.length);
      increment_token(token_counts, SYNTAX_IDENT_TOKEN, "NAME", 4);
      break;
    case SYNTAX_NUMBER_LIT:
      increment_token(token_counts, SYNTAX_NUMBER_LIT, "NUMBER", 6);
      break;
    case SYNTAX_STRING_LIT:
      increment_token(token_counts, SYNTAX_STRING_LIT, "STRING", 6);
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
    CounterEntry **entry = array_at(count->token_counts, i);
    counter_entry_free(*entry);
  }
  array_free(count->token_counts);

  for (i = 0; i < array_count(count->identifer_counts); ++i) {
    CounterEntry **entry = array_at(count->identifer_counts, i);
    counter_entry_free(*entry);
  }
  array_free(count->identifer_counts);
}

unsigned long get_token_display_width(CounterEntry *entry)
{
  return entry->token.text_length;
}

unsigned long get_max_token_display_width(CounterEntry **entries, unsigned long length)
{
  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < length; ++i) {
    unsigned long width = get_token_display_width(entries[i]);
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

unsigned long get_max_count_display_width(CounterEntry **entries, unsigned long length)
{
  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < length; ++i) {
    unsigned long width = get_count_display_width(entries[i]);
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
    CounterEntry *token_entry       = *(CounterEntry **) array_at(count->token_counts, i);
    unsigned long token_space_width = (max_token_display_width - get_token_display_width(token_entry))
      + (max_count_display_width - get_count_display_width(token_entry)) + 2;
    printf("\"%s\"%*c%lu\n", token_entry->token.text, (int) token_space_width, ' ', token_entry->count);

    if (token_entry->token.kind == SYNTAX_IDENT_TOKEN) {
      for (j = 0; j < array_count(count->identifer_counts); ++j) {
        CounterEntry *identifier_entry = *(CounterEntry **) array_at(count->identifer_counts, j);
        unsigned long identifier_space_width
          = (max_token_display_width - identifier_prefix_width - get_token_display_width(identifier_entry))
          + (max_count_display_width - get_count_display_width(identifier_entry)) + 2;
        printf("%s\"%s\"%*c%lu\n",
          identifier_prefix, identifier_entry->token.text, (int) identifier_space_width, ' ', identifier_entry->count);
      }
    }
  }
}

int mpplc_task1(int argc, const char **argv)
{
  int status = EXIT_SUCCESS;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s INPUT\n", argv[0]);
    status = EXIT_FAILURE;
  } else {
    Source *source = source_new(argv[1], strlen(argv[1]));
    if (!source) {
      fprintf(stderr, "Cannot open file: %s\n", argv[1]);
      status = EXIT_FAILURE;
    } else {
      Counter counter;
      switch (token_count_init(&counter, source)) {
      case LEX_EOF:
        token_count_print(&counter);
        break;

      case LEX_ERROR_STRAY_CHAR:
        fprintf(stderr, "Error: Stray character in program\n");
        status = EXIT_FAILURE;
        break;

      case LEX_ERROR_NONGRAPHIC_CHAR:
        fprintf(stderr, "Error: Non-graphic character in string\n");
        status = EXIT_FAILURE;
        break;

      case LEX_ERROR_UNTERMINATED_STRING:
        fprintf(stderr, "Error: String is unterminated\n");
        status = EXIT_FAILURE;
        break;

      case LEX_ERROR_UNTERMINATED_COMMENT:
        fprintf(stderr, "Error: Comment is unterminated\n");
        status = EXIT_FAILURE;
        break;

      default:
        unreachable();
      }
      source_free(source);
      token_count_deinit(&counter);
    }
  }
  return status;
}

int mpplc_task2(int argc, const char **argv)
{
  int status = EXIT_SUCCESS;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s INPUT\n", argv[0]);
    status = EXIT_FAILURE;
  } else {
    Source      *source = source_new(argv[1], strlen(argv[1]));
    MpplProgram *syntax = NULL;
    if (!source) {
      fprintf(stderr, "Cannot open file: %s\n", argv[1]);
      status = EXIT_FAILURE;
    } else if (mpplc_parse(source, NULL, &syntax)) {
      mpplc_pretty_print(syntax, NULL);
      status = EXIT_FAILURE;
    }
    mppl_unref(syntax);
    source_free(source);
  }
  return status;
}

int mpplc_task3(int argc, const char **argv)
{
  fprintf(stderr, "Task 3 is not implemented\n");
  return EXIT_FAILURE;
}

int mpplc_task4(int argc, const char **argv)
{
  int status = EXIT_SUCCESS;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s INPUT\n", argv[0]);
    status = EXIT_FAILURE;
  } else {
    Ctx         *ctx    = ctx_new();
    Source      *source = source_new(argv[1], strlen(argv[1]));
    MpplProgram *syntax = NULL;
    if (!source) {
      fprintf(stderr, "Cannot open file: %s\n", argv[1]);
      status = EXIT_FAILURE;
    } else if (mpplc_parse(source, ctx, &syntax) && mpplc_resolve(source, syntax, ctx) && mpplc_check(source, syntax, ctx)) {
      mpplc_codegen_casl2(source, syntax, ctx);
    }
    mppl_unref(syntax);
    source_free(source);
    ctx_free(ctx);
  }
  return status;
}
