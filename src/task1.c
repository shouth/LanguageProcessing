#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lexical_tree.h"
#include "map.h"
#include "module.h"
#include "symbol.h"
#include "tasks.h"
#include "utility.h"
#include "vector.h"

typedef struct TokenCountEntry TokenCountEntry;

struct TokenCountEntry {
  Symbol        symbol;
  unsigned long count;
};

static SyntaxKind convert_kind(LexerTokenKind kind, Symbol token)
{
  switch (kind) {
  case LEXER_TOKEN_KIND_IDENTIFIER: {
    switch (token) {
    case SYMBOL_KEYWORD_AND:
      return SYNTAX_KIND_KEYWORD_AND;
    case SYMBOL_KEYWORD_ARRAY:
      return SYNTAX_KIND_KEYWORD_ARRAY;
    case SYMBOL_KEYWORD_BEGIN:
      return SYNTAX_KIND_KEYWORD_BEGIN;
    case SYMBOL_KEYWORD_BOOLEAN:
      return SYNTAX_KIND_KEYWORD_BOOLEAN;
    case SYMBOL_KEYWORD_BREAK:
      return SYNTAX_KIND_KEYWORD_BREAK;
    case SYMBOL_KEYWORD_CALL:
      return SYNTAX_KIND_KEYWORD_CALL;
    case SYMBOL_KEYWORD_CHAR:
      return SYNTAX_KIND_KEYWORD_CHAR;
    case SYMBOL_KEYWORD_DIV:
      return SYNTAX_KIND_KEYWORD_DIV;
    case SYMBOL_KEYWORD_DO:
      return SYNTAX_KIND_KEYWORD_DO;
    case SYMBOL_KEYWORD_ELSE:
      return SYNTAX_KIND_KEYWORD_ELSE;
    case SYMBOL_KEYWORD_END:
      return SYNTAX_KIND_KEYWORD_END;
    case SYMBOL_KEYWORD_FALSE:
      return SYNTAX_KIND_KEYWORD_FALSE;
    case SYMBOL_KEYWORD_IF:
      return SYNTAX_KIND_KEYWORD_IF;
    case SYMBOL_KEYWORD_INTEGER:
      return SYNTAX_KIND_KEYWORD_INTEGER;
    case SYMBOL_KEYWORD_NOT:
      return SYNTAX_KIND_KEYWORD_NOT;
    case SYMBOL_KEYWORD_OF:
      return SYNTAX_KIND_KEYWORD_OF;
    case SYMBOL_KEYWORD_OR:
      return SYNTAX_KIND_KEYWORD_OR;
    case SYMBOL_KEYWORD_PROCEDURE:
      return SYNTAX_KIND_KEYWORD_PROCEDURE;
    case SYMBOL_KEYWORD_PROGRAM:
      return SYNTAX_KIND_KEYWORD_PROGRAM;
    case SYMBOL_KEYWORD_READ:
      return SYNTAX_KIND_KEYWORD_READ;
    case SYMBOL_KEYWORD_READLN:
      return SYNTAX_KIND_KEYWORD_READLN;
    case SYMBOL_KEYWORD_RETURN:
      return SYNTAX_KIND_KEYWORD_RETURN;
    case SYMBOL_KEYWORD_THEN:
      return SYNTAX_KIND_KEYWORD_THEN;
    case SYMBOL_KEYWORD_TRUE:
      return SYNTAX_KIND_KEYWORD_TRUE;
    case SYMBOL_KEYWORD_VAR:
      return SYNTAX_KIND_KEYWORD_VAR;
    case SYMBOL_KEYWORD_WHILE:
      return SYNTAX_KIND_KEYWORD_WHILE;
    case SYMBOL_KEYWORD_WRITE:
      return SYNTAX_KIND_KEYWORD_WRITE;
    case SYMBOL_KEYWORD_WRITELN:
      return SYNTAX_KIND_KEYWORD_WRITELN;
    case SYMBOL_RANGE_KEYWORD_END:
      return SYNTAX_KIND_KEYWORD_END;
    default:
      return SYNTAX_KIND_IDENTIFIER;
    }
  }

  case LEXER_TOKEN_KIND_INTEGER:
    return SYNTAX_KIND_INTEGER;
  case LEXER_TOKEN_KIND_STRING:
    return SYNTAX_KIND_STRING;
  case LEXER_TOKEN_KIND_PLUS:
    return SYNTAX_KIND_PLUS;
  case LEXER_TOKEN_KIND_MINUS:
    return SYNTAX_KIND_MINUS;
  case LEXER_TOKEN_KIND_STAR:
    return SYNTAX_KIND_STAR;
  case LEXER_TOKEN_KIND_EQUAL:
    return SYNTAX_KIND_EQUAL;
  case LEXER_TOKEN_KIND_NOT_EQUAL:
    return SYNTAX_KIND_NOT_EQUAL;
  case LEXER_TOKEN_KIND_LESS_THAN:
    return SYNTAX_KIND_LESS_THAN;
  case LEXER_TOKEN_KIND_LESS_THAN_EQUAL:
    return SYNTAX_KIND_LESS_THAN_EQUAL;
  case LEXER_TOKEN_KIND_GREATER_THAN:
    return SYNTAX_KIND_GREATER_THAN;
  case LEXER_TOKEN_KIND_GREATER_THAN_EQUAL:
    return SYNTAX_KIND_GREATER_THAN_EQUAL;
  case LEXER_TOKEN_KIND_LEFT_PARENTHESIS:
    return SYNTAX_KIND_LEFT_PARENTHESIS;
  case LEXER_TOKEN_KIND_RIGHT_PARENTHESIS:
    return SYNTAX_KIND_RIGHT_PARENTHESIS;
  case LEXER_TOKEN_KIND_LEFT_BRACKET:
    return SYNTAX_KIND_LEFT_BRACKET;
  case LEXER_TOKEN_KIND_RIGHT_BRACKET:
    return SYNTAX_KIND_RIGHT_BRACKET;
  case LEXER_TOKEN_KIND_ASSIGN:
    return SYNTAX_KIND_ASSIGN;
  case LEXER_TOKEN_KIND_DOT:
    return SYNTAX_KIND_DOT;
  case LEXER_TOKEN_KIND_COMMA:
    return SYNTAX_KIND_COMMA;
  case LEXER_TOKEN_KIND_COLON:
    return SYNTAX_KIND_COLON;
  case LEXER_TOKEN_KIND_SEMICOLON:
    return SYNTAX_KIND_SEMICOLON;
  case LEXER_TOKEN_KIND_SPACE:
    return SYNTAX_KIND_SPACE;
  case LEXER_TOKEN_KIND_NEWLINE:
    return SYNTAX_KIND_NEWLINE;
  case LEXER_TOKEN_KIND_BRACES_COMMENT:
    return SYNTAX_KIND_BRACES_COMMENT;
  case LEXER_TOKEN_KIND_C_COMMENT:
    return SYNTAX_KIND_C_COMMENT;

  default:
    return SYNTAX_KIND_ERROR;
  }
}

