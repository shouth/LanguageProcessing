/*
 * parse.c -- parser
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stdlib.h>

#include "compiler.h"
#include "diag.h"
#include "ds.h"
#include "syn.h"

struct parser {
  char const *text;
  size_t len;
  size_t off;

  struct syn_bldr bldr;
  struct token tok;

  struct diag *diag;
  diag_id_t src;
  syn_kinds_t expected;
  size_t loop;
  int recovery;
  int error;

  syn_kinds_t first_type;
  syn_kinds_t first_rel_op;
  syn_kinds_t first_add_op;
  syn_kinds_t first_mul_op;
  syn_kinds_t first_stmt;
};

static void null(struct parser *p)
{
  syn_bldr_empty(&p->bldr);
}

static void bump(struct parser *p)
{
  while (lex(p->text + p->off, p->len - p->off, &p->tok) && p->tok.kind >= TRIV_BEGIN && p->tok.kind <= TRIV_END) {
    syn_bldr_triv(&p->bldr, p->tok.kind, p->text + p->off, p->tok.len);
    p->off += p->tok.len;
  }

  if (p->tok.kind == SYN_ERROR) {
    diag_error_stray_char(p->diag, p->src, p->off, p->text[p->off], &p->expected);
    p->recovery = 1;
    p->error = 1;
  } else if (p->tok.kind == SYN_STRING_LIT && p->tok.nonclosed) {
    diag_error_unterminated_string(p->diag, p->src, p->off, p->tok.len);
    p->error = 1;
  } else if (p->tok.kind == SYN_SLASH_STAR_COMMENT && p->tok.nonclosed) {
    diag_error_unterminated_comment(p->diag, p->src, p->off, p->tok.len);
    p->error = 1;
  } else if (p->tok.kind == SYN_NUMBER_LIT && strtoul(p->text + p->off, NULL, 10) > 32768) {
    diag_error_too_large_integer(p->diag, p->src, p->off, p->tok.len);
    p->error = 1;
  }

  bits_clear(&p->expected);
}

static int at_any(struct parser *p, syn_kinds_t const *kinds)
{
  return bits_test(kinds, p->tok.kind);
}

static int at(struct parser *p, enum syn_kind kind)
{
  syn_kinds_t kinds = { 0 };
  bits_set(&kinds, kind);
  return at_any(p, &kinds);
}

static int check_any(struct parser *p, syn_kinds_t const *kinds)
{
  bits_or(&p->expected, kinds);
  return at_any(p, kinds);
}

static int check(struct parser *p, enum syn_kind kind)
{
  syn_kinds_t kinds = { 0 };
  bits_set(&kinds, kind);
  return check_any(p, &kinds);
}

static int eat_any(struct parser *p, syn_kinds_t const *kinds)
{
  int status = check_any(p, kinds);
  if (status) {
    syn_bldr_tok(&p->bldr, p->tok.kind, p->text + p->off, p->tok.len);
    p->off += p->tok.len;
    bits_clear(&p->expected);
    bump(p);
  }
  return status;
}

static int eat(struct parser *p, enum syn_kind kind)
{
  syn_kinds_t kinds = { 0 };
  bits_set(&kinds, kind);
  return eat_any(p, &kinds);
}

static int expect_any(struct parser *p, syn_kinds_t const *kinds)
{
  if (eat_any(p, kinds)) {
    return 1;
  } else {
    null(p);
    if (!p->recovery) {
      diag_error_unexpected_token(p->diag, p->src, p->off, p->tok.len, p->text + p->off, &p->expected);
      p->recovery = 1;
      p->error = 1;
    }
    return 0;
  }
}

static int expect(struct parser *p, enum syn_kind kind)
{
  syn_kinds_t kinds = { 0 };
  bits_set(&kinds, kind);
  return expect_any(p, &kinds);
}

static int eof(struct parser *p)
{
  return at(p, SYN_EOF);
}

static syn_ckpt_t open(struct parser *p)
{
  return syn_bldr_open(&p->bldr);
}

static void close(struct parser *p, syn_ckpt_t ckpt, enum syn_kind kind)
{
  syn_bldr_close(&p->bldr, kind, ckpt);
}

static void bogus(struct parser *p, syn_ckpt_t ckpt, enum syn_kind kind, syn_kinds_t const *recovery)
{
  while (!eof(p) && !at_any(p, recovery)) {
    eat(p, p->tok.kind);
  }
  close(p, ckpt, kind);
  p->recovery = 0;
}

static void parse_expr(struct parser *p, syn_kinds_t const *sync);

static void parse_type(struct parser *p, syn_kinds_t const *recovery)
{
  if (check(p, SYN_INTEGER_KW)) {
    syn_ckpt_t int_type = open(p);
    expect(p, SYN_INTEGER_KW);
    close(p, int_type, SYN_INT_TYPE);
  } else if (check(p, SYN_BOOLEAN_KW)) {
    syn_ckpt_t bool_type = open(p);
    expect(p, SYN_BOOLEAN_KW);
    close(p, bool_type, SYN_BOOL_TYPE);
  } else if (check(p, SYN_CHAR_KW)) {
    syn_ckpt_t char_type = open(p);
    expect(p, SYN_CHAR_KW);
    close(p, char_type, SYN_CHAR_TYPE);
  } else if (check(p, SYN_ARRAY_KW)) {
    syn_ckpt_t array_type = open(p);
    expect(p, SYN_ARRAY_KW);
    expect(p, SYN_LBRACE);
    {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RBRACE);
      parse_expr(p, &r);
    }
    expect(p, SYN_RBRACE);
    expect(p, SYN_OF_KW);
    parse_type(p, recovery);
    close(p, array_type, SYN_ARRAY_TYPE);
  } else {
    null(p);
  }
}

static void parse_expr_with_power(struct parser *p, syn_kinds_t const *recovery, int power)
{
  syn_ckpt_t ckpt = open(p);

  if (power < 2 && (eat(p, SYN_PLUS) || eat(p, SYN_MINUS))) {
    parse_expr_with_power(p, recovery, 2);
    close(p, ckpt, SYN_UNARY_EXPR);
  } else if (eat(p, SYN_IDENT)) {
    if (eat(p, SYN_LBRACE)) {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RBRACE);
      parse_expr(p, &r);
      expect(p, SYN_RBRACE);
      close(p, ckpt, SYN_IDX_VAR_EXPR);
    } else {
      close(p, ckpt, SYN_ENTIRE_VAR_EXPR);
    }
  } else if (eat(p, SYN_NUMBER_LIT)) {
    close(p, ckpt, SYN_INT_LIT_EXPR);
  } else if (eat(p, SYN_TRUE_KW) || eat(p, SYN_FALSE_KW)) {
    close(p, ckpt, SYN_BOOL_LIT_EXPR);
  } else if (eat(p, SYN_STRING_LIT)) {
    close(p, ckpt, SYN_STR_LIT_EXPR);
  } else if (eat(p, SYN_LPAREN)) {
    {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RPAREN);
      parse_expr(p, &r);
    }
    expect(p, SYN_RPAREN);
    close(p, ckpt, SYN_PAREN_EXPR);
  } else if (eat(p, SYN_NOT_KW)) {
    parse_expr_with_power(p, recovery, 10);
    close(p, ckpt, SYN_UNARY_EXPR);
  } else if (check_any(p, &p->first_type)) {
    {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_LPAREN);
      parse_type(p, &r);
    }
    expect(p, SYN_LPAREN);
    {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RPAREN);
      parse_expr(p, &r);
    }
    expect(p, SYN_RPAREN);
    close(p, ckpt, SYN_CAST_EXPR);
  } else {
    syn_kinds_t r = *recovery;
    bits_or(&r, &p->first_rel_op);
    bits_or(&r, &p->first_add_op);
    bits_or(&r, &p->first_mul_op);
    bogus(p, ckpt, SYN_BOGUS_EXPR, &r);

    diag_error_expected(p->diag, p->src, p->off, p->tok.len, p->text + p->off, "expression");
    p->error = 1;
  }

  while (!eof(p)) {
    if (power < 1 && eat_any(p, &p->first_rel_op)) {
      parse_expr_with_power(p, recovery, 1);
      close(p, ckpt, SYN_BINARY_EXPR);
    } else if (power < 2 && eat_any(p, &p->first_add_op)) {
      parse_expr_with_power(p, recovery, 2);
      close(p, ckpt, SYN_BINARY_EXPR);
    } else if (power < 3 && eat_any(p, &p->first_mul_op)) {
      parse_expr_with_power(p, recovery, 3);
      close(p, ckpt, SYN_BINARY_EXPR);
    } else {
      break;
    }
  }
}

static void parse_expr(struct parser *p, syn_kinds_t const *recovery)
{
  parse_expr_with_power(p, recovery, 0);
}

static void parse_stmt(struct parser *p, syn_kinds_t const *recovery);

static void parse_assign_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t assign_stmt = open(p);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_ASSIGN);
    parse_expr(p, &r);
  }
  expect(p, SYN_ASSIGN);
  parse_expr(p, recovery);
  close(p, assign_stmt, SYN_ASSIGN_STMT);
}

static void parse_if_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t if_stmt = open(p);
  expect(p, SYN_IF_KW);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_THEN_KW);
    parse_expr(p, &r);
  }
  expect(p, SYN_THEN_KW);
  parse_stmt(p, recovery);
  if (check(p, SYN_ELSE_KW)) {
    syn_ckpt_t else_clause = open(p);
    expect(p, SYN_ELSE_KW);
    parse_stmt(p, recovery);
    close(p, else_clause, SYN_ELSE_CLAUSE);
  } else {
    null(p);
  }
  close(p, if_stmt, SYN_IF_STMT);
}

static void parse_while_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t while_stmt = open(p);
  ++p->loop;
  expect(p, SYN_WHILE_KW);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_DO_KW);
    parse_expr(p, &r);
  }
  expect(p, SYN_DO_KW);
  parse_stmt(p, recovery);
  --p->loop;
  close(p, while_stmt, SYN_WHILE_STMT);
}

static void parse_break_stmt(struct parser *p)
{
  syn_ckpt_t break_stmt = open(p);
  assert(at(p, SYN_BREAK_KW));
  if (!p->loop) {
    diag_error_break_outside_loop(p->diag, p->src, p->off, p->tok.len);
    p->error = 1;
  }
  expect(p, SYN_BREAK_KW);
  close(p, break_stmt, SYN_BREAK_STMT);

}

static void parse_act_params(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t act_params = open(p);
  expect(p, SYN_LPAREN);
  {
    syn_ckpt_t expr_list = open(p);
    while (!eof(p) && !check(p, SYN_RPAREN)) {
      syn_ckpt_t expr_list_item = open(p);
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RPAREN);
      bits_set(&r, SYN_COMMA);

      parse_expr(p, &r);
      if (!check(p, SYN_RPAREN)) {
        expect(p, SYN_COMMA);
      } else {
        null(p);
      }
      close(p, expr_list_item, SYN_EXPR_LIST_ITEM);
    }
    close(p, expr_list, SYN_EXPR_LIST);
  }
  expect(p, SYN_RPAREN);
  close(p, act_params, SYN_ACT_PARAMS);
}

static void parse_call_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t call_stmt = open(p);
  expect(p, SYN_CALL_KW);
  expect(p, SYN_IDENT);
  if (check(p, SYN_LPAREN)) {
    parse_act_params(p, recovery);
  } else {
    null(p);
  }
  close(p, call_stmt, SYN_CALL_STMT);
}

static void parse_return_stmt(struct parser *p)
{
  syn_ckpt_t return_stmt = open(p);
  expect(p, SYN_RETURN_KW);
  close(p, return_stmt, SYN_RETURN_STMT);
}

static void parse_input_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t input_stmt = open(p);
  {
    syn_kinds_t op = { 0 };
    bits_set(&op, SYN_READ_KW);
    bits_set(&op, SYN_READLN_KW);
    expect_any(p, &op);
  }
  if (check(p, SYN_LPAREN)) {
    parse_act_params(p, recovery);
  } else {
    null(p);
  }
  close(p, input_stmt, SYN_INPUT_STMT);
}

static void parse_output_value(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t output_value = open(p);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_COLON);
    parse_expr(p, &r);
  }
  if (check(p, SYN_COLON)) {
    syn_ckpt_t output_fmt = open(p);
    expect(p, SYN_COLON);
    parse_expr(p, recovery);
    close(p, output_fmt, SYN_OUTPUT_FMT);
  } else {
    null(p);
  }
  close(p, output_value, SYN_OUTPUT_VALUE);
}

static void parse_output_values(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t output_values = open(p);
  expect(p, SYN_LPAREN);
  {
    syn_ckpt_t output_value_list = open(p);
    while (!eof(p) && !check(p, SYN_RPAREN)) {
      syn_ckpt_t output_value_list_item = open(p);
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_RPAREN);
      bits_set(&r, SYN_COMMA);

      parse_output_value(p, &r);
      if (!check(p, SYN_RPAREN)) {
        expect(p, SYN_COMMA);
      } else {
        null(p);
      }
      close(p, output_value_list_item, SYN_OUTPUT_VALUE_LIST_ITEM);
    }
    close(p, output_value_list, SYN_OUTPUT_VALUE_LIST);
  }
  expect(p, SYN_RPAREN);
  close(p, output_values, SYN_OUTPUT_VALUES);
}

static void parse_output_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t output_stmt = open(p);
  {
    syn_kinds_t op = { 0 };
    bits_set(&op, SYN_WRITE_KW);
    bits_set(&op, SYN_WRITELN_KW);
    expect_any(p, &op);
  }
  if (check(p, SYN_LPAREN)) {
    parse_output_values(p, recovery);
  } else {
    null(p);
  }
  close(p, output_stmt, SYN_OUTPUT_STMT);
}

static void parse_comp_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t comp_stmt = open(p);
  expect(p, SYN_BEGIN_KW);
  {
    syn_ckpt_t stmt_list = open(p);
    while (!eof(p) && !check(p, SYN_END_KW)) {
      syn_ckpt_t stmt_list_item = open(p);
      syn_kinds_t r = *recovery;
      bits_or(&r, &p->first_stmt);
      bits_set(&r, SYN_SEMI);
      bits_set(&r, SYN_END_KW);

      if (check_any(p, &p->first_stmt)) {
        parse_stmt(p, &r);
      } else if (at_any(p, recovery)) {
        break;
      } else {
        bogus(p, open(p), SYN_BOGUS_STMT, &r);
      }
      if (!check(p, SYN_END_KW)) {
        expect(p, SYN_SEMI);
      } else {
        null(p);
      }
      close(p, stmt_list_item, SYN_STMT_LIST_ITEM);
    }
    close(p, stmt_list, SYN_STMT_LIST);
  }
  expect(p, SYN_END_KW);
  close(p, comp_stmt, SYN_COMP_STMT);
}

static void parse_stmt(struct parser *p, syn_kinds_t const *recovery)
{
  if (check(p, SYN_IDENT)) {
    parse_assign_stmt(p, recovery);
  } else if (check(p, SYN_IF_KW)) {
    parse_if_stmt(p, recovery);
  } else if (check(p, SYN_WHILE_KW)) {
    parse_while_stmt(p, recovery);
  } else if (check(p, SYN_BREAK_KW)) {
    parse_break_stmt(p);
  } else if (check(p, SYN_CALL_KW)) {
    parse_call_stmt(p, recovery);
  } else if (check(p, SYN_RETURN_KW)) {
    parse_return_stmt(p);
  } else if (check(p, SYN_READ_KW) || check(p, SYN_READLN_KW)) {
    parse_input_stmt(p, recovery);
  } else if (check(p, SYN_WRITE_KW) || check(p, SYN_WRITELN_KW)) {
    parse_output_stmt(p, recovery);
  } else if (check(p, SYN_BEGIN_KW)) {
    parse_comp_stmt(p, recovery);
  } else {
    close(p, open(p), SYN_EMPTY_STMT);
  }
}

static void parse_ident_list(struct parser *p, syn_kinds_t const *next, syn_kinds_t const *recovery)
{
  syn_ckpt_t ident_list = open(p);
  while (!eof(p) && !check_any(p, next)) {
    syn_ckpt_t ident_list_item = open(p);
    if (!eat(p, SYN_IDENT)) {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_IDENT);
      bits_set(&r, SYN_COMMA);
      bogus(p, open(p), SYN_BOGUS_IDENT, &r);
    }
    if (!check_any(p, next)) {
      expect(p, SYN_COMMA);
    } else {
      null(p);
    }
    close(p, ident_list_item, SYN_IDENT_LIST_ITEM);
  }
  close(p, ident_list, SYN_IDENT_LIST);
}

static void parse_var_decl(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t var_decl = open(p);
  {
    syn_kinds_t next = { 0 };
    syn_kinds_t r = *recovery;
    bits_set(&next, SYN_COLON);
    bits_or(&r, &next);
    parse_ident_list(p, &next, &r);
  }
  expect(p, SYN_COLON);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_SEMI);
    parse_type(p, &r);
  }
  expect(p, SYN_SEMI);
  close(p, var_decl, SYN_VAR_DECL);
}

static void parse_var_decl_part(struct parser *p, syn_kinds_t const *next, syn_kinds_t const *recovery)
{
  syn_ckpt_t var_decl_part = open(p);
  expect(p, SYN_VAR_KW);
  {
    syn_ckpt_t var_decl_list = open(p);
    while (!eof(p) && !check_any(p, next)) {
      syn_kinds_t r = *recovery;
      bits_set(&r, SYN_IDENT);

      if (check(p, SYN_IDENT)) {
        parse_var_decl(p, &r);
      } else if (at_any(p, recovery)) {
        break;
      } else {
        bogus(p, open(p), SYN_BOGUS_VAR_DECL, &r);
      }
    }
    close(p, var_decl_list, SYN_VAR_DECL_LIST);
  }
  close(p, var_decl_part, SYN_VAR_DECL_PART);
}

static void parse_fml_param_sec(struct parser *p, syn_kinds_t const *next, syn_kinds_t const *recovery)
{
  syn_ckpt_t fml_param_sec = open(p);
  {
    syn_kinds_t next = { 0 };
    syn_kinds_t r = *recovery;
    bits_set(&next, SYN_COLON);
    bits_or(&r, &next);
    parse_ident_list(p, &next, &r);
  }
  expect(p, SYN_COLON);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_SEMI);
    parse_type(p, &r);
  }
  if (!check_any(p, next)) {
    expect(p, SYN_SEMI);
  } else {
    null(p);
  }
  close(p, fml_param_sec, SYN_FML_PARAM_SEC);
}

static void parse_fml_params(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t fml_params = open(p);
  expect(p, SYN_LPAREN);
  {
    syn_ckpt_t fml_param_list = open(p);
    while (!eof(p) && !check(p, SYN_RPAREN)) {
      syn_kinds_t next = { 0 };
      syn_kinds_t r = *recovery;
      bits_set(&next, SYN_RPAREN);
      bits_set(&r, SYN_IDENT);
      bits_or(&r, &next);

      if (check(p, SYN_IDENT)) {
        parse_fml_param_sec(p, &next, &r);
      } else if (at_any(p, recovery)) {
        break;
      } else {
        bogus(p, open(p), SYN_BOGUS_FML_PARAM_SEC, &r);
      }
    }
    close(p, fml_param_list, SYN_FML_PARAM_LIST);
  }
  expect(p, SYN_RPAREN);
  close(p, fml_params, SYN_FML_PARAMS);
}

static void parse_proc_decl_head(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t proc_decl_head = open(p);
  expect(p, SYN_PROCEDURE_KW);
  expect(p, SYN_IDENT);
  if (check(p, SYN_LPAREN)) {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_SEMI);
    parse_fml_params(p, &r);
  } else {
    null(p);
  }
  expect(p, SYN_SEMI);
  close(p, proc_decl_head, SYN_PROC_DECL_HEAD);
}

static void parse_proc_decl_part(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t proc_decl_part = open(p);
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_VAR_KW);
    bits_set(&r, SYN_BEGIN_KW);
    parse_proc_decl_head(p, &r);
  }
  if (check(p, SYN_VAR_KW)) {
    syn_kinds_t next = { 0 };
    syn_kinds_t r = *recovery;
    bits_set(&next, SYN_BEGIN_KW);
    bits_or(&r, &next);
    parse_var_decl_part(p, &next, &r);
  } else {
    null(p);
  }
  {
    syn_kinds_t r = *recovery;
    bits_set(&r, SYN_SEMI);
    parse_comp_stmt(p, &r);
  }
  expect(p, SYN_SEMI);
  close(p, proc_decl_part, SYN_PROC_DECL_PART);
}

static void parse_block(struct parser *p, syn_kinds_t const *recovery)
{
  syn_ckpt_t block = open(p);
  {
    syn_ckpt_t decl_part_list = open(p);
    while (!eof(p) && !check(p, SYN_BEGIN_KW)) {
      syn_kinds_t next = { 0 };
      syn_kinds_t r = *recovery;
      bits_set(&next, SYN_BEGIN_KW);
      bits_set(&next, SYN_VAR_KW);
      bits_set(&next, SYN_PROCEDURE_KW);
      bits_or(&r, &next);

      if (check(p, SYN_VAR_KW)) {
        parse_var_decl_part(p, &next, &r);
      } else if (check(p, SYN_PROCEDURE_KW)) {
        parse_proc_decl_part(p, &r);
      } else if (at_any(p, recovery)) {
        break;
      } else {
        bogus(p, open(p), SYN_BOGUS_DECL_PART, &r);
      }
    }
    close(p, decl_part_list, SYN_DECL_PART_LIST);
  }
  parse_comp_stmt(p, recovery);
  close(p, block, SYN_BLOCK);
}

static void parse_program(struct parser *p)
{
  syn_ckpt_t program = open(p);
  expect(p, SYN_PROGRAM_KW);
  expect(p, SYN_IDENT);
  expect(p, SYN_SEMI);
  {
    syn_kinds_t r = { 0 };
    bits_set(&r, SYN_DOT);
    parse_block(p, &r);
  }
  expect(p, SYN_DOT);
  if (check(p, SYN_EOF)) {
    close(p, open(p), SYN_EMPTY_END);
  } else {
    syn_kinds_t r = { 0 };
    bits_set(&r, SYN_EOF);
    bogus(p, open(p), SYN_BOGUS_END, &r);
  }
  expect(p, SYN_EOF);
  close(p, program, SYN_PROGRAM);
}

int parse(char const *text, size_t len, char const *filename, struct diag *diag, struct syn_program **program)
{
  struct parser p;
  p.text = text;
  p.len = len;
  p.off = 0;

  syn_bldr_init(&p.bldr);
  bump(&p);

  p.diag = diag;
  p.src = diag_add_source(diag, filename, text, len);
  bits_clear(&p.expected);
  p.loop = 0;
  p.recovery = 0;
  p.error = 0;

  bits_set(&p.first_rel_op, SYN_EQ);
  bits_set(&p.first_rel_op, SYN_NEQ);
  bits_set(&p.first_rel_op, SYN_LT);
  bits_set(&p.first_rel_op, SYN_GT);
  bits_set(&p.first_rel_op, SYN_LTEQ);
  bits_set(&p.first_rel_op, SYN_GTEQ);

  bits_set(&p.first_add_op, SYN_PLUS);
  bits_set(&p.first_add_op, SYN_MINUS);
  bits_set(&p.first_add_op, SYN_OR_KW);

  bits_set(&p.first_mul_op, SYN_STAR);
  bits_set(&p.first_mul_op, SYN_DIV_KW);
  bits_set(&p.first_mul_op, SYN_AND_KW);

  bits_set(&p.first_type, SYN_INTEGER_KW);
  bits_set(&p.first_type, SYN_BOOLEAN_KW);
  bits_set(&p.first_type, SYN_CHAR_KW);
  bits_set(&p.first_type, SYN_ARRAY_KW);

  bits_set(&p.first_stmt, SYN_IDENT);
  bits_set(&p.first_stmt, SYN_IF_KW);
  bits_set(&p.first_stmt, SYN_WHILE_KW);
  bits_set(&p.first_stmt, SYN_BREAK_KW);
  bits_set(&p.first_stmt, SYN_CALL_KW);
  bits_set(&p.first_stmt, SYN_RETURN_KW);
  bits_set(&p.first_stmt, SYN_READ_KW);
  bits_set(&p.first_stmt, SYN_READLN_KW);
  bits_set(&p.first_stmt, SYN_WRITE_KW);
  bits_set(&p.first_stmt, SYN_WRITELN_KW);
  bits_set(&p.first_stmt, SYN_BEGIN_KW);

  parse_program(&p);
  *program = (struct syn_program *) syn_bldr_finish(&p.bldr);
  syn_bldr_deinit(&p.bldr);
  return !p.error;
}
