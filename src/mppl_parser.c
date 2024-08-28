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

#include <stddef.h>

#include "array.h"
#include "diagnostics.h"
#include "mppl_passes.h"
#include "mppl_syntax.h"
#include "string.h"
#include "syntax_tree.h"
#include "utility.h"

typedef RawSyntaxCheckpoint Checkpoint;
typedef struct Parser       Parser;

struct Parser {
  const char    *text;
  unsigned long  length;
  unsigned long  offset;
  MpplSyntaxKind kind;
  unsigned long  span;

  RawSyntaxBuilder *builder;
  MpplSyntaxKindSet expected;
  Array            *diagnostics;
  unsigned long     breakable;
  int               recovery;
};

static void diag(Parser *p, Diag *diagnostics)
{
  array_push(p->diagnostics, &diagnostics);
}

static void next_token(Parser *p)
{
  unsigned long next_offset = p->offset + p->span;

  MpplLexResult result = mppl_lex(p->text + next_offset, p->length - next_offset);
  p->offset            = next_offset;
  p->kind              = result.kind;
  p->span              = result.span;

  switch (p->kind) {
  case MPPL_SYNTAX_NUMBER_LIT: {
    char buffer[8];
    strncpy(buffer, p->text + p->offset, p->span);
    buffer[p->span] = '\0';

    if (strtol(buffer, NULL, 10) > 32767) {
      diag(p, diag_too_big_number_error(p->offset, p->span));
    }
    break;
  }
  case MPPL_SYNTAX_STRING_LIT: {
    unsigned long i;
    if (result.is_unterminated) {
      diag(p, diag_unterminated_string_error(p->offset, p->span));
    }
    if (result.has_nongraphic) {
      for (i = 0; i < p->span; ++i) {
        if (!is_graphic(p->text[p->offset + i])) {
          diag(p, diag_nongraphic_char_error(p->offset + i, p->text[p->offset + i]));
        }
      }
    }
    break;
  }
  case MPPL_SYNTAX_BRACES_COMMENT_TRIVIA: {
    if (result.is_unterminated) {
      diag(p, diag_unterminated_comment_error(p->offset, p->span));
    }
    break;
  }
  case MPPL_SYNTAX_C_COMMENT_TRIVIA: {
    if (result.is_unterminated) {
      diag(p, diag_unterminated_comment_error(p->offset, p->span));
    }
    break;
  }
  default:
    /* do nothing */
    break;
  }
}

static void next_nontrivia(Parser *p)
{
  unsigned long trivia_offset = p->offset + p->span;
  Array        *trivia_pieces = array_new(sizeof(RawSyntaxTriviaPiece));
  while (next_token(p), mppl_syntax_kind_is_trivia(p->kind)) {
    RawSyntaxTriviaPiece piece;
    piece.kind             = p->kind;
    piece.span.text_length = p->span;
    array_push(trivia_pieces, &piece);
  }
  raw_syntax_builder_trivia(p->builder, p->text + trivia_offset, array_data(trivia_pieces), array_count(trivia_pieces));
  array_free(trivia_pieces);
}

static int is_eof(Parser *p)
{
  return p->kind == MPPL_SYNTAX_END_OF_FILE;
}

static void null(Parser *p)
{
  raw_syntax_builder_empty(p->builder);
}

static void bump(Parser *p)
{
  raw_syntax_builder_token(p->builder, p->kind, p->text + p->offset, p->span);
  bitset_clear(&p->expected);
  next_nontrivia(p);
}

static int check_any(Parser *p, const MpplSyntaxKindSet *kinds)
{
  bitset_insert(&p->expected, kinds);
  return bitset_get(kinds, p->kind);
}

static int check(Parser *p, MpplSyntaxKind kind)
{
  MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind);
  return check_any(p, &kinds);
}

static int eat_any(Parser *p, const MpplSyntaxKindSet *kinds)
{
  if (check_any(p, kinds)) {
    bump(p);
    return 1;
  } else {
    return 0;
  }
}

