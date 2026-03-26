/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPL_SYNTAX_H
#define MPL_SYNTAX_H

#define DEF_FLD_MPL_PROGRAM(FLD) \
  FLD(MPL_PROGRAM_KW, syn_node, program_kw) \
  FLD(MPL_IDENT, syn_node, name) \
  FLD(MPL_SEMI, syn_node, semi) \
  FLD(MPL_BLOCK, syn_node, block) \
  FLD(MPL_DOT, syn_node, dot) \
  FLD(MPL_EOF, syn_node, eof)

#define DEF_FLD_MPL_BLOCK(FLD) \
  FLD(MPL_DECL_PART_LIST, mpl_decl_part_list, decl_part_list) \
  FLD(MPL_COMP_STMT, mpl_comp_stmt, comp_stmt)

#define DEF_FLD_ANY_MPL_DECL_PART(OPT) \
  OPT(MPL_VAR_DECL_PART, ANY_MPL_DECL_PART_VAR, mpl_var_decl_part) \
  OPT(MPL_PROC_DECL_PART, ANY_MPL_DECL_PART_PROC, mpl_proc_decl_part)

#define DEF_FLD_MPL_VAR_DECL_PART(FLD) \
  FLD(MPL_VAR_KW, syn_node, var_kw) \
  FLD(MPL_VAR_DECL_LIST, mpl_var_decl_list, var_decl_list)

#define DEF_FLD_MPL_VAR_DECL(FLD) \
  FLD(MPL_IDENT_LIST, mpl_ident_list, ident_list) \
  FLD(MPL_COLON, syn_node, colon) \
  FLD(ANY_MPL_TYPE, any_mpl_type, type) \
  FLD(MPL_SEMI, syn_node, semi)

#define DEF_FLD_MPL_PROC_DECL_PART(FLD) \
  FLD(MPL_PROC_DECL_HEAD, mpl_proc_decl_head, proc_decl_head) \
  FLD(MPL_VAR_DECL_PART, mpl_var_decl_part, var_decl_part) \
  FLD(MPL_COMP_STMT, mpl_comp_stmt, comp_stmt) \
  FLD(MPL_SEMI, syn_node, semi)

#define DEF_FLD_MPL_PROC_DECL_HEAD(FLD) \
  FLD(MPL_PROCEDURE_KW, syn_node, procedure_kw) \
  FLD(MPL_IDENT, syn_node, name) \
  FLD(MPL_FML_PARAMS, mpl_fml_params, fml_params) \
  FLD(MPL_SEMI, syn_node, semi)

#define DEF_FLD_MPL_FML_PARAMS(FLD) \
  FLD(MPL_LPAREN, syn_node, lparen) \
  FLD(MPL_FML_PARAM_LIST, mpl_fml_param_list, fml_param_list) \
  FLD(MPL_RPAREN, syn_node, rparen)

#define DEF_FLD_MPL_FML_PARAM_SEC(FLD) \
  FLD(MPL_IDENT_LIST, mpl_ident_list, ident_list) \
  FLD(MPL_COLON, syn_node, colon) \
  FLD(ANY_MPL_TYPE, any_mpl_type, type) \
  FLD(MPL_SEMI, syn_node, semi)

#define DEF_FLD_ANY_MPL_TYPE(OPT) \
  OPT(MPL_INT_TYPE, mpl_int_type, ANY_MPL_TYPE_INT) \
  OPT(MPL_BOOL_TYPE, mpl_bool_type, ANY_MPL_TYPE_BOOL) \
  OPT(MPL_CHAR_TYPE, mpl_char_type, ANY_MPL_TYPE_CHAR) \
  OPT(MPL_ARRAY_TYPE, mpl_array_type, ANY_MPL_TYPE_ARRAY)

#define DEF_FLD_MPL_INT_TYPE(FLD) \
  FLD(MPL_INTEGER_KW, syn_node, integer_kw)

#define DEF_FLD_MPL_BOOL_TYPE(FLD) \
  FLD(MPL_BOOLEAN_KW, syn_node, boolean_kw)

#define DEF_FLD_MPL_CHAR_TYPE(FLD) \
  FLD(MPL_CHAR_KW, syn_node, char_kw)

