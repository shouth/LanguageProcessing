/*
 * syn.h -- syntax
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYN_H
#define SYN_H

#include <stddef.h>
#include <stdio.h>

#include "ds.h"

#define DEF_FLD_SYN_PROGRAM(FLD) \
  FLD(SYN_PROGRAM_KW, syn_tok, program_kw) \
  FLD(SYN_IDENT, syn_tok, name) \
  FLD(SYN_SEMI, syn_tok, semi) \
  FLD(SYN_BLOCK, syn_tok, block) \
  FLD(SYN_DOT, syn_tok, dot) \
  FLD(SYN_EOF, syn_tok, eof)

#define DEF_FLD_SYN_BLOCK(FLD) \
  FLD(SYN_DECL_PART_LIST, syn_decl_part_list, decl_part_list) \
  FLD(SYN_COMP_STMT, syn_comp_stmt, comp_stmt)

#define DEF_OPT_ANY_SYN_DECL_PART(OPT) \
  OPT(SYN_VAR_DECL_PART, ANY_SYN_DECL_PART_VAR, syn_var_decl_part) \
  OPT(SYN_PROC_DECL_PART, ANY_SYN_DECL_PART_PROC, syn_proc_decl_part)

#define DEF_FLD_SYN_VAR_DECL_PART(FLD) \
  FLD(SYN_VAR_KW, syn_tok, var_kw) \
  FLD(SYN_VAR_DECL_LIST, syn_var_decl_list, var_decl_list)

#define DEF_FLD_SYN_VAR_DECL(FLD) \
  FLD(SYN_IDENT_LIST, syn_ident_list, ident_list) \
  FLD(SYN_COLON, syn_tok, colon) \
  FLD(ANY_SYN_TYPE, any_syn_type, type) \
  FLD(SYN_SEMI, syn_tok, semi)

#define DEF_FLD_SYN_PROC_DECL_PART(FLD) \
  FLD(SYN_PROC_DECL_HEAD, syn_proc_decl_head, proc_decl_head) \
  FLD(SYN_VAR_DECL_PART, syn_var_decl_part, var_decl_part) \
  FLD(SYN_COMP_STMT, syn_comp_stmt, comp_stmt) \
  FLD(SYN_SEMI, syn_tok, semi)

#define DEF_FLD_SYN_PROC_DECL_HEAD(FLD) \
  FLD(SYN_PROCEDURE_KW, syn_tok, procedure_kw) \
  FLD(SYN_IDENT, syn_tok, name) \
  FLD(SYN_FML_PARAMS, syn_fml_params, fml_params) \
  FLD(SYN_SEMI, syn_tok, semi)

#define DEF_FLD_SYN_FML_PARAMS(FLD) \
  FLD(SYN_LPAREN, syn_tok, lparen) \
  FLD(SYN_FML_PARAM_LIST, syn_fml_param_list, fml_param_list) \
  FLD(SYN_RPAREN, syn_tok, rparen)

#define DEF_FLD_SYN_FML_PARAM_SEC(FLD) \
  FLD(SYN_IDENT_LIST, syn_ident_list, ident_list) \
  FLD(SYN_COLON, syn_tok, colon) \
  FLD(ANY_SYN_TYPE, any_syn_type, type) \
  FLD(SYN_SEMI, syn_tok, semi)

#define DEF_OPT_ANY_SYN_TYPE(OPT) \
  OPT(SYN_INT_TYPE, syn_int_type, ANY_SYN_TYPE_INT) \
  OPT(SYN_BOOL_TYPE, syn_bool_type, ANY_SYN_TYPE_BOOL) \
  OPT(SYN_CHAR_TYPE, syn_char_type, ANY_SYN_TYPE_CHAR) \
  OPT(SYN_ARRAY_TYPE, syn_array_type, ANY_SYN_TYPE_ARRAY)

#define DEF_FLD_SYN_INT_TYPE(FLD) \
  FLD(SYN_INTEGER_KW, syn_tok, integer_kw)

#define DEF_FLD_SYN_BOOL_TYPE(FLD) \
  FLD(SYN_BOOLEAN_KW, syn_tok, boolean_kw)

#define DEF_FLD_SYN_CHAR_TYPE(FLD) \
  FLD(SYN_CHAR_KW, syn_tok, char_kw)

#define DEF_FLD_SYN_ARRAY_TYPE(FLD) \
  FLD(SYN_ARRAY_KW, syn_tok, array_kw) \
  FLD(SYN_LBRKT, syn_tok, lbrkt) \
  FLD(ANY_SYN_EXPR, any_syn_expr, size) \
  FLD(SYN_RBRKT, syn_tok, rbrkt) \
  FLD(SYN_OF_KW, syn_tok, of_kw) \
  FLD(ANY_SYN_TYPE, any_syn_type, type)

#define DEF_FLD_SYN_IDENT_LIST_ITEM(FLD) \
  FLD(SYN_IDENT, syn_tok, name) \
  FLD(SYN_COMMA, syn_tok, comma)

#define DEF_OPT_ANY_SYN_STMT(OPT) \
  OPT(SYN_ASSIGN_STMT, syn_assign_stmt, ANY_SYN_STMT_ASSIGN) \
  OPT(SYN_IF_STMT, syn_if_stmt, ANY_SYN_STMT_IF) \
  OPT(SYN_WHILE_STMT, syn_while_stmt, ANY_SYN_STMT_WHILE) \
  OPT(SYN_BREAK_STMT, syn_break_stmt, ANY_SYN_STMT_BREAK) \
  OPT(SYN_CALL_STMT, syn_call_stmt, ANY_SYN_STMT_CALL) \
  OPT(SYN_RETURN_STMT, syn_return_stmt, ANY_SYN_STMT_RETURN) \
  OPT(SYN_INPUT_STMT, syn_input_stmt, ANY_SYN_STMT_INPUT) \
  OPT(SYN_OUTPUT_STMT, syn_output_stmt, ANY_SYN_STMT_OUTPUT) \
  OPT(SYN_COMP_STMT, syn_comp_stmt, ANY_SYN_STMT_COMP) \
  OPT(SYN_EMPTY_STMT, syn_empty_stmt, ANY_SYN_STMT_EMPTY)

#define DEF_FLD_SYN_ASSIGN_STMT(FLD) \
  FLD(ANY_SYN_EXPR, any_syn_expr, lhs) \
  FLD(SYN_ASSIGN_OP, syn_tok, op) \
  FLD(ANY_SYN_EXPR, any_syn_expr, rhs)

#define DEF_FLD_SYN_IF_STMT(FLD) \
  FLD(SYN_IF_KW, syn_tok, if_kw) \
  FLD(ANY_SYN_EXPR, any_syn_expr, cond) \
  FLD(SYN_THEN_KW, syn_tok, then_kw) \
  FLD(ANY_SYN_STMT, any_syn_stmt, then_stmt) \
  FLD(SYN_ELSE_CLAUSE, syn_else_clause, else_clause)

#define DEF_FLD_SYN_ELSE_CLAUSE(FLD) \
  FLD(SYN_ELSE_KW, syn_tok, else_kw) \
  FLD(ANY_SYN_STMT, any_syn_stmt, else_stmt)

#define DEF_FLD_SYN_WHILE_STMT(FLD) \
  FLD(SYN_WHILE_KW, syn_tok, while_kw) \
  FLD(ANY_SYN_EXPR, any_syn_expr, cond) \
  FLD(SYN_DO_KW, syn_tok, do_kw) \
  FLD(ANY_SYN_STMT, any_syn_stmt, do_stmt)

#define DEF_FLD_SYN_BREAK_STMT(FLD) \
  FLD(SYN_BREAK_KW, syn_tok, break_kw)

#define DEF_FLD_SYN_CALL_STMT(FLD) \
  FLD(SYN_CALL_KW, syn_tok, call_kw) \
  FLD(SYN_IDENT, syn_tok, name) \
  FLD(SYN_ACT_PARAMS, syn_act_params, act_params)

#define DEF_FLD_SYN_ACT_PARAMS(FLD) \
  FLD(SYN_LPAREN, syn_tok, lparen) \
  FLD(SYN_EXPR_LIST, syn_expr_list, expr_list) \
  FLD(SYN_RPAREN, syn_tok, rparen)

#define DEF_FLD_SYN_EXPR_LIST_ITEM(FLD) \
  FLD(ANY_SYN_EXPR, any_syn_expr, expr) \
  FLD(SYN_COMMA, syn_tok, comma)

#define DEF_FLD_SYN_RETURN_STMT(FLD) \
  FLD(SYN_RETURN_KW, syn_tok, return_kw)

#define DEF_FLD_SYN_INPUT_STMT(FLD) \
  FLD(SYN_READ_KW, syn_tok, read_kw) \
  FLD(SYN_ACT_PARAMS, syn_act_params, act_params)

#define DEF_FLD_SYN_OUTPUT_STMT(FLD) \
  FLD(SYN_WRITE_KW, syn_tok, write_kw) \
  FLD(SYN_OUTPUT_VALUES, syn_output_values, output_values)

#define DEF_FLD_SYN_OUTPUT_VALUES(FLD) \
  FLD(SYN_LPAREN, syn_tok, lparen) \
  FLD(SYN_OUTPUT_VALUE_LIST, syn_output_value_list, output_value_list) \
  FLD(SYN_RPAREN, syn_tok, rparen)

#define DEF_FLD_SYN_OUTPUT_VALUE_LIST_ITEM(FLD) \
  FLD(SYN_OUTPUT_VALUE, syn_output_value, output_value) \
  FLD(SYN_COMMA, syn_tok, comma)

#define DEF_FLD_SYN_OUTPUT_VALUE(FLD) \
  FLD(ANY_SYN_EXPR, any_syn_expr, expr) \
  FLD(SYN_OUTPUT_FMT, syn_output_fmt, output_fmt)

#define DEF_FLD_SYN_OUTPUT_FMT(FLD) \
  FLD(SYN_COLON, syn_tok, colon) \
  FLD(ANY_SYN_EXPR, any_syn_expr, width)

#define DEF_FLD_SYN_COMP_STMT(FLD) \
  FLD(SYN_BEGIN_KW, syn_tok, begin_kw) \
  FLD(SYN_STMT_LIST, syn_stmt_list, stmt_list) \
  FLD(SYN_END_KW, syn_tok, end_kw)

#define DEF_FLD_SYN_STMT_LIST_ITEM(FLD) \
  FLD(ANY_SYN_STMT, any_syn_stmt, stmt) \
  FLD(SYN_SEMI, syn_tok, semi)

#define DEF_FLD_SYN_EMPTY_STMT(FLD) \
  /* empty */