static int eat(Parser *p, MpplSyntaxKind kind)
{
  MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind);
  return eat_any(p, &kinds);
}

static void error_unexpected(Parser *p)
{
  if (p->kind == MPPL_SYNTAX_ERROR) {
    diag(p, diag_stray_char_error(p->offset, p->text[p->offset], p->expected));
  } else {
    char *found = strndup(p->text + p->offset, p->span);
    diag(p, diag_unexpected_token_error(p->offset, p->span, found, p->expected));
  }
}

static int expect_any(Parser *p, const MpplSyntaxKindSet *kinds)
{
  if (eat_any(p, kinds)) {
    return 1;
  } else {
    if (!p->recovery) {
      error_unexpected(p);
    }
    p->recovery = 1;
    null(p);
    return 0;
  }
}

static int expect(Parser *p, MpplSyntaxKind kind)
{
  MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind);
  return expect_any(p, &kinds);
}

static void recover(Parser *p, const MpplSyntaxKindSet *kinds)
{
  while (!check_any(p, kinds) && !is_eof(p)) {
    p->kind = MPPL_SYNTAX_ERROR;
    bump(p);
  }
  p->recovery = is_eof(p);
}

static Checkpoint open(Parser *p)
{
  return raw_syntax_builder_open(p->builder);
}

static void close(Parser *p, MpplSyntaxKind kind, Checkpoint checkpoint)
{
  raw_syntax_builder_close(p->builder, kind, checkpoint);
}

static void expect_semi(Parser *p)
{
  if (!eat(p, MPPL_SYNTAX_SEMI_TOKEN) && !p->recovery) {
    diag(p, diag_missing_semicolon_error(p->offset));
    bump(p);
  }
}

static const MpplSyntaxKindSet *first_std_type(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_INTEGER_KW);
  bitset_set(&kinds, MPPL_SYNTAX_BOOLEAN_KW);
  bitset_set(&kinds, MPPL_SYNTAX_CHAR_KW);
  return &kinds;
}

static void parse_std_type(Parser *p)
{
  expect_any(p, first_std_type());
}

static void parse_array_type(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_ARRAY_KW);
  expect(p, MPPL_SYNTAX_LBRACKET_TOKEN);
  expect(p, MPPL_SYNTAX_NUMBER_LIT);
  expect(p, MPPL_SYNTAX_RBRACKET_TOKEN);
  expect(p, MPPL_SYNTAX_OF_KW);
  parse_std_type(p);
  close(p, MPPL_SYNTAX_ARRAY_TYPE, checkpoint);
}

static void parse_type(Parser *p)
{
  if (check_any(p, first_std_type())) {
    parse_std_type(p);
  } else {
    parse_array_type(p);
  }
}

static void parse_expr(Parser *p, const MpplSyntaxKindSet *recovery);

static const MpplSyntaxKindSet *first_const(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_NUMBER_LIT);
  bitset_set(&kinds, MPPL_SYNTAX_TRUE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_FALSE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_STRING_LIT);
  return &kinds;
}

static const MpplSyntaxKindSet *first_multi_op(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_STAR_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_DIV_KW);
  bitset_set(&kinds, MPPL_SYNTAX_AND_KW);
  return &kinds;
}

static const MpplSyntaxKindSet *first_add_op(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_PLUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_MINUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_OR_KW);
  return &kinds;
}

static const MpplSyntaxKindSet *first_relat_op(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_EQUAL_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_NOTEQ_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_LESS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_LESSEQ_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_GREATER_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_GREATEREQ_TOKEN);
  return &kinds;
}

static const MpplSyntaxKindSet *first_expr(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_PLUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_MINUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_NOT_KW);
  bitset_set(&kinds, MPPL_SYNTAX_LPAREN_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_IDENT_TOKEN);
  bitset_insert(&kinds, first_const());
  bitset_insert(&kinds, first_const());
  return &kinds;
}