#define DEF_FLD_MPL_ARRAY_TYPE(FLD) \
  FLD(MPL_ARRAY_KW, syn_node, array_kw) \
  FLD(MPL_LBRKT, syn_node, lbrkt) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, size) \
  FLD(MPL_RBRKT, syn_node, rbrkt) \
  FLD(MPL_OF_KW, syn_node, of_kw) \
  FLD(ANY_MPL_TYPE, any_mpl_type, type)

#define DEF_FLD_MPL_IDENT_LIST_ITEM(FLD) \
  FLD(MPL_IDENT, syn_node, name) \
  FLD(MPL_COMMA, syn_node, comma)

#define DEF_FLD_ANY_MPL_STMT(OPT) \
  OPT(MPL_ASSIGN_STMT, mpl_assign_stmt, ANY_MPL_STMT_ASSIGN) \
  OPT(MPL_IF_STMT, mpl_if_stmt, ANY_MPL_STMT_IF) \
  OPT(MPL_WHILE_STMT, mpl_while_stmt, ANY_MPL_STMT_WHILE) \
  OPT(MPL_BREAK_STMT, mpl_break_stmt, ANY_MPL_STMT_BREAK) \
  OPT(MPL_CALL_STMT, mpl_call_stmt, ANY_MPL_STMT_CALL) \
  OPT(MPL_RETURN_STMT, mpl_return_stmt, ANY_MPL_STMT_RETURN) \
  OPT(MPL_INPUT_STMT, mpl_input_stmt, ANY_MPL_STMT_INPUT) \
  OPT(MPL_OUTPUT_STMT, mpl_output_stmt, ANY_MPL_STMT_OUTPUT) \
  OPT(MPL_COMP_STMT, mpl_comp_stmt, ANY_MPL_STMT_COMP) \
  OPT(MPL_EMPTY_STMT, mpl_empty_stmt, ANY_MPL_STMT_EMPTY)

#define DEF_FLD_MPL_ASSIGN_STMT(FLD) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, lhs) \
  FLD(MPL_ASSIGN_OP, syn_node, op) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, rhs)

#define DEF_FLD_MPL_IF_STMT(FLD) \
  FLD(MPL_IF_KW, syn_node, if_kw) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, cond) \
  FLD(MPL_THEN_KW, syn_node, then_kw) \
  FLD(ANY_MPL_STMT, any_mpl_stmt, then_stmt) \
  FLD(MPL_ELSE_CLAUSE, mpl_else_clause, else_clause)

#define DEF_FLD_MPL_ELSE_CLAUSE(FLD) \
  FLD(MPL_ELSE_KW, syn_node, else_kw) \
  FLD(ANY_MPL_STMT, any_mpl_stmt, else_stmt)

#define DEF_FLD_MPL_WHILE_STMT(FLD) \
  FLD(MPL_WHILE_KW, syn_node, while_kw) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, cond) \
  FLD(MPL_DO_KW, syn_node, do_kw) \
  FLD(ANY_MPL_STMT, any_mpl_stmt, do_stmt)

#define DEF_FLD_MPL_BREAK_STMT(FLD) \
  FLD(MPL_BREAK_KW, syn_node, break_kw)

#define DEF_FLD_MPL_CALL_STMT(FLD) \
  FLD(MPL_CALL_KW, syn_node, call_kw) \
  FLD(MPL_IDENT, syn_node, name) \
  FLD(MPL_ACT_PARAMS, mpl_act_params, act_params)

#define DEF_FLD_MPL_ACT_PARAMS(FLD) \
  FLD(MPL_LPAREN, syn_node, lparen) \
  FLD(MPL_EXPR_LIST, mpl_expr_list, expr_list) \
  FLD(MPL_RPAREN, syn_node, rparen)

#define DEF_FLD_MPL_EXPR_LIST_ITEM(FLD) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, expr) \
  FLD(MPL_COMMA, syn_node, comma)

#define DEF_FLD_MPL_RETURN_STMT(FLD) \
  FLD(MPL_RETURN_KW, syn_node, return_kw)

#define DEF_FLD_MPL_INPUT_STMT(FLD) \
  FLD(MPL_READ_KW, syn_node, read_kw) \
  FLD(MPL_ACT_PARAMS, mpl_act_params, act_params)