static void count_token(const char *source, unsigned long size, Map *tokens, Map *identifiers)
{
  Lexer         lexer;
  unsigned long offset = 0;

  map_init(tokens, NULL, NULL);
  map_init(identifiers, NULL, NULL);

  lexer_init(&lexer, source, size);
  while (!lexer_eof(&lexer)) {
    LexerToken token;
    lexer_next_token(&lexer, &token);
    if (!lexer_token_trivial(&token)) {
      Symbol     symbol = symbol_from(source + offset, token.size);
      SyntaxKind kind   = convert_kind(token.kind, symbol);

      if (kind == SYNTAX_KIND_IDENTIFIER) {
        MapEntry      entry;
        unsigned long count = 0;
        if (map_entry(identifiers, (void *) symbol, &entry)) {
          count = (unsigned long) map_entry_value(&entry);
        }
        map_entry_update(&entry, (void *) (count + 1));
        symbol = symbol_from("NAME", sizeof("NAME") - 1);
      } else if (kind == SYNTAX_KIND_INTEGER) {
        symbol = symbol_from("NUMBER", sizeof("NUMBER") - 1);
      } else if (kind == SYNTAX_KIND_STRING) {
        symbol = symbol_from("STRING", sizeof("STRING") - 1);
      }

      {
        MapEntry      entry;
        unsigned long count = 0;
        if (map_entry(tokens, (void *) symbol, &entry)) {
          count = (unsigned long) map_entry_value(&entry);
        }
        map_entry_update(&entry, (void *) (count + 1));
      }
    }
    offset += token.size;
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

static void print_token_counts(Map *counts)
{
  unsigned long i;
  int           name_width, count_width;
  Vector        count_list;

  token_count_list_init(&count_list, counts);
  get_display_width(&count_list, &name_width, &count_width);
  for (i = 0; i < vector_size(&count_list); ++i) {
    TokenCountEntry *entry       = vector_data(&count_list)[i];
    int              space_width = name_width - (int) symbol_size(entry->symbol) + 2;
    printf("\"%s\"%*c%*ld\n", symbol_string(entry->symbol), space_width, ' ', count_width, entry->count);
  }
  token_count_list_deinit(&count_list);
}

static void print_identifier_counts(Map *counts)
{
  unsigned long i;
  int           name_width, count_width;
  Vector        count_list;

  token_count_list_init(&count_list, counts);
  get_display_width(&count_list, &name_width, &count_width);
  for (i = 0; i < vector_size(&count_list); ++i) {
    TokenCountEntry *entry       = vector_data(&count_list)[i];
    int              space_width = name_width - (int) symbol_size(entry->symbol) + 2;
    printf("\"NAME:%s\"%*c%*ld\n", symbol_string(entry->symbol), space_width, ' ', count_width, entry->count);
  }
  token_count_list_deinit(&count_list);
}

void task1(int argc, const char **argv)
{
  Module module;
  Map    token_counts;
  Map    identifier_counts;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  module_init(&module, argv[1]);
  count_token(module_source(&module), module_source_size(&module), &token_counts, &identifier_counts);

  print_token_counts(&token_counts);
  print_identifier_counts(&identifier_counts);

  map_deinit(&token_counts);
  map_deinit(&identifier_counts);
  module_deinit(&module);
}
