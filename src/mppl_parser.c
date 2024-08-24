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
#include "mppl_compiler.h"
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
  int               alive;
  unsigned long     breakable;
};

static void diagnostics(Parser *p, Diag *diagnostics)
{
  array_push(p->diagnostics, &diagnostics);
}

static void null(Parser *p)
{
  raw_syntax_builder_empty(p->builder);
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
      diagnostics(p, diag_too_big_number_error(p->offset, p->span));
    }
    break;
  }
  case MPPL_SYNTAX_STRING_LIT: {
    unsigned long i;
    if (result.is_unterminated) {
      diagnostics(p, diag_unterminated_string_error(p->offset, p->span));
    }
    if (result.has_nongraphic) {
      for (i = 0; i < p->span; ++i) {
        if (!is_graphic(p->text[p->offset + i])) {
          diagnostics(p, diag_nongraphic_char_error(p->offset + i));
        }
      }
    }
    break;
  }
  case MPPL_SYNTAX_BRACES_COMMENT_TRIVIA: {
    if (result.is_unterminated) {
      diagnostics(p, diag_unterminated_comment_error(p->offset, p->span));
    }
    break;
  }
  case MPPL_SYNTAX_C_COMMENT_TRIVIA: {
    if (result.is_unterminated) {
      diagnostics(p, diag_unterminated_comment_error(p->offset, p->span));
    }
    break;
  }
  default:
    /* Do nothing */
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

    if (p->kind == MPPL_SYNTAX_EOF_TRIVIA) {
      break;
    }
  }
  raw_syntax_builder_trivia(p->builder, p->text + trivia_offset, array_data(trivia_pieces), array_count(trivia_pieces));
  array_free(trivia_pieces);
}

static void bump(Parser *p)
{
  raw_syntax_builder_token(p->builder, p->kind, p->text + p->offset, p->span);
  bitset_clear(&p->expected);
  next_nontrivia(p);
}

static int check_any(Parser *p, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!p->alive) {
    return 0;
  } else {
    unsigned long i;
    for (i = 0; i < count; ++i) {
      bitset_set(&p->expected, kinds[i]);
    }
    for (i = 0; i < count; ++i) {
      if (p->kind == kinds[i]) {
        return 1;
      }
    }
    return 0;
  }
}

static int check(Parser *p, MpplSyntaxKind kind)
{
  return check_any(p, &kind, 1);
}

static int eat_any(Parser *p, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!p->alive) {
    return 0;
  } else if (check_any(p, kinds, count)) {
    bump(p);
    return 1;
  } else {
    return 0;
  }
}

static int eat(Parser *p, MpplSyntaxKind kind)
{
  return eat_any(p, &kind, 1);
}

static void error_unexpected(Parser *p)
{
  if (p->kind != MPPL_SYNTAX_ERROR) {
    diagnostics(p, diag_unexpected_token_error(p->offset, p->span, &p->expected));
    p->alive = 0;
  }
  bump(p);
}

static int expect_any(Parser *p, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!p->alive) {
    null(p);
    return 0;
  } else if (eat_any(p, kinds, count)) {
    return 1;
  } else {
    error_unexpected(p);
    return 0;
  }
}

static int expect(Parser *p, MpplSyntaxKind kind)
{
  return expect_any(p, &kind, 1);
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
  if (!eat(p, MPPL_SYNTAX_SEMI_TOKEN) && p->alive) {
    diagnostics(p, diag_missing_semicolon_error(p->offset));
    p->alive = 0;
    bump(p);
  }
}

static const MpplSyntaxKind FIRST_STD_TYPE[] = { MPPL_SYNTAX_INTEGER_KW, MPPL_SYNTAX_BOOLEAN_KW, MPPL_SYNTAX_CHAR_KW };

static void parse_std_type(Parser *p)
{
  expect_any(p, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind));
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
  if (check_any(p, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind))) {
    parse_std_type(p);
  } else {
    parse_array_type(p);
  }
}

static void parse_factor(Parser *p);
static void parse_expr(Parser *p);

static void parse_var(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (eat(p, MPPL_SYNTAX_LBRACKET_TOKEN)) {
    parse_expr(p);
    expect(p, MPPL_SYNTAX_RBRACKET_TOKEN);
    close(p, MPPL_SYNTAX_INDEXED_VAR, checkpoint);
  } else {
    close(p, MPPL_SYNTAX_ENTIRE_VAR, checkpoint);
  }
}

static void parse_paren_expr(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_PAREN_EXPR, checkpoint);
}