#define DEF_FLD_MPL_OUTPUT_STMT(FLD) \
  FLD(MPL_WRITE_KW, syn_node, write_kw) \
  FLD(MPL_OUTPUT_VALUES, mpl_output_values, output_values)

#define DEF_FLD_MPL_OUTPUT_VALUES(FLD) \
  FLD(MPL_LPAREN, syn_node, lparen) \
  FLD(MPL_OUTPUT_VALUE_LIST, mpl_output_value_list, output_value_list) \
  FLD(MPL_RPAREN, syn_node, rparen)

#define DEF_FLD_MPL_OUTPUT_VALUE_LIST_ITEM(FLD) \
  FLD(MPL_OUTPUT_VALUE, mpl_output_value, output_value) \
  FLD(MPL_COMMA, syn_node, comma)

#define DEF_FLD_MPL_OUTPUT_VALUE(FLD) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, expr) \
  FLD(MPL_OUTPUT_FMT, mpl_output_fmt, output_fmt)

#define DEF_FLD_MPL_OUTPUT_FMT(FLD) \
  FLD(MPL_COLON, syn_node, colon) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, width)

#define DEF_FLD_MPL_COMP_STMT(FLD) \
  FLD(MPL_BEGIN_KW, syn_node, begin_kw) \
  FLD(MPL_STMT_LIST, mpl_stmt_list, stmt_list) \
  FLD(MPL_END_KW, syn_node, end_kw)

#define DEF_FLD_MPL_STMT_LIST_ITEM(FLD) \
  FLD(ANY_MPL_STMT, any_mpl_stmt, stmt) \
  FLD(MPL_SEMI, syn_node, semi)

#define DEF_FLD_MPL_EMPTY_STMT(FLD) \
  /* empty */

#define DEF_FLD_ANY_MPL_EXPR(OPT) \
  OPT(MPL_ENTIRE_VAR_EXPR, mpl_entire_var_expr, ANY_MPL_EXPR_ENTIRE_VAR) \
  OPT(MPL_IDX_VAR_EXPR, mpl_idx_var_expr, ANY_MPL_EXPR_IDX_VAR) \
  OPT(MPL_INT_LIT_EXPR, mpl_int_lit_expr, ANY_MPL_EXPR_INT_LIT) \
  OPT(MPL_BOOL_LIT_EXPR, mpl_bool_lit_expr, ANY_MPL_EXPR_BOOL_LIT) \
  OPT(MPL_STR_LIT_EXPR, mpl_str_lit_expr, ANY_MPL_EXPR_STR_LIT) \
  OPT(MPL_PAREN_EXPR, mpl_paren_expr, ANY_MPL_EXPR_PAREN) \
  OPT(MPL_CAST_EXPR, mpl_cast_expr, ANY_MPL_EXPR_CAST) \
  OPT(MPL_UNARY_EXPR, mpl_unary_expr, ANY_MPL_EXPR_UNARY) \
  OPT(MPL_BINARY_EXPR, mpl_binary_expr, ANY_MPL_EXPR_BINARY)

#define DEF_FLD_MPL_ENTIRE_VAR_EXPR(FLD) \
  FLD(MPL_IDENT, syn_node, name)

#define DEF_FLD_MPL_IDX_VAR_EXPR(FLD) \
  FLD(MPL_IDENT, syn_node, name) \
  FLD(MPL_LBRKT, syn_node, lbrkt) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, index) \
  FLD(MPL_RBRKT, syn_node, rbrkt)

#define DEF_FLD_MPL_INT_LIT_EXPR(FLD) \
  FLD(MPL_NUMBER, syn_node, int_lit)

#define DEF_FLD_MPL_BOOL_LIT_EXPR(FLD) \
  FLD(ANY_MPL_BOOL_LIT, any_mpl_bool_lit, bool_lit)

#define DEF_FLD_ANY_MPL_BOOL_LIT(OPT) \
  OPT(MPL_TRUE_KW, mpl_true_lit, ANY_MPL_BOOL_LIT_TRUE) \
  OPT(MPL_FALSE_KW, mpl_false_lit, ANY_MPL_BOOL_LIT_FALSE)

