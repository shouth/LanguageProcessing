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
#include <stdio.h>

#include "array.h"
#include "diagnostics.h"
#include "mppl_compiler.h"
#include "mppl_syntax_kind.h"
#include "source.h"
#include "syntax_tree.h"
#include "utility.h"

typedef struct Parser Parser;

struct Parser {
  const Source  *source;
  LexedToken     token;
  SyntaxBuilder *builder;

  MpplSyntaxKindSet expected;
  Array            *diagnostics;
  int               alive;
  unsigned long     breakable;
};

static void diagnostics(Parser *self, Diag *diagnostics)
{
  array_push(self->diagnostics, &diagnostics);
}

static void null(Parser *self)
{
  syntax_builder_empty(self->builder);
}

static void lex(Parser *self)
{
  while (1) {
    LexStatus status = mpplc_lex(self->source, self->token.offset + self->token.length, &self->token);
    if (status == LEX_OK || status == LEX_EOF) {
      if (mppl_syntax_kind_is_token(self->token.kind)) {
        break;
      } else {
        syntax_builder_trivia(self->builder, self->token.kind, self->source->text + self->token.offset, self->token.length);
      }
    } else {
      switch (status) {
      case LEX_ERROR_STRAY_CHAR: {
        diagnostics(self, diag_stray_char_error(self->token.offset, &self->expected));
        self->alive = 0;
        break;
      }
      case LEX_ERROR_NONGRAPHIC_CHAR: {
        diagnostics(self, diag_nongraphic_char_error(self->token.offset));
        break;
      }
      case LEX_ERROR_UNTERMINATED_STRING: {
        diagnostics(self, diag_unterminated_string_error(self->token.offset, self->token.length));
        self->alive = 0;
        break;
      }
      case LEX_ERROR_UNTERMINATED_COMMENT: {
        diagnostics(self, diag_unterminated_comment_error(self->token.offset, self->token.length));
        self->alive = 0;
        break;
      }
      case LEX_ERROR_TOO_BIG_NUMBER: {
        diagnostics(self, diag_too_big_number_error(self->token.offset, self->token.length));
        break;
      }
      default:
        unreachable();
      }
      break;
    }
  }
}

static void bump(Parser *self)
{
  syntax_builder_token(self->builder, self->token.kind, self->source->text + self->token.offset, self->token.length);
  bitset_clear(&self->expected);
  lex(self);
}

static int check_any(Parser *self, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    return 0;
  } else {
    unsigned long i;
    for (i = 0; i < count; ++i) {
      bitset_set(&self->expected, kinds[i]);
    }
    for (i = 0; i < count; ++i) {
      if (self->token.kind == kinds[i]) {
        return 1;
      }
    }
    return 0;
  }
}

static int check(Parser *self, MpplSyntaxKind kind)
{
  return check_any(self, &kind, 1);
}

static int eat_any(Parser *self, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    return 0;
  } else if (check_any(self, kinds, count)) {
    bump(self);
    return 1;
  } else {
    return 0;
  }
}

static int eat(Parser *self, MpplSyntaxKind kind)
{
  return eat_any(self, &kind, 1);
}

static void error_unexpected(Parser *self)
{
  if (self->token.kind != MPPL_SYNTAX_ERROR) {
    diagnostics(self, diag_unexpected_token_error(self->token.offset, self->token.length, &self->expected));
    self->alive = 0;
  }
  bump(self);
}

static int expect_any(Parser *self, const MpplSyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    null(self);
    return 0;
  } else if (eat_any(self, kinds, count)) {
    return 1;
  } else {
    error_unexpected(self);
    return 0;
  }
}

static int expect(Parser *self, MpplSyntaxKind kind)
{
  return expect_any(self, &kind, 1);
}

static SyntaxCheckpoint open(Parser *self)
{
  return syntax_builder_open(self->builder);
}

static void close(Parser *self, MpplSyntaxKind kind, SyntaxCheckpoint checkpoint)
{
  syntax_builder_close(self->builder, kind, checkpoint);
}