static void parse_not_expr(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_NOT_KW);
  parse_factor(p);
  close(p, MPPL_SYNTAX_NOT_EXPR, checkpoint);
}

static void parse_cast_expr(Parser *p)
{
  Checkpoint checkpoint = open(p);
  parse_std_type(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_CAST_EXPR, checkpoint);
}

static const MpplSyntaxKind FIRST_CONST[] = { MPPL_SYNTAX_NUMBER_LIT, MPPL_SYNTAX_TRUE_KW, MPPL_SYNTAX_FALSE_KW, MPPL_SYNTAX_STRING_LIT };

static void parse_factor(Parser *p)
{
  if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_var(p);
  } else if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_paren_expr(p);
  } else if (check(p, MPPL_SYNTAX_NOT_KW)) {
    parse_not_expr(p);
  } else if (check_any(p, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind))) {
    parse_cast_expr(p);
  } else {
    expect_any(p, FIRST_CONST, sizeof(FIRST_CONST) / sizeof(MpplSyntaxKind));
  }
}

static const MpplSyntaxKind FIRST_MULTI_OP[] = { MPPL_SYNTAX_STAR_TOKEN, MPPL_SYNTAX_DIV_KW, MPPL_SYNTAX_AND_KW };

static void parse_term(Parser *p)
{
  Checkpoint checkpoint = open(p);
  parse_factor(p);
  while (eat_any(p, FIRST_MULTI_OP, sizeof(FIRST_MULTI_OP) / sizeof(MpplSyntaxKind))) {
    parse_factor(p);
    close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
  }
}

static const MpplSyntaxKind FIRST_ADD_OP[] = { MPPL_SYNTAX_PLUS_TOKEN, MPPL_SYNTAX_MINUS_TOKEN, MPPL_SYNTAX_OR_KW };

static void parse_simple_expr(Parser *p)
{
  Checkpoint checkpoint = open(p);
  if (check_any(p, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(MpplSyntaxKind))) {
    null(p);
  } else {
    parse_term(p);
  }
  while (eat_any(p, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(MpplSyntaxKind))) {
    parse_term(p);
    close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
  }
}

static const MpplSyntaxKind FIRST_RELAT_OP[] = {
  MPPL_SYNTAX_EQUAL_TOKEN,
  MPPL_SYNTAX_NOTEQ_TOKEN,
  MPPL_SYNTAX_LESS_TOKEN,
  MPPL_SYNTAX_LESSEQ_TOKEN,
  MPPL_SYNTAX_GREATER_TOKEN,
  MPPL_SYNTAX_GREATEREQ_TOKEN,
};

static void parse_expr(Parser *p)
{
  Checkpoint checkpoint = open(p);
  parse_simple_expr(p);
  while (eat_any(p, FIRST_RELAT_OP, sizeof(FIRST_RELAT_OP) / sizeof(MpplSyntaxKind))) {
    parse_simple_expr(p);
    close(p, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
  }
}

static void parse_stmt(Parser *p);

static void parse_assign_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  parse_var(p);
  expect(p, MPPL_SYNTAX_ASSIGN_TOKEN);
  parse_expr(p);
  close(p, MPPL_SYNTAX_ASSIGN_STMT, checkpoint);
}

static void parse_if_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_IF_KW);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_THEN_KW);
  parse_stmt(p);
  if (eat(p, MPPL_SYNTAX_ELSE_KW)) {
    parse_stmt(p);
  } else {
    null(p);
    null(p);
  }
  close(p, MPPL_SYNTAX_IF_STMT, checkpoint);
}

static void parse_while_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  ++p->breakable;
  expect(p, MPPL_SYNTAX_WHILE_KW);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_DO_KW);
  parse_stmt(p);
  --p->breakable;
  close(p, MPPL_SYNTAX_WHILE_STMT, checkpoint);
}

static void parse_break_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  if (check(p, MPPL_SYNTAX_BREAK_KW)) {
    if (!p->breakable && p->alive) {
      diagnostics(p, diag_break_outside_loop_error(p->offset, p->span));
    }
    bump(p);
  } else {
    error_unexpected(p);
  }
  close(p, MPPL_SYNTAX_BREAK_STMT, checkpoint);
}

static void parse_act_param_list(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_expr(p);
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_ACT_PARAM_LIST, checkpoint);
}

static void parse_call_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_CALL_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_act_param_list(p);
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

static void parse_input_list(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_var(p);
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_INPUT_LIST, checkpoint);
}

static const MpplSyntaxKind FIRST_INPUT_STMT[] = { MPPL_SYNTAX_READ_KW, MPPL_SYNTAX_READLN_KW };