static void parse_expr_with_power(Parser *p, int power, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);

  if (power < 20 && (eat(p, MPPL_SYNTAX_PLUS_TOKEN) || eat(p, MPPL_SYNTAX_MINUS_TOKEN))) {
    /* ('+' | '-') expr */
    parse_expr_with_power(p, 21, recovery);
    close(p, MPPL_SYNTAX_UNARY_EXPR, checkpoint);
  } else if (eat(p, MPPL_SYNTAX_NOT_KW)) {
    /* 'not' expr */
    parse_expr_with_power(p, 100, recovery);
    close(p, MPPL_SYNTAX_UNARY_EXPR, checkpoint);
  } else if (eat(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    /* '(' expr ')' */
    parse_expr(p, &kinds);
    expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
    close(p, MPPL_SYNTAX_PAREN_EXPR, checkpoint);
  } else if (eat_any(p, first_std_type())) {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    /* type '(' expr ')' */
    expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
    parse_expr(p, &kinds);
    expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
    close(p, MPPL_SYNTAX_CAST_EXPR, checkpoint);
  } else if (eat_any(p, first_const())) {
    /* number | true | false | string */
  } else if (eat(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    if (eat(p, MPPL_SYNTAX_LBRACKET_TOKEN)) {
      MpplSyntaxKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_RBRACKET_TOKEN);

      /* identifier '[' expr ']' */
      parse_expr(p, &kinds);
      expect(p, MPPL_SYNTAX_RBRACKET_TOKEN);
      close(p, MPPL_SYNTAX_INDEXED_VAR, checkpoint);
    } else {
      /* identifier */
      close(p, MPPL_SYNTAX_ENTIRE_VAR, checkpoint);
    }
  } else {
    diag(p, diag_expected_expression_error(p->offset, p->span));
    return;
  }

  while (1) {
    if (power < 10 && eat_any(p, first_relat_op())) {
      /* expr ('=' | '<>' | '<' | '<=' | '>' | '>=) expr */
      parse_expr_with_power(p, 11, recovery);
      close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
    } else if (power < 20 && eat_any(p, first_add_op())) {
      /* expr ('+' | '-' | 'or') expr */
      parse_expr_with_power(p, 21, recovery);
      close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
    } else if (power < 30 && eat_any(p, first_multi_op())) {
      /* expr ('*' | 'div' | 'and') expr */
      parse_expr_with_power(p, 31, recovery);
      close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
    } else {
      break;
    }
  }
}

static void parse_expr(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  parse_expr_with_power(p, 0, recovery);

  /*
  if (!bitset_get(&recovery, p->kind)) {
    error_unexpected(p);
    while (!bitset_get(&recovery, p->kind) && !is_eof(p)) {
      MpplSyntaxKindSet kinds = first_expr();
      bitset_insert(&kinds, &recovery);
      recover(p, kinds);

      if (!bitset_get(&recovery, p->kind) && check_any(p, first_expr())) {
        parse_expr_with_power(p, 0, recovery);
      }
    }
    close(p, MPPL_SYNTAX_BOGUS, checkpoint);
  }
*/
}

static void parse_stmt(Parser *p, const MpplSyntaxKindSet *recovery);

static const MpplSyntaxKindSet *first_input_stmt(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_READ_KW);
  bitset_set(&kinds, MPPL_SYNTAX_READLN_KW);
  return &kinds;
}

static const MpplSyntaxKindSet *first_output_stmt(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_WRITE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_WRITELN_KW);
  return &kinds;
}

static const MpplSyntaxKindSet *first_stmt(void)
{
  static MpplSyntaxKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_IDENT_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_IF_KW);
  bitset_set(&kinds, MPPL_SYNTAX_WHILE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_BREAK_KW);
  bitset_set(&kinds, MPPL_SYNTAX_CALL_KW);
  bitset_set(&kinds, MPPL_SYNTAX_RETURN_KW);
  bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);
  bitset_insert(&kinds, first_input_stmt());
  bitset_insert(&kinds, first_output_stmt());
  return &kinds;
}