static void expect_semi(Parser *self)
{
  if (!eat(self, MPPL_SYNTAX_SEMI_TOKEN) && self->alive) {
    diagnostics(self, diag_missing_semicolon_error(self->token.offset));
    self->alive = 0;
    bump(self);
  }
}

static const MpplSyntaxKind FIRST_STD_TYPE[] = { MPPL_SYNTAX_INTEGER_KW, MPPL_SYNTAX_BOOLEAN_KW, MPPL_SYNTAX_CHAR_KW };

static void parse_std_type(Parser *self)
{
  expect_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind));
}

static void parse_array_type(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_ARRAY_KW);
  expect(self, MPPL_SYNTAX_LBRACKET_TOKEN);
  expect(self, MPPL_SYNTAX_NUMBER_LIT);
  expect(self, MPPL_SYNTAX_RBRACKET_TOKEN);
  expect(self, MPPL_SYNTAX_OF_KW);
  parse_std_type(self);
  close(self, MPPL_SYNTAX_ARRAY_TYPE, checkpoint);
}

static void parse_type(Parser *self)
{
  if (check_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind))) {
    parse_std_type(self);
  } else {
    parse_array_type(self);
  }
}

static void parse_factor(Parser *self);
static void parse_expr(Parser *self);

static void parse_var(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  if (eat(self, MPPL_SYNTAX_LBRACKET_TOKEN)) {
    parse_expr(self);
    expect(self, MPPL_SYNTAX_RBRACKET_TOKEN);
    close(self, MPPL_SYNTAX_INDEXED_VAR, checkpoint);
  } else {
    close(self, MPPL_SYNTAX_ENTIRE_VAR, checkpoint);
  }
}

static void parse_paren_expr(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  parse_expr(self);
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_PAREN_EXPR, checkpoint);
}

static void parse_not_expr(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_NOT_KW);
  parse_factor(self);
  close(self, MPPL_SYNTAX_NOT_EXPR, checkpoint);
}

static void parse_cast_expr(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  parse_std_type(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  parse_expr(self);
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_CAST_EXPR, checkpoint);
}

static const MpplSyntaxKind FIRST_CONST[] = { MPPL_SYNTAX_NUMBER_LIT, MPPL_SYNTAX_TRUE_KW, MPPL_SYNTAX_FALSE_KW, MPPL_SYNTAX_STRING_LIT };

static void parse_factor(Parser *self)
{
  if (check(self, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_var(self);
  } else if (check(self, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_paren_expr(self);
  } else if (check(self, MPPL_SYNTAX_NOT_KW)) {
    parse_not_expr(self);
  } else if (check_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(MpplSyntaxKind))) {
    parse_cast_expr(self);
  } else {
    expect_any(self, FIRST_CONST, sizeof(FIRST_CONST) / sizeof(MpplSyntaxKind));
  }
}

static const MpplSyntaxKind FIRST_MULTI_OP[] = { MPPL_SYNTAX_STAR_TOKEN, MPPL_SYNTAX_DIV_KW, MPPL_SYNTAX_AND_KW };

static void parse_term(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  parse_factor(self);
  while (eat_any(self, FIRST_MULTI_OP, sizeof(FIRST_MULTI_OP) / sizeof(MpplSyntaxKind))) {
    parse_factor(self);
    close(self, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
  }
}

static const MpplSyntaxKind FIRST_ADD_OP[] = { MPPL_SYNTAX_PLUS_TOKEN, MPPL_SYNTAX_MINUS_TOKEN, MPPL_SYNTAX_OR_KW };

static void parse_simple_expr(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  if (check_any(self, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(MpplSyntaxKind))) {
    null(self);
  } else {
    parse_term(self);
  }
  while (eat_any(self, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(MpplSyntaxKind))) {
    parse_term(self);
    close(self, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
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

static void parse_expr(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  parse_simple_expr(self);
  while (eat_any(self, FIRST_RELAT_OP, sizeof(FIRST_RELAT_OP) / sizeof(MpplSyntaxKind))) {
    parse_simple_expr(self);
    close(self, MPPL_SYNTAX_BINARY_EXPR, checkpoint);
  }
}

static void parse_stmt(Parser *self);

static void parse_assign_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  parse_var(self);
  expect(self, MPPL_SYNTAX_ASSIGN_TOKEN);
  parse_expr(self);
  close(self, MPPL_SYNTAX_ASSIGN_STMT, checkpoint);
}

static void parse_if_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_IF_KW);
  parse_expr(self);
  expect(self, MPPL_SYNTAX_THEN_KW);
  parse_stmt(self);
  if (eat(self, MPPL_SYNTAX_ELSE_KW)) {
    parse_stmt(self);
  } else {
    null(self);
    null(self);
  }
  close(self, MPPL_SYNTAX_IF_STMT, checkpoint);
}

static void parse_while_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  ++self->breakable;
  expect(self, MPPL_SYNTAX_WHILE_KW);
  parse_expr(self);
  expect(self, MPPL_SYNTAX_DO_KW);
  parse_stmt(self);
  --self->breakable;
  close(self, MPPL_SYNTAX_WHILE_STMT, checkpoint);
}

static void parse_break_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  if (check(self, MPPL_SYNTAX_BREAK_KW)) {
    if (!self->breakable && self->alive) {
      diagnostics(self, diag_break_outside_loop_error(self->token.offset, self->token.length));
    }
    bump(self);
  } else {
    error_unexpected(self);
  }
  close(self, MPPL_SYNTAX_BREAK_STMT, checkpoint);
}

