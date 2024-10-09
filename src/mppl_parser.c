/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>

#include "diag.h"
#include "mppl_passes.h"
#include "mppl_syntax.h"
#include "stdio.h"
#include "string.h"
#include "syntax_tree.h"
#include "util.h"

typedef SyntaxCheckpoint Checkpoint;
typedef struct Parser    Parser;

struct Parser {
  const char    *text;
  unsigned long  length;
  unsigned long  offset;
  MpplSyntaxKind kind;
  unsigned long  span;

  SyntaxBuilder   *builder;
  MpplTokenKindSet expected;
  Vec(Report *) diags;
  unsigned long breakable;
  int           recovery;
};

static void diag(Parser *p, Report *diagnostics)
{
  vec_push(&p->diags, &diagnostics, 1);
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
    if (strtoul(p->text + p->offset, NULL, 10) > 32767) {
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
  Vec(RawSyntaxTriviaPiece) trivia_pieces;
  unsigned long trivia_offset = p->offset + p->span;

  vec_alloc(&trivia_pieces, 0);
  while (next_token(p), p->kind >= MPPL_BEGIN_TRIVIA && p->kind <= MPPL_END_TRIVIA) {
    RawSyntaxTriviaPiece piece;
    piece.kind             = p->kind;
    piece.span.text_length = p->span;
    vec_push(&trivia_pieces, &piece, 1);
  }
  syntax_builder_trivia(p->builder, p->text + trivia_offset, trivia_pieces.ptr, trivia_pieces.count);
  vec_free(&trivia_pieces);
}

static int is_eof(Parser *p)
{
  return p->kind == MPPL_SYNTAX_EOF_TOKEN;
}

static void null(Parser *p)
{
  syntax_builder_empty(p->builder);
}

static void bump(Parser *p)
{
  syntax_builder_token(p->builder, p->kind, p->text + p->offset, p->span);
  bitset_clear(&p->expected);
  next_nontrivia(p);
}

static int check_any(Parser *p, const MpplTokenKindSet *kinds)
{
  if (kinds) {
    bitset_insert(&p->expected, kinds);
    return bitset_get(kinds, p->kind);
  } else {
    return 0;
  }
}

static int check(Parser *p, MpplSyntaxKind kind)
{
  MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind - MPPL_BEGIN_TOKEN);
  return check_any(p, &kinds);
}

static int eat_any(Parser *p, const MpplTokenKindSet *kinds)
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
  MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind);
  return eat_any(p, &kinds);
}

static void error_unexpected(Parser *p)
{
  if (p->kind == MPPL_SYNTAX_ERROR) {
    diag(p, diag_stray_char_error(p->offset, p->text[p->offset], p->expected));
  } else {
    diag(p, diag_unexpected_token_error(p->offset, p->span, p->text + p->offset, p->expected));
  }
}

static int expect_any(Parser *p, const MpplTokenKindSet *kinds)
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
  MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, kind);
  return expect_any(p, &kinds);
}

static Checkpoint open(Parser *p)
{
  return syntax_builder_open(p->builder);
}

static void close(Parser *p, MpplSyntaxKind kind, Checkpoint checkpoint)
{
  syntax_builder_close(p->builder, kind, checkpoint);
}

static void parse_bogus(Parser *p, MpplSyntaxKind kind, const MpplTokenKindSet *kinds)
{
  Checkpoint bogus = open(p);
  if (!p->recovery) {
    error_unexpected(p);
  }
  while (!check_any(p, kinds) && !is_eof(p)) {
    p->kind = MPPL_SYNTAX_ERROR;
    bump(p);
  }
  p->recovery = is_eof(p);
  close(p, kind, bogus);
}