#define DEF_OPT_ANY_SYN_EXPR(OPT) \
  OPT(SYN_ENTIRE_VAR_EXPR, syn_entire_var_expr, ANY_SYN_EXPR_ENTIRE_VAR) \
  OPT(SYN_IDX_VAR_EXPR, syn_idx_var_expr, ANY_SYN_EXPR_IDX_VAR) \
  OPT(SYN_INT_LIT_EXPR, syn_int_lit_expr, ANY_SYN_EXPR_INT_LIT) \
  OPT(SYN_BOOL_LIT_EXPR, syn_bool_lit_expr, ANY_SYN_EXPR_BOOL_LIT) \
  OPT(SYN_STR_LIT_EXPR, syn_str_lit_expr, ANY_SYN_EXPR_STR_LIT) \
  OPT(SYN_PAREN_EXPR, syn_paren_expr, ANY_SYN_EXPR_PAREN) \
  OPT(SYN_CAST_EXPR, syn_cast_expr, ANY_SYN_EXPR_CAST) \
  OPT(SYN_UNARY_EXPR, syn_unary_expr, ANY_SYN_EXPR_UNARY) \
  OPT(SYN_BINARY_EXPR, syn_binary_expr, ANY_SYN_EXPR_BINARY)

#define DEF_FLD_SYN_ENTIRE_VAR_EXPR(FLD) \
  FLD(SYN_IDENT, syn_tok, name)