static void parse_act_param_list(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_expr(self);
  } while (eat(self, MPPL_SYNTAX_COMMA_TOKEN));
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_ACT_PARAM_LIST, checkpoint);
}

static void parse_call_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_CALL_KW);
  expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(self, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_act_param_list(self);
  } else {
    null(self);
  }
  close(self, MPPL_SYNTAX_CALL_STMT, checkpoint);
}

static void parse_return_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_RETURN_KW);
  close(self, MPPL_SYNTAX_RETURN_STMT, checkpoint);
}

static void parse_input_list(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_var(self);
  } while (eat(self, MPPL_SYNTAX_COMMA_TOKEN));
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_INPUT_LIST, checkpoint);
}

static const MpplSyntaxKind FIRST_INPUT_STMT[] = { MPPL_SYNTAX_READ_KW, MPPL_SYNTAX_READLN_KW };

static void parse_input_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect_any(self, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(MpplSyntaxKind));
  if (check(self, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(self);
  } else {
    null(self);
  }
  close(self, MPPL_SYNTAX_INPUT_STMT, checkpoint);
}

static void parse_output_value(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  parse_expr(self);
  if (eat(self, MPPL_SYNTAX_COLON_TOKEN)) {
    expect(self, MPPL_SYNTAX_NUMBER_LIT);
  } else {
    null(self);
    null(self);
  }
  close(self, MPPL_SYNTAX_OUTPUT_VALUE, checkpoint);
}

static void parse_output_list(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_output_value(self);
  } while (eat(self, MPPL_SYNTAX_COMMA_TOKEN));
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_OUTPUT_LIST, checkpoint);
}

static const MpplSyntaxKind FIRST_OUTPUT_STMT[] = { MPPL_SYNTAX_WRITE_KW, MPPL_SYNTAX_WRITELN_KW };

static void parse_output_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect_any(self, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(MpplSyntaxKind));
  if (check(self, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(self);
  } else {
    null(self);
  }
  close(self, MPPL_SYNTAX_OUTPUT_STMT, checkpoint);
}

static void parse_comp_stmt(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_BEGIN_KW);
  do {
    parse_stmt(self);
  } while (eat(self, MPPL_SYNTAX_SEMI_TOKEN));
  expect(self, MPPL_SYNTAX_END_KW);
  close(self, MPPL_SYNTAX_COMP_STMT, checkpoint);
}