static void parse_input_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect_any(p, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(MpplSyntaxKind));
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(p);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_INPUT_STMT, checkpoint);
}

static void parse_output_value(Parser *p)
{
  Checkpoint checkpoint = open(p);
  parse_expr(p);
  if (eat(p, MPPL_SYNTAX_COLON_TOKEN)) {
    expect(p, MPPL_SYNTAX_NUMBER_LIT);
  } else {
    null(p);
    null(p);
  }
  close(p, MPPL_SYNTAX_OUTPUT_VALUE, checkpoint);
}

static void parse_output_list(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_output_value(p);
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_OUTPUT_LIST, checkpoint);
}

static const MpplSyntaxKind FIRST_OUTPUT_STMT[] = { MPPL_SYNTAX_WRITE_KW, MPPL_SYNTAX_WRITELN_KW };

static void parse_output_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect_any(p, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(MpplSyntaxKind));
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(p);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_OUTPUT_STMT, checkpoint);
}

static void parse_comp_stmt(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_BEGIN_KW);
  do {
    parse_stmt(p);
  } while (eat(p, MPPL_SYNTAX_SEMI_TOKEN));
  expect(p, MPPL_SYNTAX_END_KW);
  close(p, MPPL_SYNTAX_COMP_STMT, checkpoint);
}

static void parse_stmt(Parser *p)
{
  if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(p);
  } else if (check(p, MPPL_SYNTAX_IF_KW)) {
    parse_if_stmt(p);
  } else if (check(p, MPPL_SYNTAX_WHILE_KW)) {
    parse_while_stmt(p);
  } else if (check(p, MPPL_SYNTAX_BREAK_KW)) {
    parse_break_stmt(p);
  } else if (check(p, MPPL_SYNTAX_CALL_KW)) {
    parse_call_stmt(p);
  } else if (check(p, MPPL_SYNTAX_RETURN_KW)) {
    parse_return_stmt(p);
  } else if (check_any(p, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(MpplSyntaxKind))) {
    parse_input_stmt(p);
  } else if (check_any(p, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(MpplSyntaxKind))) {
    parse_output_stmt(p);
  } else if (check(p, MPPL_SYNTAX_BEGIN_KW)) {
    parse_comp_stmt(p);
  } else {
    null(p);
  }
}

static void parse_var_decl(Parser *p)
{
  Checkpoint checkpoint = open(p);
  do {
    expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_VAR_DECL, checkpoint);
}

static void parse_var_decl_part(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_VAR_KW);
  do {
    parse_var_decl(p);
    expect_semi(p);
  } while (check(p, MPPL_SYNTAX_IDENT_TOKEN));
  close(p, MPPL_SYNTAX_VAR_DECL_PART, checkpoint);
}

static void parse_fml_param_sec(Parser *p)
{
  Checkpoint checkpoint = open(p);
  do {
    expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  } while (eat(p, MPPL_SYNTAX_COMMA_TOKEN));
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_FML_PARAM_SEC, checkpoint);
}

static void parse_fml_param_list(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_fml_param_sec(p);
  } while (eat(p, MPPL_SYNTAX_SEMI_TOKEN));
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_FML_PARAM_LIST, checkpoint);
}

static void parse_proc_decl(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_PROCEDURE_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_fml_param_list(p);
  } else {
    null(p);
  }
  expect_semi(p);
  if (check(p, MPPL_SYNTAX_VAR_KW)) {
    parse_var_decl_part(p);
  } else {
    null(p);
  }
  parse_comp_stmt(p);
  expect_semi(p);
  close(p, MPPL_SYNTAX_PROC_DECL, checkpoint);
}

static void parse_program(Parser *p)
{
  Checkpoint checkpoint = open(p);
  expect(p, MPPL_SYNTAX_PROGRAM_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  expect_semi(p);

  while (1) {
    if (check(p, MPPL_SYNTAX_VAR_KW)) {
      parse_var_decl_part(p);
    } else if (check(p, MPPL_SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl(p);
    } else {
      break;
    }
  }
  parse_comp_stmt(p);
  expect(p, MPPL_SYNTAX_DOT_TOKEN);
  close(p, MPPL_SYNTAX_PROGRAM, checkpoint);
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
  p.alive       = 1;
  p.breakable   = 0;

  next_nontrivia(&p); /* initialize `p.span` and `p.kind` */
  parse_program(&p);

  result.root       = raw_syntax_builder_finish(p.builder);
  result.diag_count = array_count(p.diagnostics);
  result.diags      = array_steal(p.diagnostics);

  return result;
}