#define DEF_FLD_SYN_IDX_VAR_EXPR(FLD) \
  FLD(SYN_IDENT, syn_tok, name) \
  FLD(SYN_LBRKT, syn_tok, lbrkt) \
  FLD(ANY_SYN_EXPR, any_syn_expr, index) \
  FLD(SYN_RBRKT, syn_tok, rbrkt)

#define DEF_FLD_SYN_INT_LIT_EXPR(FLD) \
  FLD(SYN_NUMBER, syn_tok, int_lit)

#define DEF_FLD_SYN_BOOL_LIT_EXPR(FLD) \
  FLD(ANY_SYN_BOOL_LIT, any_syn_bool_lit, bool_lit)

#define DEF_OPT_ANY_SYN_BOOL_LIT(OPT) \
  OPT(SYN_TRUE_KW, syn_true_lit, ANY_SYN_BOOL_LIT_TRUE) \
  OPT(SYN_FALSE_KW, syn_false_lit, ANY_SYN_BOOL_LIT_FALSE)

#define DEF_FLD_SYN_STR_LIT_EXPR(FLD) \
  FLD(SYN_STR, syn_tok, str_lit)

#define DEF_FLD_SYN_PAREN_EXPR(FLD) \
  FLD(SYN_LPAREN, syn_tok, lparen) \
  FLD(ANY_SYN_EXPR, any_syn_expr, expr) \
  FLD(SYN_RPAREN, syn_tok, rparen)