static const MpplTokenKindSet *first_std_type(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
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
  Checkpoint array_type = open(p);
  expect(p, MPPL_SYNTAX_ARRAY_KW);
  expect(p, MPPL_SYNTAX_LBRACKET_TOKEN);
  expect(p, MPPL_SYNTAX_NUMBER_LIT);
  expect(p, MPPL_SYNTAX_RBRACKET_TOKEN);
  expect(p, MPPL_SYNTAX_OF_KW);
  parse_std_type(p);
  close(p, MPPL_SYNTAX_ARRAY_TYPE, array_type);
}

static void parse_type(Parser *p)
{
  if (check_any(p, first_std_type())) {
    parse_std_type(p);
  } else {
    parse_array_type(p);
  }
}

static void parse_ref_ident(Parser *p)
{
  Checkpoint reference_ident = open(p);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  close(p, MPPL_SYNTAX_REF_IDENT, reference_ident);
}

static void parse_expr(Parser *p);

static const MpplTokenKindSet *first_const(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_NUMBER_LIT);
  bitset_set(&kinds, MPPL_SYNTAX_TRUE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_FALSE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_STRING_LIT);
  return &kinds;
}

static const MpplTokenKindSet *first_multi_op(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_STAR_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_DIV_KW);
  bitset_set(&kinds, MPPL_SYNTAX_AND_KW);
  return &kinds;
}

static const MpplTokenKindSet *first_add_op(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_PLUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_MINUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_OR_KW);
  return &kinds;
}

static const MpplTokenKindSet *first_relat_op(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_EQUAL_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_NOTEQ_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_LESS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_LESSEQ_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_GREATER_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_GREATEREQ_TOKEN);
  return &kinds;
}

static const MpplTokenKindSet *first_expr(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_PLUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_MINUS_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_NOT_KW);
  bitset_set(&kinds, MPPL_SYNTAX_LPAREN_TOKEN);
  bitset_set(&kinds, MPPL_SYNTAX_IDENT_TOKEN);
  bitset_insert(&kinds, first_const());
  bitset_insert(&kinds, first_const());
  return &kinds;
}

static void parse_expr_with_power(Parser *p, int power)
{
  Checkpoint expr = open(p);

  if (power < 20 && (eat(p, MPPL_SYNTAX_PLUS_TOKEN) || eat(p, MPPL_SYNTAX_MINUS_TOKEN))) {
    /* ('+' | '-') expr */
    parse_expr_with_power(p, 21);
    close(p, MPPL_SYNTAX_UNARY_EXPR, expr);
  } else if (eat(p, MPPL_SYNTAX_NOT_KW)) {
    /* 'not' expr */
    parse_expr_with_power(p, 100);
    close(p, MPPL_SYNTAX_UNARY_EXPR, expr);
  } else if (eat(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    /* '(' expr ')' */
    parse_expr(p);
    expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
    close(p, MPPL_SYNTAX_PAREN_EXPR, expr);
  } else if (eat_any(p, first_std_type())) {
    /* type '(' expr ')' */
    expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
    parse_expr(p);
    expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
    close(p, MPPL_SYNTAX_CAST_EXPR, expr);
  } else if (eat_any(p, first_const())) {
    /* number | true | false | string */
  } else if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_ref_ident(p);
    if (eat(p, MPPL_SYNTAX_LBRACKET_TOKEN)) {
      /* identifier '[' expr ']' */
      parse_expr(p);
      expect(p, MPPL_SYNTAX_RBRACKET_TOKEN);
      close(p, MPPL_SYNTAX_INDEXED_VAR, expr);
    } else {
      /* identifier */
      close(p, MPPL_SYNTAX_ENTIRE_VAR, expr);
    }
  } else {
    diag(p, diag_expected_expression_error(p->offset, p->span));
    null(p);
    bitset_clear(&p->expected);
    p->recovery = 1;
    return;
  }

  while (1) {
    if (power < 10 && eat_any(p, first_relat_op())) {
      /* expr ('=' | '<>' | '<' | '<=' | '>' | '>=') expr */
      parse_expr_with_power(p, 11);
      close(p, MPPL_SYNTAX_BINARY_EXPR, expr);
    } else if (power < 20 && eat_any(p, first_add_op())) {
      /* expr ('+' | '-' | 'or') expr */
      parse_expr_with_power(p, 21);
      close(p, MPPL_SYNTAX_BINARY_EXPR, expr);
    } else if (power < 30 && eat_any(p, first_multi_op())) {
      /* expr ('*' | 'div' | 'and') expr */
      parse_expr_with_power(p, 31);
      close(p, MPPL_SYNTAX_BINARY_EXPR, expr);
    } else {
      break;
    }
  }
}