#define DEF_FLD_MPL_STR_LIT_EXPR(FLD) \
  FLD(MPL_STR, syn_node, str_lit)

#define DEF_FLD_MPL_PAREN_EXPR(FLD) \
  FLD(MPL_LPAREN, syn_node, lparen) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, expr) \
  FLD(MPL_RPAREN, syn_node, rparen)

#define DEF_FLD_MPL_CAST_EXPR(FLD) \
  FLD(ANY_MPL_TYPE, any_mpl_type, type) \
  FLD(MPL_LPAREN, syn_node, lparen) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, expr) \
  FLD(MPL_RPAREN, syn_node, rparen)

#define DEF_FLD_MPL_UNARY_EXPR(FLD) \
  FLD(ANY_MPL_UNARY_OP, any_mpl_unary_op, op) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, expr)

#define DEF_FLD_ANY_MPL_UNARY_OP(OPT) \
  OPT(MPL_PLUS, syn_node, ANY_MPL_UNARY_OP_PLUS) \
  OPT(MPL_MINUS, syn_node, ANY_MPL_UNARY_OP_MINUS) \
  OPT(MPL_NOT_KW, syn_node, ANY_MPL_UNARY_OP_NOT)

#define DEF_FLD_MPL_BINARY_EXPR(FLD) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, lhs) \
  FLD(ANY_MPL_BINARY_OP, any_mpl_binary_op, op) \
  FLD(ANY_MPL_EXPR, any_mpl_expr, rhs)

#define DEF_FLD_ANY_MPL_BINARY_OP(OPT) \
  OPT(MPL_PLUS, syn_node, ANY_MPL_BINARY_OP_PLUS) \
  OPT(MPL_MINUS, syn_node, ANY_MPL_BINARY_OP_MINUS) \
  OPT(MPL_STAR, syn_node, ANY_MPL_BINARY_OP_STAR) \
  OPT(MPL_EQUAL, syn_node, ANY_MPL_BINARY_OP_EQUAL) \
  OPT(MPL_NOTEQ, syn_node, ANY_MPL_BINARY_OP_NOTEQ) \
  OPT(MPL_LE, syn_node, ANY_MPL_BINARY_OP_LE) \
  OPT(MPL_LEEQ, syn_node, ANY_MPL_BINARY_OP_LEEQ) \
  OPT(MPL_GR, syn_node, ANY_MPL_BINARY_OP_GR) \
  OPT(MPL_GREQ, syn_node, ANY_MPL_BINARY_OP_GREQ)

