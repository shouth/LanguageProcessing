/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>

#include "mppl_passes.h"
#include "mppl_syntax.h"
#include "utility.h"

typedef struct Lexer Lexer;

struct Lexer {
  const char   *text;
  unsigned long length;
  unsigned long span;
};

static void bump(Lexer *l)
{
  if (l->span < l->length) {
    ++l->span;
  }
}

static int first(Lexer *l)
{
  return l->span < l->length ? l->text[l->span] : EOF;
}

static int eat(Lexer *l, int c)
{
  int result = first(l) == c;
  if (result) {
    bump(l);
  }
  return result;
}

static int eat_if(Lexer *l, int (*pred)(int))
{
  int result = pred(first(l));
  if (result) {
    bump(l);
  }
  return result;
}

static MpplLexResult lex(Lexer *l, MpplSyntaxKind kind)
{
  MpplLexResult result;
  result.kind            = kind;
  result.span            = l->span;
  result.is_unterminated = 0;
  result.has_nongraphic  = 0;

  l->text += l->span;
  l->length -= l->span;
  l->span = 0;

  return result;
}

static MpplLexResult lex_identifier_or_keyword(Lexer *l)
{
  MpplSyntaxKind kind;
  while (eat_if(l, &is_alphabet) || eat_if(l, &is_number)) { }
  kind = mppl_syntax_kind_from_keyword(l->text, l->span);
  return lex(l, kind != MPPL_SYNTAX_ERROR ? kind : MPPL_SYNTAX_IDENT_TOKEN);
}

static MpplLexResult lex_integer(Lexer *l)
{
  while (eat_if(l, &is_number)) { }
  return lex(l, MPPL_SYNTAX_NUMBER_LIT);
}

static MpplLexResult lex_string(Lexer *l)
{
  int has_nongraphic = 0;
  while (1) {
    if (eat(l, '\'')) {
      if (!eat(l, '\'')) {
        MpplLexResult result = lex(l, MPPL_SYNTAX_STRING_LIT);
        if (has_nongraphic) {
          result.has_nongraphic = 1;
        }
        return result;
      }
    } else if (first(l) == '\r' || first(l) == '\n' || first(l) == EOF) {
      MpplLexResult result   = lex(l, MPPL_SYNTAX_STRING_LIT);
      result.is_unterminated = 1;
      return result;
    } else if (!eat_if(l, &is_graphic)) {
      has_nongraphic = 1;
      bump(l);
    }
  }
}

static MpplLexResult lex_whitespace(Lexer *l)
{
  while (eat_if(l, &is_space)) { }
  return lex(l, MPPL_SYNTAX_SPACE_TRIVIA);
}

static MpplLexResult lex_braces_comment(Lexer *l)
{
  while (1) {
    if (eat(l, '}')) {
      return lex(l, MPPL_SYNTAX_BRACES_COMMENT_TRIVIA);
    } else if (first(l) == EOF) {
      MpplLexResult result   = lex(l, MPPL_SYNTAX_BRACES_COMMENT_TRIVIA);
      result.is_unterminated = 1;
      return result;
    } else {
      bump(l);
    }
  }
}

static MpplLexResult lex_c_comment(Lexer *l)
{
  while (1) {
    if (eat(l, '*') && eat(l, '/')) {
      return lex(l, MPPL_SYNTAX_C_COMMENT_TRIVIA);
    } else if (first(l) == EOF) {
      MpplLexResult result   = lex(l, MPPL_SYNTAX_C_COMMENT_TRIVIA);
      result.is_unterminated = 1;
      return result;
    } else {
      bump(l);
    }
  }
}

static MpplLexResult lex_token(Lexer *l)
{
  if (first(l) == EOF) {
    return lex(l, MPPL_SYNTAX_END_OF_FILE);
  } else if (eat_if(l, &is_alphabet)) {
    return lex_identifier_or_keyword(l);
  } else if (eat_if(l, &is_number)) {
    return lex_integer(l);
  } else if (eat(l, '\'')) {
    return lex_string(l);
  } else if (eat_if(l, &is_space)) {
    return lex_whitespace(l);
  } else if (eat(l, '{')) {
    return lex_braces_comment(l);
  } else if (eat(l, '/')) {
    if (eat(l, '*')) {
      return lex_c_comment(l);
    } else {
      return lex(l, MPPL_SYNTAX_ERROR);
    }
  } else if (eat(l, '+')) {
    return lex(l, MPPL_SYNTAX_PLUS_TOKEN);
  } else if (eat(l, '-')) {
    return lex(l, MPPL_SYNTAX_MINUS_TOKEN);
  } else if (eat(l, '*')) {
    return lex(l, MPPL_SYNTAX_STAR_TOKEN);
  } else if (eat(l, '=')) {
    return lex(l, MPPL_SYNTAX_EQUAL_TOKEN);
  } else if (eat(l, '<')) {
    if (eat(l, '>')) {
      return lex(l, MPPL_SYNTAX_NOTEQ_TOKEN);
    } else if (eat(l, '=')) {
      return lex(l, MPPL_SYNTAX_LESSEQ_TOKEN);
    } else {
      return lex(l, MPPL_SYNTAX_LESS_TOKEN);
    }
  } else if (eat(l, '>')) {
    if (eat(l, '=')) {
      return lex(l, MPPL_SYNTAX_GREATEREQ_TOKEN);
    } else {
      return lex(l, MPPL_SYNTAX_GREATER_TOKEN);
    }
  } else if (eat(l, '(')) {
    return lex(l, MPPL_SYNTAX_LPAREN_TOKEN);
  } else if (eat(l, ')')) {
    return lex(l, MPPL_SYNTAX_RPAREN_TOKEN);
  } else if (eat(l, '[')) {
    return lex(l, MPPL_SYNTAX_LBRACKET_TOKEN);
  } else if (eat(l, ']')) {
    return lex(l, MPPL_SYNTAX_RBRACKET_TOKEN);
  } else if (eat(l, ':')) {
    if (eat(l, '=')) {
      return lex(l, MPPL_SYNTAX_ASSIGN_TOKEN);
    } else {
      return lex(l, MPPL_SYNTAX_COLON_TOKEN);
    }
  } else if (eat(l, '.')) {
    return lex(l, MPPL_SYNTAX_DOT_TOKEN);
  } else if (eat(l, ',')) {
    return lex(l, MPPL_SYNTAX_COMMA_TOKEN);
  } else if (eat(l, ';')) {
    return lex(l, MPPL_SYNTAX_SEMI_TOKEN);
  } else {
    bump(l);
    return lex(l, MPPL_SYNTAX_ERROR);
  }
}

MpplLexResult mppl_lex(const char *text, unsigned long length)
{
  Lexer l;
  l.text   = text;
  l.length = length;
  l.span   = 0;
  return lex_token(&l);
}