static void parse_expr(Parser *p)
{
  parse_expr_with_power(p, 0);
}

static void parse_stmt(Parser *p, const MpplTokenKindSet *recovery);

static const MpplTokenKindSet *first_input_stmt(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_READ_KW);
  bitset_set(&kinds, MPPL_SYNTAX_READLN_KW);
  return &kinds;
}

static const MpplTokenKindSet *first_output_stmt(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
  bitset_set(&kinds, MPPL_SYNTAX_WRITE_KW);
  bitset_set(&kinds, MPPL_SYNTAX_WRITELN_KW);
  return &kinds;
}

static const MpplTokenKindSet *first_stmt(void)
{
  static MpplTokenKindSet kinds = bitset_zero();
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

static void parse_assign_stmt(Parser *p)
{
  Checkpoint assign_stmt = open(p);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_ASSIGN_TOKEN);
  parse_expr(p);
  close(p, MPPL_SYNTAX_ASSIGN_STMT, assign_stmt);
}

static void parse_if_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint if_stmt = open(p);
  expect(p, MPPL_SYNTAX_IF_KW);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_THEN_KW);
  {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_ELSE_KW);
    parse_stmt(p, &kinds);
  }
  if (check(p, MPPL_SYNTAX_ELSE_KW)) {
    Checkpoint else_clause = open(p);
    expect(p, MPPL_SYNTAX_ELSE_KW);
    parse_stmt(p, recovery);
    close(p, MPPL_SYNTAX_ELSE_CLAUSE, else_clause);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_IF_STMT, if_stmt);
}

static void parse_while_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint while_stmt = open(p);
  ++p->breakable;
  expect(p, MPPL_SYNTAX_WHILE_KW);
  parse_expr(p);
  expect(p, MPPL_SYNTAX_DO_KW);
  parse_stmt(p, recovery);
  --p->breakable;
  close(p, MPPL_SYNTAX_WHILE_STMT, while_stmt);
}

static void parse_break_stmt(Parser *p)
{
  Checkpoint    break_stmt = open(p);
  unsigned long offset     = p->offset;
  unsigned long span       = p->span;
  if (expect(p, MPPL_SYNTAX_BREAK_KW) && !p->breakable) {
    diag(p, diag_break_outside_loop_error(offset, span));
  }
  close(p, MPPL_SYNTAX_BREAK_STMT, break_stmt);
}

static void parse_act_param_list(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint act_params, expr_list, expr_list_elem;

  act_params = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  expr_list = open(p);
  while (!check(p, MPPL_SYNTAX_RPAREN_TOKEN) && !is_eof(p)) {
    expr_list_elem = open(p);
    if (check_any(p, first_expr())) {
      parse_expr(p);
    } else {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
      bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);
      parse_bogus(p, MPPL_SYNTAX_BOGUS_EXPR, &kinds);
    }

    if (check(p, MPPL_SYNTAX_RPAREN_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_COMMA_TOKEN);
    }
    close(p, MPPL_SYNTAX_EXPR_LIST_ELEM, expr_list_elem);
  }
  close(p, MPPL_SYNTAX_EXPR_LIST, expr_list);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_ACT_PARAMS, act_params);
}

static void parse_call_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint call_stmt = open(p);
  expect(p, MPPL_SYNTAX_CALL_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_act_param_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_CALL_STMT, call_stmt);
}

