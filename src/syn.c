/*
 * syn.c -- syntax
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syn.h"

char const *syn_kind_to_lexeme(enum syn_kind kind)
{
  switch (kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return LEXEME;

  DEF_SYN(TOK)

#undef TOK

  default: return NULL;
  }
}

char const *syn_kind_to_string(enum syn_kind kind)
{
  switch (kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return #KIND;

  DEF_SYN(TOK)

#undef TOK

  default: return NULL;
  }
}