#define DEF_FLD_SYN_CAST_EXPR(FLD) \
  FLD(ANY_SYN_TYPE, any_syn_type, type) \
  FLD(SYN_LPAREN, syn_tok, lparen) \
  FLD(ANY_SYN_EXPR, any_syn_expr, expr) \
  FLD(SYN_RPAREN, syn_tok, rparen)

#define DEF_FLD_SYN_UNARY_EXPR(FLD) \
  FLD(ANY_SYN_UNARY_OP, any_syn_unary_op, op) \
  FLD(ANY_SYN_EXPR, any_syn_expr, expr)

#define DEF_OPT_ANY_SYN_UNARY_OP(OPT) \
  OPT(SYN_PLUS, syn_tok, ANY_SYN_UNARY_OP_PLUS) \
  OPT(SYN_MINUS, syn_tok, ANY_SYN_UNARY_OP_MINUS) \
  OPT(SYN_NOT_KW, syn_tok, ANY_SYN_UNARY_OP_NOT)

#define DEF_FLD_SYN_BINARY_EXPR(FLD) \
  FLD(ANY_SYN_EXPR, any_syn_expr, lhs) \
  FLD(ANY_SYN_BINARY_OP, any_syn_binary_op, op) \
  FLD(ANY_SYN_EXPR, any_syn_expr, rhs)

#define DEF_OPT_ANY_SYN_BINARY_OP(OPT) \
  OPT(SYN_PLUS, syn_tok, ANY_SYN_BINARY_OP_PLUS) \
  OPT(SYN_MINUS, syn_tok, ANY_SYN_BINARY_OP_MINUS) \
  OPT(SYN_STAR, syn_tok, ANY_SYN_BINARY_OP_STAR) \
  OPT(SYN_EQ, syn_tok, ANY_SYN_BINARY_OP_EQUAL) \
  OPT(SYN_NEQ, syn_tok, ANY_SYN_BINARY_OP_NOTEQ) \
  OPT(SYN_LT, syn_tok, ANY_SYN_BINARY_OP_LE) \
  OPT(SYN_LTEQ, syn_tok, ANY_SYN_BINARY_OP_LEEQ) \
  OPT(SYN_GT, syn_tok, ANY_SYN_BINARY_OP_GR) \
  OPT(SYN_GTEQ, syn_tok, ANY_SYN_BINARY_OP_GREQ)