static void parse_return_stmt(Parser *p)
{
  Checkpoint return_stmt = open(p);
  expect(p, MPPL_SYNTAX_RETURN_KW);
  close(p, MPPL_SYNTAX_RETURN_STMT, return_stmt);
}

static void parse_input_list(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint inputs, expr_list, expr_list_elem;

  inputs = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  expr_list = open(p);
  while (!check(p, MPPL_SYNTAX_RPAREN_TOKEN) && !is_eof(p)) {
    expr_list_elem = open(p);
    if (check_any(p, first_expr())) {
      parse_expr(p);
    } else {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
      bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);
      parse_bogus(p, MPPL_SYNTAX_BOGUS_EXPR, &kinds);
    }

    if (check(p, MPPL_SYNTAX_RPAREN_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_COMMA_TOKEN);
    }
    close(p, MPPL_SYNTAX_EXPR_LIST_ELEM, expr_list_elem);
  }
  close(p, MPPL_SYNTAX_EXPR_LIST, expr_list);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_INPUTS, inputs);
}

static void parse_input_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint input_stmt = open(p);
  expect_any(p, first_input_stmt());
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_INPUT_STMT, input_stmt);
}

static void parse_output_value(Parser *p)
{
  Checkpoint output_value = open(p);
  parse_expr(p);
  if (eat(p, MPPL_SYNTAX_COLON_TOKEN)) {
    expect(p, MPPL_SYNTAX_NUMBER_LIT);
    close(p, MPPL_SYNTAX_OUTPUT_VALUE, output_value);
  }
}

static void parse_output_list(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint outputs, output_list, output_list_elem;

  outputs = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  output_list = open(p);
  while (!check(p, MPPL_SYNTAX_RPAREN_TOKEN) && !is_eof(p)) {
    output_list_elem = open(p);
    if (check_any(p, first_expr())) {
      parse_output_value(p);
    } else {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
      bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);
      parse_bogus(p, MPPL_SYNTAX_BOGUS_OUTPUT_VALUE, &kinds);
    }

    if (check(p, MPPL_SYNTAX_RPAREN_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_COMMA_TOKEN);
    }
    close(p, MPPL_SYNTAX_OUTPUT_LIST_ELEM, output_list_elem);
  }
  close(p, MPPL_SYNTAX_OUTPUT_LIST, output_list);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_OUTPUTS, outputs);
}

static void parse_output_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint output_stmt = open(p);
  expect_any(p, first_output_stmt());
  if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(p, recovery);
  } else {
    null(p);
  }
  close(p, MPPL_SYNTAX_OUTPUT_STMT, output_stmt);
}

static void parse_comp_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint comp_stmt, stmt_list, stmt_list_elem;

  comp_stmt = open(p);
  expect(p, MPPL_SYNTAX_BEGIN_KW);
  stmt_list = open(p);
  while (!check(p, MPPL_SYNTAX_END_KW) && !is_eof(p)) {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_END_KW);

    stmt_list_elem = open(p);
    if (check_any(p, first_stmt()) || check(p, MPPL_SYNTAX_SEMI_TOKEN)) {
      parse_stmt(p, &kinds);
    } else {
      parse_bogus(p, MPPL_SYNTAX_BOGUS_STMT, &kinds);
    }

    if (check(p, MPPL_SYNTAX_END_KW)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_SEMI_TOKEN);
    }
    close(p, MPPL_SYNTAX_STMT_LIST_ELEM, stmt_list_elem);
  }
  close(p, MPPL_SYNTAX_STMT_LIST, stmt_list);
  expect(p, MPPL_SYNTAX_END_KW);
  close(p, MPPL_SYNTAX_COMP_STMT, comp_stmt);
}