static void parse_stmt(Parser *self)
{
  if (check(self, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(self);
  } else if (check(self, MPPL_SYNTAX_IF_KW)) {
    parse_if_stmt(self);
  } else if (check(self, MPPL_SYNTAX_WHILE_KW)) {
    parse_while_stmt(self);
  } else if (check(self, MPPL_SYNTAX_BREAK_KW)) {
    parse_break_stmt(self);
  } else if (check(self, MPPL_SYNTAX_CALL_KW)) {
    parse_call_stmt(self);
  } else if (check(self, MPPL_SYNTAX_RETURN_KW)) {
    parse_return_stmt(self);
  } else if (check_any(self, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(MpplSyntaxKind))) {
    parse_input_stmt(self);
  } else if (check_any(self, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(MpplSyntaxKind))) {
    parse_output_stmt(self);
  } else if (check(self, MPPL_SYNTAX_BEGIN_KW)) {
    parse_comp_stmt(self);
  } else {
    null(self);
  }
}

static void parse_var_decl(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  do {
    expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  } while (eat(self, MPPL_SYNTAX_COMMA_TOKEN));
  expect(self, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(self);
  close(self, MPPL_SYNTAX_VAR_DECL, checkpoint);
}

static void parse_var_decl_part(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_VAR_KW);
  do {
    parse_var_decl(self);
    expect_semi(self);
  } while (check(self, MPPL_SYNTAX_IDENT_TOKEN));
  close(self, MPPL_SYNTAX_VAR_DECL_PART, checkpoint);
}

static void parse_fml_param_sec(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  do {
    expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  } while (eat(self, MPPL_SYNTAX_COMMA_TOKEN));
  expect(self, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(self);
  close(self, MPPL_SYNTAX_FML_PARAM_SEC, checkpoint);
}

static void parse_fml_param_list(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_LPAREN_TOKEN);
  do {
    parse_fml_param_sec(self);
  } while (eat(self, MPPL_SYNTAX_SEMI_TOKEN));
  expect(self, MPPL_SYNTAX_RPAREN_TOKEN);
  close(self, MPPL_SYNTAX_FML_PARAM_LIST, checkpoint);
}

static void parse_proc_decl(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_PROCEDURE_KW);
  expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(self, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_fml_param_list(self);
  } else {
    null(self);
  }
  expect_semi(self);
  if (check(self, MPPL_SYNTAX_VAR_KW)) {
    parse_var_decl_part(self);
  } else {
    null(self);
  }
  parse_comp_stmt(self);
  expect_semi(self);
  close(self, MPPL_SYNTAX_PROC_DECL, checkpoint);
}

static void parse_program(Parser *self)
{
  SyntaxCheckpoint checkpoint = open(self);
  expect(self, MPPL_SYNTAX_PROGRAM_KW);
  expect(self, MPPL_SYNTAX_IDENT_TOKEN);
  expect_semi(self);

  while (1) {
    if (check(self, MPPL_SYNTAX_VAR_KW)) {
      parse_var_decl_part(self);
    } else if (check(self, MPPL_SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl(self);
    } else {
      break;
    }
  }
  parse_comp_stmt(self);
  expect(self, MPPL_SYNTAX_DOT_TOKEN);
  expect(self, MPPL_SYNTAX_EOF_TOKEN);
  close(self, MPPL_SYNTAX_PROGRAM, checkpoint);
}

static void mppl_interface_print_kind(FILE *file, RawSyntaxKind kind)
{
  fprintf(file, "%s", mppl_syntax_kind_to_string(kind));
}

static int mppl_interface_is_token(RawSyntaxKind kind)
{
  return mppl_syntax_kind_is_token(kind);
}

void mpplc_parse(const Source *source, MpplParseResult *result)
{
  Parser          self;
  SyntaxInterface interface;

  self.source  = source;
  self.builder = syntax_builder_new();
  bitset_clear(&self.expected);
  self.diagnostics = array_new(sizeof(Diag *));
  self.alive       = 1;
  self.breakable   = 0;

  self.token.kind   = MPPL_SYNTAX_EMPTY;
  self.token.offset = 0;
  self.token.length = 0;

  lex(&self);
  parse_program(&self);

  interface.print_kind = &mppl_interface_print_kind;
  interface.is_token   = &mppl_interface_is_token;

  result->root       = syntax_builder_finish(self.builder, &interface);
  result->diag_count = array_count(self.diagnostics);
  result->diags      = array_steal(self.diagnostics);
}