#define DEF_SYN(TOK, SEQ, ALT, REP) \
  TOK(SYN_ERROR, NULL) \
  TOK(SYN_IDENT, NULL) \
  TOK(SYN_PROGRAM_KW, "program") \
  TOK(SYN_VAR_KW, "var") \
  TOK(SYN_ARRAY_KW, "array") \
  TOK(SYN_OF_KW, "of") \
  TOK(SYN_BEGIN_KW, "begin") \
  TOK(SYN_END_KW, "end") \
  TOK(SYN_IF_KW, "if") \
  TOK(SYN_THEN_KW, "then") \
  TOK(SYN_ELSE_KW, "else") \
  TOK(SYN_PROCEDURE_KW, "procedure") \
  TOK(SYN_RETURN_KW, "return") \
  TOK(SYN_CALL_KW, "call") \
  TOK(SYN_WHILE_KW, "while") \
  TOK(SYN_DO_KW, "do") \
  TOK(SYN_NOT_KW, "not") \
  TOK(SYN_OR_KW, "or") \
  TOK(SYN_DIV_KW, "div") \
  TOK(SYN_AND_KW, "and") \
  TOK(SYN_CHAR_KW, "char") \
  TOK(SYN_INTEGER_KW, "integer") \
  TOK(SYN_BOOLEAN_KW, "boolean") \
  TOK(SYN_READ_KW, "read") \
  TOK(SYN_WRITE_KW, "write") \
  TOK(SYN_READLN_KW, "readln") \
  TOK(SYN_WRITELN_KW, "writeln") \
  TOK(SYN_TRUE_KW, "true") \
  TOK(SYN_FALSE_KW, "false") \
  TOK(SYN_BREAK_KW, "break") \
  TOK(SYN_NUMBER_LIT, NULL) \
  TOK(SYN_STRING_LIT, NULL) \
  TOK(SYN_PLUS, "+") \
  TOK(SYN_MINUS, "-") \
  TOK(SYN_STAR, "*") \
  TOK(SYN_EQ, "=") \
  TOK(SYN_NEQ, "<>") \
  TOK(SYN_LT, "<") \
  TOK(SYN_LTEQ, "<=") \
  TOK(SYN_GT, ">") \
  TOK(SYN_GTEQ, ">=") \
  TOK(SYN_LPAREN, "(") \
  TOK(SYN_RPAREN, ")") \
  TOK(SYN_LBRACE, "[") \
  TOK(SYN_RBRACE, "]") \
  TOK(SYN_ASSIGN, ":=") \
  TOK(SYN_DOT, ".") \
  TOK(SYN_COMMA, ",") \
  TOK(SYN_COLON, ":") \
  TOK(SYN_SEMI, ";") \
  TOK(SYN_EOF, NULL) \
  TOK(SYN_NEWLINE, NULL) \
  TOK(SYN_WHITESPACE, NULL) \
  TOK(SYN_BRACKET_COMMENT, NULL) \
  TOK(SYN_SLASH_STAR_COMMENT, NULL) \
  SEQ(SYN_PROGRAM, syn_program, DEF_FLD_SYN_PROGRAM) \
  SEQ(SYN_BLOCK, syn_block, DEF_FLD_SYN_BLOCK) \
  REP(SYN_DECL_PART_LIST, syn_decl_part_list, any_syn_decl_part) \
  ALT(ANY_SYN_DECL_PART, any_syn_decl_part, DEF_OPT_ANY_SYN_DECL_PART) \
  SEQ(SYN_VAR_DECL_PART, syn_var_decl_part, DEF_FLD_SYN_VAR_DECL_PART) \
  REP(SYN_VAR_DECL_LIST, syn_var_decl_list, syn_var_decl) \
  SEQ(SYN_VAR_DECL, syn_var_decl, DEF_FLD_SYN_VAR_DECL) \
  SEQ(SYN_PROC_DECL_PART, syn_proc_decl_part, DEF_FLD_SYN_PROC_DECL_PART) \
  SEQ(SYN_PROC_DECL_HEAD, syn_proc_decl_head, DEF_FLD_SYN_PROC_DECL_HEAD) \
  SEQ(SYN_FML_PARAMS, syn_fml_params, DEF_FLD_SYN_FML_PARAMS) \
  REP(SYN_FML_PARAM_LIST, syn_fml_param_list, syn_fml_param_sec) \
  SEQ(SYN_FML_PARAM_SEC, syn_fml_param_sec, DEF_FLD_SYN_FML_PARAM_SEC) \
  ALT(ANY_SYN_TYPE, any_syn_type, DEF_OPT_ANY_SYN_TYPE) \
  SEQ(SYN_INT_TYPE, syn_int_type, DEF_FLD_SYN_INT_TYPE) \
  SEQ(SYN_BOOL_TYPE, syn_bool_type, DEF_FLD_SYN_BOOL_TYPE) \
  SEQ(SYN_CHAR_TYPE, syn_char_type, DEF_FLD_SYN_CHAR_TYPE) \
  SEQ(SYN_ARRAY_TYPE, syn_array_type, DEF_FLD_SYN_ARRAY_TYPE) \
  REP(SYN_IDENT_LIST, syn_ident_list, syn_ident_list_item) \
  SEQ(SYN_IDENT_LIST_ITEM, syn_ident_list_item, DEF_FLD_SYN_IDENT_LIST_ITEM) \
  ALT(ANY_SYN_STMT, any_syn_stmt, DEF_OPT_ANY_SYN_STMT) \
  SEQ(SYN_ASSIGN_STMT, syn_assign_stmt, DEF_FLD_SYN_ASSIGN_STMT) \
  SEQ(SYN_IF_STMT, syn_if_stmt, DEF_FLD_SYN_IF_STMT) \
  SEQ(SYN_ELSE_CLAUSE, syn_else_clause, DEF_FLD_SYN_ELSE_CLAUSE) \
  SEQ(SYN_WHILE_STMT, syn_while_stmt, DEF_FLD_SYN_WHILE_STMT) \
  SEQ(SYN_BREAK_STMT, syn_break_stmt, DEF_FLD_SYN_BREAK_STMT) \
  SEQ(SYN_CALL_STMT, syn_call_stmt, DEF_FLD_SYN_CALL_STMT) \
  SEQ(SYN_ACT_PARAMS, syn_act_params, DEF_FLD_SYN_ACT_PARAMS) \
  REP(SYN_EXPR_LIST, syn_expr_list, syn_expr_list_item) \
  SEQ(SYN_EXPR_LIST_ITEM, syn_expr_list_item, DEF_FLD_SYN_EXPR_LIST_ITEM) \
  SEQ(SYN_RETURN_STMT, syn_return_stmt, DEF_FLD_SYN_RETURN_STMT) \
  SEQ(SYN_INPUT_STMT, syn_input_stmt, DEF_FLD_SYN_INPUT_STMT) \
  SEQ(SYN_OUTPUT_STMT, syn_output_stmt, DEF_FLD_SYN_OUTPUT_STMT) \
  SEQ(SYN_OUTPUT_VALUES, syn_output_values, DEF_FLD_SYN_OUTPUT_VALUES) \
  REP(SYN_OUTPUT_VALUE_LIST, syn_output_value_list, syn_output_value_list_item) \
  SEQ(SYN_OUTPUT_VALUE_LIST_ITEM, syn_output_value_list_item, DEF_FLD_SYN_OUTPUT_VALUE_LIST_ITEM) \
  SEQ(SYN_OUTPUT_VALUE, syn_output_value, DEF_FLD_SYN_OUTPUT_VALUE) \
  SEQ(SYN_OUTPUT_FMT, syn_output_fmt, DEF_FLD_SYN_OUTPUT_FMT) \
  SEQ(SYN_COMP_STMT, syn_comp_stmt, DEF_FLD_SYN_COMP_STMT) \
  REP(SYN_STMT_LIST, syn_stmt_list, syn_stmt_list_item) \
  SEQ(SYN_STMT_LIST_ITEM, syn_stmt_list_item, DEF_FLD_SYN_STMT_LIST_ITEM) \
  SEQ(SYN_EMPTY_STMT, syn_empty_stmt, DEF_FLD_SYN_EMPTY_STMT) \
  ALT(ANY_SYN_EXPR, any_syn_expr, DEF_OPT_ANY_SYN_EXPR) \
  SEQ(SYN_ENTIRE_VAR_EXPR, syn_entire_var_expr, DEF_FLD_SYN_ENTIRE_VAR_EXPR) \
  SEQ(SYN_IDX_VAR_EXPR, syn_idx_var_expr, DEF_FLD_SYN_IDX_VAR_EXPR) \
  SEQ(SYN_INT_LIT_EXPR, syn_int_lit_expr, DEF_FLD_SYN_INT_LIT_EXPR) \
  SEQ(SYN_BOOL_LIT_EXPR, syn_bool_lit_expr, DEF_FLD_SYN_BOOL_LIT_EXPR) \
  ALT(ANY_SYN_BOOL_LIT, any_syn_bool_lit, DEF_OPT_ANY_SYN_BOOL_LIT) \
  SEQ(SYN_STR_LIT_EXPR, syn_str_lit_expr, DEF_FLD_SYN_STR_LIT_EXPR) \
  SEQ(SYN_PAREN_EXPR, syn_paren_expr, DEF_FLD_SYN_PAREN_EXPR) \
  SEQ(SYN_CAST_EXPR, syn_cast_expr, DEF_FLD_SYN_CAST_EXPR) \
  SEQ(SYN_UNARY_EXPR, syn_unary_expr, DEF_FLD_SYN_UNARY_EXPR) \
  ALT(ANY_SYN_UNARY_OP, any_syn_unary_op, DEF_OPT_ANY_SYN_UNARY_OP) \
  SEQ(SYN_BINARY_EXPR, syn_binary_expr, DEF_FLD_SYN_BINARY_EXPR) \
  ALT(ANY_SYN_BINARY_OP, any_syn_binary_op, DEF_OPT_ANY_SYN_BINARY_OP)