#define DEF_SYN_MPL(TOK, SEQ, ALT, REP) \
  TOK(MPL_ERROR) \
  TOK(MPL_EOF) \
  TOK(MPL_IDENT) \
  TOK(MPL_NUMBER) \
  TOK(MPL_STR) \
  TOK(MPL_PLUS) \
  TOK(MPL_MINUS) \
  TOK(MPL_STAR) \
  TOK(MPL_EQUAL) \
  TOK(MPL_NOTEQ) \
  TOK(MPL_LE) \
  TOK(MPL_LEEQ) \
  TOK(MPL_GR) \
  TOK(MPL_GREQ) \
  TOK(MPL_LPAREN) \
  TOK(MPL_RPAREN) \
  TOK(MPL_LBRKT) \
  TOK(MPL_RBRKT) \
  TOK(MPL_ASSIGN) \
  TOK(MPL_DOT) \
  TOK(MPL_COMMA) \
  TOK(MPL_COLON) \
  TOK(MPL_SEMI) \
  TOK(MPL_PROGRAM_KW) \
  TOK(MPL_VAR_KW) \
  TOK(MPL_PROCEDURE_KW) \
  TOK(MPL_INTEGER_KW) \
  TOK(MPL_BOOLEAN_KW) \
  TOK(MPL_CHAR_KW) \
  TOK(MPL_ARRAY_KW) \
  TOK(MPL_OF_KW) \
  TOK(MPL_IF_KW) \
  TOK(MPL_THEN_KW) \
  TOK(MPL_ELSE_KW) \
  TOK(MPL_WHILE_KW) \
  TOK(MPL_DO_KW) \
  TOK(MPL_BREAK_KW) \
  TOK(MPL_CALL_KW) \
  TOK(MPL_RETURN_KW) \
  TOK(MPL_READ_KW) \
  TOK(MPL_READLN_KW) \
  TOK(MPL_WRITE_KW) \
  TOK(MPL_WRITELN_KW) \
  TOK(MPL_BEGIN_KW) \
  TOK(MPL_END_KW) \
  TOK(MPL_NOT_KW) \
  TOK(MPL_OR_KW) \
  TOK(MPL_AND_KW) \
  TOK(MPL_DIV_KW) \
  TOK(MPL_TRUE_KW) \
  TOK(MPL_FALSE_KW) \
  TOK(MPL_SPACE_TRIVIA) \
  TOK(MPL_BRACES_COMMENT_TRIVIA) \
  TOK(MPL_C_COMMENT_TRIVIA) \
  SEQ(MPL_PROGRAM, mpl_program, DEF_FLD_MPL_PROGRAM) \
  SEQ(MPL_BLOCK, mpl_block, DEF_FLD_MPL_BLOCK) \
  REP(MPL_DECL_PART_LIST, mpl_decl_part_list, any_mpl_decl_part) \
  ALT(ANY_MPL_DECL_PART, any_mpl_decl_part, DEF_FLD_ANY_MPL_DECL_PART) \
  SEQ(MPL_VAR_DECL_PART, mpl_var_decl_part, DEF_FLD_MPL_VAR_DECL_PART) \
  REP(MPL_VAR_DECL_LIST, mpl_var_decl_list, mpl_var_decl) \
  SEQ(MPL_VAR_DECL, mpl_var_decl, DEF_FLD_MPL_VAR_DECL) \
  SEQ(MPL_PROC_DECL_PART, mpl_proc_decl_part, DEF_FLD_MPL_PROC_DECL_PART) \
  SEQ(MPL_PROC_DECL_HEAD, mpl_proc_decl_head, DEF_FLD_MPL_PROC_DECL_HEAD) \
  SEQ(MPL_FML_PARAMS, mpl_fml_params, DEF_FLD_MPL_FML_PARAMS) \
  REP(MPL_FML_PARAM_LIST, mpl_fml_param_list, mpl_fml_param_sec) \
  SEQ(MPL_FML_PARAM_SEC, mpl_fml_param_sec, DEF_FLD_MPL_FML_PARAM_SEC) \
  ALT(ANY_MPL_TYPE, any_mpl_type, DEF_FLD_ANY_MPL_TYPE) \
  SEQ(MPL_INT_TYPE, mpl_int_type, DEF_FLD_MPL_INT_TYPE) \
  SEQ(MPL_BOOL_TYPE, mpl_bool_type, DEF_FLD_MPL_BOOL_TYPE) \
  SEQ(MPL_CHAR_TYPE, mpl_char_type, DEF_FLD_MPL_CHAR_TYPE) \
  SEQ(MPL_ARRAY_TYPE, mpl_array_type, DEF_FLD_MPL_ARRAY_TYPE) \
  REP(MPL_IDENT_LIST, mpl_ident_list, mpl_ident_list_item) \
  SEQ(MPL_IDENT_LIST_ITEM, mpl_ident_list_item, DEF_FLD_MPL_IDENT_LIST_ITEM) \
  ALT(ANY_MPL_STMT, any_mpl_stmt, DEF_FLD_ANY_MPL_STMT) \
  SEQ(MPL_ASSIGN_STMT, mpl_assign_stmt, DEF_FLD_MPL_ASSIGN_STMT) \
  SEQ(MPL_IF_STMT, mpl_if_stmt, DEF_FLD_MPL_IF_STMT) \
  SEQ(MPL_ELSE_CLAUSE, mpl_else_clause, DEF_FLD_MPL_ELSE_CLAUSE) \
  SEQ(MPL_WHILE_STMT, mpl_while_stmt, DEF_FLD_MPL_WHILE_STMT) \
  SEQ(MPL_BREAK_STMT, mpl_break_stmt, DEF_FLD_MPL_BREAK_STMT) \
  SEQ(MPL_CALL_STMT, mpl_call_stmt, DEF_FLD_MPL_CALL_STMT) \
  SEQ(MPL_ACT_PARAMS, mpl_act_params, DEF_FLD_MPL_ACT_PARAMS) \
  REP(MPL_EXPR_LIST, mpl_expr_list, mpl_expr_list_item) \
  SEQ(MPL_EXPR_LIST_ITEM, mpl_expr_list_item, DEF_FLD_MPL_EXPR_LIST_ITEM) \
  SEQ(MPL_RETURN_STMT, mpl_return_stmt, DEF_FLD_MPL_RETURN_STMT) \
  SEQ(MPL_INPUT_STMT, mpl_input_stmt, DEF_FLD_MPL_INPUT_STMT) \
  SEQ(MPL_OUTPUT_STMT, mpl_output_stmt, DEF_FLD_MPL_OUTPUT_STMT) \
  SEQ(MPL_OUTPUT_VALUES, mpl_output_values, DEF_FLD_MPL_OUTPUT_VALUES) \
  REP(MPL_OUTPUT_VALUE_LIST, mpl_output_value_list, mpl_output_value_list_item) \
  SEQ(MPL_OUTPUT_VALUE_LIST_ITEM, mpl_output_value_list_item, DEF_FLD_MPL_OUTPUT_VALUE_LIST_ITEM) \
  SEQ(MPL_OUTPUT_VALUE, mpl_output_value, DEF_FLD_MPL_OUTPUT_VALUE) \
  SEQ(MPL_OUTPUT_FMT, mpl_output_fmt, DEF_FLD_MPL_OUTPUT_FMT) \
  SEQ(MPL_COMP_STMT, mpl_comp_stmt, DEF_FLD_MPL_COMP_STMT) \
  REP(MPL_STMT_LIST, mpl_stmt_list, mpl_stmt_list_item) \
  SEQ(MPL_STMT_LIST_ITEM, mpl_stmt_list_item, DEF_FLD_MPL_STMT_LIST_ITEM) \
  SEQ(MPL_EMPTY_STMT, mpl_empty_stmt, DEF_FLD_MPL_EMPTY_STMT) \
  ALT(ANY_MPL_EXPR, any_mpl_expr, DEF_FLD_ANY_MPL_EXPR) \
  SEQ(MPL_ENTIRE_VAR_EXPR, mpl_entire_var_expr, DEF_FLD_MPL_ENTIRE_VAR_EXPR) \
  SEQ(MPL_IDX_VAR_EXPR, mpl_idx_var_expr, DEF_FLD_MPL_IDX_VAR_EXPR) \
  SEQ(MPL_INT_LIT_EXPR, mpl_int_lit_expr, DEF_FLD_MPL_INT_LIT_EXPR) \
  SEQ(MPL_BOOL_LIT_EXPR, mpl_bool_lit_expr, DEF_FLD_MPL_BOOL_LIT_EXPR) \
  ALT(ANY_MPL_BOOL_LIT, any_mpl_bool_lit, DEF_FLD_ANY_MPL_BOOL_LIT) \
  SEQ(MPL_STR_LIT_EXPR, mpl_str_lit_expr, DEF_FLD_MPL_STR_LIT_EXPR) \
  SEQ(MPL_PAREN_EXPR, mpl_paren_expr, DEF_FLD_MPL_PAREN_EXPR) \
  SEQ(MPL_CAST_EXPR, mpl_cast_expr, DEF_FLD_MPL_CAST_EXPR) \
  SEQ(MPL_UNARY_EXPR, mpl_unary_expr, DEF_FLD_MPL_UNARY_EXPR) \
  ALT(ANY_MPL_UNARY_OP, any_mpl_unary_op, DEF_FLD_ANY_MPL_UNARY_OP) \
  SEQ(MPL_BINARY_EXPR, mpl_binary_expr, DEF_FLD_MPL_BINARY_EXPR) \
  ALT(ANY_MPL_BINARY_OP, any_mpl_binary_op, DEF_FLD_ANY_MPL_BINARY_OP)

#define SYN_DEF DEF_SYN_MPL
#define SYN_PREFIX mpl

#include "syntax.h.inc"

#undef SYN_DEF
#undef SYN_PREFIX

#endif /* MPL_SYNTAX_H */
