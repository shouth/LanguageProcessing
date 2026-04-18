/*
 * 1_tc.c -- task1: lexical analysis
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "ds.h"
#include "syn.h"

#define TOK_BEGIN SYN_IDENT
#define TOK_END SYN_SEMI

hash_t str_hash(void const *x)
{
  char const *str = *(char const **) x;
  size_t len = strlen(str);
  hash_t hash;
  hash_init(&hash);
  hash_add(&hash, str, len);
  return hash;
}

int str_eq(void const *a, void const *b)
{
  return strcmp(*(char const **) a, *(char const **) b) == 0;
}

int main(int argc, char const *argv[])
{
  size_t i;
  struct ht_entry e;
  struct token token;

  char const *text = NULL;
  size_t off = 0;
  size_t len = 0;

  size_t counts[TOK_END - TOK_BEGIN + 1] = { 0 };
  hm(char const *, size_t) idents;

  int status = EXIT_FAILURE;

  ht_init(&idents, str_hash, str_eq);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    goto cleanup;
  }

  if (!(text = load(argv[1], &len))) {
    fprintf(stderr, "error: failed to load file\n");
    goto cleanup;
  }

  while (lex(text + off, len, &token)) {
    if (token.kind >= TOK_BEGIN && token.kind <= TOK_END) {
      ++counts[token.kind - TOK_BEGIN];
      if (token.kind == SYN_IDENT) {
        struct ht_entry e;
        char *lexeme = malloc(token.len + 1);
        memcpy(lexeme, text + off, token.len);
        lexeme[token.len] = '\0';

        if (ht_entry(&idents, &lexeme, &e)) {
          free(lexeme);
        } else {
          ht_occupy(&idents, &e, &lexeme);
        }
        ++ht_at(&idents, &e)->value;
      } else if (token.kind == SYN_NUMBER_LIT) {
        if (strtoul(text + off, NULL, 10) > 32768) {
          fprintf(stderr, "error: number literal is larger than 32768\n");
          goto cleanup;
        }
      } else if (token.kind == SYN_STRING_LIT) {
        if (token.nongraphic) {
          fprintf(stderr, "error: string literal contains nongraphic characters\n");
          goto cleanup;
        } else if (token.nonclosed) {
          fprintf(stderr, "error: string literal is not closed\n");
          goto cleanup;
        }
      } else if (token.kind == SYN_BRACKET_COMMENT || token.kind == SYN_SLASH_STAR_COMMENT) {
        if (token.nonclosed) {
          fprintf(stderr, "error: comment is not closed\n");
          goto cleanup;
        }
      }
    }
    off += token.len;
    len -= token.len;
  }

  for (i = 0; i <= TOK_END - TOK_BEGIN; ++i) {
    if (counts[i]) {
      enum syn_kind kind = TOK_BEGIN + i;
      char const *name = syn_kind_to_lexeme(kind);
      if (kind == SYN_IDENT) {
        name = "NAME";
      } else if (kind == SYN_NUMBER_LIT) {
        name = "NUMBER";
      } else if (kind == SYN_STRING_LIT) {
        name = "STRING";
      }
      printf("\"%s\"\t%lu\n", name, counts[i]);

      if (kind == SYN_IDENT) {
        for (ht_entry(&idents, NULL, &e); ht_next(&idents, &e);) {
          printf("\t\"Identifier\" \"%s\"\t%lu\n", ht_at(&idents, &e)->key, ht_at(&idents, &e)->value);
        }
      }
    }
  }
  status = EXIT_SUCCESS;

cleanup:
  free((void *) text);
  for (ht_entry(&idents, NULL, &e); ht_next(&idents, &e);) {
    free((void *) ht_at(&idents, &e)->key);
  }
  ht_deinit(&idents);

  return status;
}