#define KW_BEGIN SYN_PROGRAM_KW
#define KW_END SYN_BREAK_KW

#define NONTRIV_BEGIN SYN_IDENT
#define NONTRIV_END SYN_SEMI

#define TRIV_BEGIN SYN_NEWLINE
#define TRIV_END SYN_SLASH_STAR_COMMENT

#define TOK_BEGIN SYN_ERROR
#define TOK_END SYN_SLASH_STAR_COMMENT

#define TOK(KIND, LEXEME) KIND,
#define SEQ(KIND, TYPE, FIELDS) KIND,
#define ALT(KIND, TYPE, OPTIONS)
#define REP(KIND, TYPE, ITEM) KIND,

enum syn_kind {
  DEF_SYN(TOK, SEQ, ALT, REP)
  SIZE_SYN
};

#undef TOK
#undef SEQ
#undef ALT
#undef REP

char const *syn_kind_to_lexeme(enum syn_kind kind);

char const *syn_kind_to_string(enum syn_kind kind);

struct syn_node {
  enum syn_kind kind;
  size_t index;
  struct syn_node const *parent;
};

struct syn_tree {
  struct syn_node node;
  size_t *offsets;
};

struct syn_tok {
  struct syn_node node;
  char const *text;
  size_t len;
  struct syn_triv *triv;
};