static void parse_assign_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_ASSIGN_TOKEN);
    parse_expr(p, &kinds);
  }
  expect(p, MPPL_SYNTAX_ASSIGN_TOKEN);
  parse_expr(p, recovery);
  close(p, MPPL_SYNTAX_ASSIGN_STMT, checkpoint);
}

static void parse_if_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_IF_KW);
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_THEN_KW);
    bitset_set(&kinds, MPPL_SYNTAX_ELSE_KW);
    parse_expr(p, &kinds);
  }
  expect(p, MPPL_SYNTAX_THEN_KW);
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_ELSE_KW);
    parse_stmt(p, &kinds);
  }
  if (eat(p, MPPL_SYNTAX_ELSE_KW)) {
    parse_stmt(p, recovery);
  } else {
    null(p);
    null(p);
  }
  close(p, MPPL_SYNTAX_IF_STMT, checkpoint);
}

static void parse_while_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  ++p->breakable;
  expect(p, MPPL_SYNTAX_WHILE_KW);
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_DO_KW);
    parse_expr(p, &kinds);
  }
  expect(p, MPPL_SYNTAX_DO_KW);
  parse_stmt(p, recovery);
  --p->breakable;
  close(p, MPPL_SYNTAX_WHILE_STMT, checkpoint);
}

static void parse_break_stmt(Parser *p)
{
  Checkpoint    checkpoint = open(p);
  unsigned long offset     = p->offset;
  unsigned long span       = p->span;
  if (expect(p, MPPL_SYNTAX_BREAK_KW) && !p->breakable) {
    diag(p, diag_break_outside_loop_error(offset, span));
  }
  close(p, MPPL_SYNTAX_BREAK_STMT, checkpoint);
}

static void parse_act_param_list(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    if (check_any(p, first_expr())) {
      parse_expr(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_ACT_PARAM_LIST, checkpoint);
}

static void parse_call_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_CALL_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_act_param_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_CALL_STMT, checkpoint);
}

static void parse_return_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_RETURN_KW);
  close(p, MPPL_SYNTAX_RETURN_STMT, checkpoint);
}

static void parse_input_list(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    if (check_any(p, first_expr())) {
      parse_expr(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_INPUT_LIST, checkpoint);
}

static void parse_input_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect_any(p, first_input_stmt());
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_INPUT_STMT, checkpoint);
}

static void parse_output_value(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COLON_TOKEN);
    parse_expr(p, &kinds);
  }
  if (eat(p, MPPL_SYNTAX_COLON_TOKEN)) {
    expect(p, MPPL_SYNTAX_NUMBER_LIT);
    close(p, MPPL_SYNTAX_OUTPUT_VALUE, checkpoint);
  }
}

static void parse_output_list(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    if (check_any(p, first_expr())) {
      parse_output_value(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_OUTPUT_LIST, checkpoint);
}

static void parse_output_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect_any(p, first_output_stmt());
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_OUTPUT_STMT, checkpoint);
}

static void parse_comp_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_BEGIN_KW);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_END_KW);

    if (check_any(p, first_stmt())) {
      parse_stmt(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_SEMI_TOKEN));
  expect(p, MPPL_SYNTAX_END_KW);
  close(p, MPPL_SYNTAX_COMP_STMT, checkpoint);
}

static void parse_stmt(Parser *p, const MpplSyntaxKindSet *recovery)
{
  if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(p, recovery);
  } else if (check(p, MPPL_SYNTAX_IF_KW)) {
    parse_if_stmt(p, recovery);
  } else if (check(p, MPPL_SYNTAX_WHILE_KW)) {
    parse_while_stmt(p, recovery);
  } else if (check(p, MPPL_SYNTAX_BREAK_KW)) {
    parse_break_stmt(p);
  } else if (check(p, MPPL_SYNTAX_CALL_KW)) {
    parse_call_stmt(p, recovery);
  } else if (check(p, MPPL_SYNTAX_RETURN_KW)) {
    parse_return_stmt(p);
  } else if (check_any(p, first_input_stmt())) {
    parse_input_stmt(p, recovery);
  } else if (check_any(p, first_output_stmt())) {
    parse_output_stmt(p, recovery);
  } else if (check(p, MPPL_SYNTAX_BEGIN_KW)) {
    parse_comp_stmt(p, recovery);
  } else {
    null(p);
  }
}