static void parse_stmt(Parser *p, const MpplTokenKindSet *recovery)
{
  if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(p);
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

static void parse_binding_ident(Parser *p)
{
  Checkpoint binding_ident = open(p);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  close(p, MPPL_SYNTAX_BIND_IDENT, binding_ident);
}

static void parse_var_decl(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint var_decl, ident_list, ident_list_elem;

  var_decl   = open(p);
  ident_list = open(p);
  while (!check(p, MPPL_SYNTAX_COLON_TOKEN) && !is_eof(p)) {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_COLON_TOKEN);

    ident_list_elem = open(p);
    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_binding_ident(p);
    } else {
      parse_bogus(p, MPPL_SYNTAX_BOGUS_IDENT, &kinds);
    }

    if (check(p, MPPL_SYNTAX_COLON_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_COMMA_TOKEN);
    }
    close(p, MPPL_SYNTAX_BIND_IDENT_LIST_ELEM, ident_list_elem);
  }
  close(p, MPPL_SYNTAX_BIND_IDENT_LIST, ident_list);
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_VAR_DECL, var_decl);
}

static void parse_var_decl_part(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint var_decl_part, var_decl_list, var_decl_list_elem;

  var_decl_part = open(p);
  expect(p, MPPL_SYNTAX_VAR_KW);
  var_decl_list = open(p);
  while (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);

    var_decl_list_elem = open(p);
    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_var_decl(p, &kinds);
    } else {
      parse_bogus(p, MPPL_SYNTAX_BOGUS_VAR_DECL, &kinds);
    }

    expect(p, MPPL_SYNTAX_SEMI_TOKEN);
    close(p, MPPL_SYNTAX_VAR_DECL_LIST_ELEM, var_decl_list_elem);
  }
  close(p, MPPL_SYNTAX_VAR_DECL_LIST, var_decl_list);
  close(p, MPPL_SYNTAX_VAR_DECL_PART, var_decl_part);
}

static void parse_fml_param_sec(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint fml_param_sec, ident_list, ident_list_elem;

  fml_param_sec = open(p);
  ident_list    = open(p);
  while (!check(p, MPPL_SYNTAX_COLON_TOKEN) && !is_eof(p)) {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_COMMA_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_COLON_TOKEN);

    ident_list_elem = open(p);
    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_binding_ident(p);
    } else {
      parse_bogus(p, MPPL_SYNTAX_BOGUS_IDENT, &kinds);
    }

    if (check(p, MPPL_SYNTAX_COLON_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_COMMA_TOKEN);
    }
    close(p, MPPL_SYNTAX_BIND_IDENT_LIST_ELEM, ident_list_elem);
  }
  close(p, MPPL_SYNTAX_BIND_IDENT_LIST, ident_list);
  expect(p, MPPL_SYNTAX_COLON_TOKEN);
  parse_type(p);
  close(p, MPPL_SYNTAX_FML_PARAM_SEC, fml_param_sec);
}

static void parse_fml_param_list(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint fml_params, fml_param_list, fml_param_list_elem;

  fml_params = open(p);
  expect(p, MPPL_SYNTAX_LPAREN_TOKEN);
  fml_param_list = open(p);
  while (!check(p, MPPL_SYNTAX_RPAREN_TOKEN) && !is_eof(p)) {
    MpplTokenKindSet kinds = *recovery;
    bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
    bitset_set(&kinds, MPPL_SYNTAX_RPAREN_TOKEN);

    fml_param_list_elem = open(p);
    if (check(p, MPPL_SYNTAX_IDENT_TOKEN)) {
      parse_fml_param_sec(p, &kinds);
    } else {
      parse_bogus(p, MPPL_SYNTAX_FML_PARAM_SEC, &kinds);
    }

    if (check(p, MPPL_SYNTAX_RPAREN_TOKEN)) {
      null(p);
    } else {
      expect(p, MPPL_SYNTAX_SEMI_TOKEN);
    }
    close(p, MPPL_SYNTAX_FML_PARAM_LIST_ELEM, fml_param_list_elem);
  }
  close(p, MPPL_SYNTAX_FML_PARAM_LIST, fml_param_list);
  expect(p, MPPL_SYNTAX_RPAREN_TOKEN);
  close(p, MPPL_SYNTAX_FML_PARAMS, fml_params);
}