struct syn_triv {
  struct syn_triv_piece *pieces;
  size_t *offsets;
  size_t count;
};

struct syn_triv_piece {
  enum syn_kind kind;
  char const *text;
};

#define TOK(KIND, LEXEME)

#define FLD(KIND, TYPE, NAME) \
  struct TYPE *NAME;

#define SEQ(KIND, TYPE, FIELDS) \
  struct TYPE { \
    struct syn_tree syn; \
    FIELDS(FLD) \
  };

#define OPT(KIND, TYPE, ENUM) \
  ENUM,

#define ALT(KIND, TYPE, OPTIONS) \
  struct TYPE; \
  \
  enum TYPE ## _kind { \
    OPTIONS(OPT) \
    SIZE_ ## KIND \
  }; \
  \
  enum TYPE ## _kind TYPE ## _kind(struct TYPE const *n);

#define REP(KIND, TYPE, ITEM) \
  struct TYPE { \
    struct syn_tree syn; \
    struct ITEM **children; \
    size_t count; \
  };

DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef FLD
#undef SEQ
#undef OPT
#undef ALT
#undef REP

void syn_free(struct syn_node *node);

size_t syn_text_len(struct syn_node const *node);

struct syn_node *syn_child_at(struct syn_node const *node, size_t index);

size_t syn_child_count(struct syn_node const *node);

void syn_print(struct syn_node const *node, FILE *out);

typedef unsigned long syn_ckpt_t;

struct syn_bldr {
  vec(struct syn_triv_piece) trivs;
  vec(size_t) triv_lens;
  vec(struct syn_node *) stack;
};

void syn_bldr_init(struct syn_bldr *b);

void syn_bldr_deinit(struct syn_bldr *b);

void syn_bldr_triv(struct syn_bldr *b, enum syn_kind kind, char const *text, size_t len);

void syn_bldr_tok(struct syn_bldr *b, enum syn_kind kind, char const *text, size_t len);

void syn_bldr_empty(struct syn_bldr *b);

syn_ckpt_t syn_bldr_open(struct syn_bldr *b);

void syn_bldr_close(struct syn_bldr *b, enum syn_kind kind, syn_ckpt_t ckpt);

struct syn_node *syn_bldr_finish(struct syn_bldr *b);

#endif /* SYN_H */