static void parse_var_decl(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_COLON_TOKEN);

    if (expect(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      /* identifier */
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_VAR_DECL, checkpoint);
}

static void parse_var_decl_part(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_VAR_KW);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);

    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_var_decl(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
    expect_semi(p);
  } while (check(p, MPPL_SYNTAX_IDENT_TOKEN));
  close(p, MPPL_SYNTAX_VAR_DECL_PART, checkpoint);
}

static void parse_fml_param_sec(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint fml_param_sec = open(p);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_COLON_TOKEN);

    if (expect(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      /* identifier */
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_FML_PARAM_SEC, fml_param_sec);
}

static void parse_fml_param_list(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint fml_param_list = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_fml_param_sec(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  } while (eat(p, MPPL_SYNTAX_SEMI_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_FML_PARAM_LIST, fml_param_list);
}

static void parse_proc_decl(Parser *p, const MpplSyntaxKindSet *recovery)
{
  Checkpoint proc_decl = open(p);
  expect(p, MPPL_SYNTAX_PROCEDURE_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_VAR_KW);
    bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);
    parse_fml_param_list(p, &kinds);
  } else {
    null(p);
  }
  expect_semi(p);
  if (check(p, MPPL_SYNTAX_VAR_KW)) {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);
    parse_var_decl_part(p, &kinds);
  } else {
    null(p);
  }
  {
    MpplSyntaxKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    parse_comp_stmt(p, &kinds);
  }
  expect_semi(p);
  close(p, MPPL_SYNTAX_PROC_DECL, proc_decl);
}

static void parse_program(Parser *p)
{
  Checkpoint program = open(p);
  expect(p, MPPL_SYNTAX_PROGRAM_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  expect_semi(p);

  while (!check(p, MPPL_SYNTAX_BEGIN_KW) && !is_eof(p)) {
    MpplSyntaxKindSet kinds = bitset_zero();
    bitset_set(&kinds, MPPL_SYNTAX_VAR_KW);
    bitset_set(&kinds, MPPL_SYNTAX_PROCEDURE_KW);
    bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);

    if (check(p, MPPL_SYNTAX_VAR_KW)) {
      parse_var_decl_part(p, &kinds);
    } else if (check(p, MPPL_SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl(p, &kinds);
    } else {
      Checkpoint bogus = open(p);
      recover(p, &kinds);
      close(p, MPPL_SYNTAX_BOGUS, bogus);
    }
  }
  {
    MpplSyntaxKindSet kinds = bitset_zero();
    bitset_set(&kinds, MPPL_SYNTAX_DOT_TOKEN);
    parse_comp_stmt(p, &kinds);
  }
  expect(p, MPPL_SYNTAX_DOT_TOKEN);
  close(p, MPPL_SYNTAX_PROGRAM, program);
}

MpplParseResult mppl_parse(const char *text, unsigned long length)
{
  MpplParseResult result;

  Parser p;
  p.text   = text;
  p.length = length;
  p.offset = 0;
  p.kind   = MPPL_SYNTAX_ERROR; /* dummy */
  p.span   = 0; /* dummy */

  p.builder = raw_syntax_builder_new();
  bitset_clear(&p.expected);
  p.diagnostics = array_new(sizeof(Diag *));
  p.breakable   = 0;
  p.recovery    = 0;

  next_nontrivia(&p); /* initialize `p.span` and `p.kind` */
  parse_program(&p);
  expect(&p, MPPL_SYNTAX_END_OF_FILE);

  result.root       = raw_syntax_builder_finish(p.builder);
  result.diag_count = array_count(p.diagnostics);
  result.diags      = array_steal(p.diagnostics);

  return result;
}