static void parse_proc_decl_part(Parser *p, const MpplTokenKindSet *recovery)
{
  Checkpoint proc_decl_part = open(p);
  Checkpoint proc_decl      = open(p);
  {
    Checkpoint proc_head = open(p);
    expect(p, MPPL_SYNTAX_PROCEDURE_KW);
    expect(p, MPPL_SYNTAX_IDENT_TOKEN);
    if (check(p, MPPL_SYNTAX_LPAREN_TOKEN)) {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
      bitset_set(&kinds, MPPL_SYNTAX_VAR_KW);
      bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);
      parse_fml_param_list(p, &kinds);
    } else {
      null(p);
    }
    close(p, MPPL_SYNTAX_PROC_HEADING, proc_head);
  }
  expect(p, MPPL_SYNTAX_SEMI_TOKEN);
  {
    Checkpoint proc_body = open(p);
    if (check(p, MPPL_SYNTAX_VAR_KW)) {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
      bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);
      parse_var_decl_part(p, &kinds);
    } else {
      null(p);
    }
    {
      MpplTokenKindSet kinds = *recovery;
      bitset_set(&kinds, MPPL_SYNTAX_SEMI_TOKEN);
      parse_comp_stmt(p, &kinds);
    }
    close(p, MPPL_SYNTAX_PROC_BODY, proc_body);
  }
  close(p, MPPL_SYNTAX_PROC_DECL, proc_decl);
  expect(p, MPPL_SYNTAX_SEMI_TOKEN);
  close(p, MPPL_SYNTAX_PROC_DECL_PART, proc_decl_part);
}

static void parse_program(Parser *p)
{
  Checkpoint program, decl_list;

  program = open(p);
  expect(p, MPPL_SYNTAX_PROGRAM_KW);
  expect(p, MPPL_SYNTAX_IDENT_TOKEN);
  expect(p, MPPL_SYNTAX_SEMI_TOKEN);

  decl_list = open(p);
  while (!check(p, MPPL_SYNTAX_BEGIN_KW) && !is_eof(p)) {
    MpplTokenKindSet kinds = bitset_zero();
    bitset_set(&kinds, MPPL_SYNTAX_VAR_KW);
    bitset_set(&kinds, MPPL_SYNTAX_PROCEDURE_KW);
    bitset_set(&kinds, MPPL_SYNTAX_BEGIN_KW);

    if (check(p, MPPL_SYNTAX_VAR_KW)) {
      parse_var_decl_part(p, &kinds);
    } else if (check(p, MPPL_SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl_part(p, &kinds);
    } else {
      parse_bogus(p, MPPL_SYNTAX_BOGUS_DECL_PART, &kinds);
    }
  }
  close(p, MPPL_SYNTAX_DECL_PART_LIST, decl_list);
  {
    MpplTokenKindSet kinds = bitset_zero();
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

  p.builder = syntax_builder_new();
  bitset_clear(&p.expected);
  vec_alloc(&p.diags, 0);
  p.breakable = 0;
  p.recovery  = 0;

  next_nontrivia(&p); /* initialize `p.span` and `p.kind` */
  parse_program(&p);
  if (check(&p, MPPL_SYNTAX_EOF_TOKEN)) {
    Checkpoint eof = open(&p);
    expect(&p, MPPL_SYNTAX_EOF_TOKEN);
    close(&p, MPPL_SYNTAX_EOF, eof);
  } else {
    parse_bogus(&p, MPPL_SYNTAX_BOGUS_EOF, NULL);
  }

  result.root        = syntax_builder_finish(p.builder);
  result.diags.count = p.diags.count;
  result.diags.ptr   = p.diags.ptr;

  return result;
}
