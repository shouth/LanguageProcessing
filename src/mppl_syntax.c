/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include "mppl_syntax.h"
#include "syntax_tree.h"
#include "util.h"

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size)
{
  MpplSyntaxKind kind;

  for (kind = MPPL_BEGIN_KEYWORD; kind <= MPPL_END_KEYWORD; ++kind) {
    const char *lexeme = mppl_syntax_kind_static_lexeme(kind);
    if (strncmp(lexeme, string, size) == 0 && !lexeme[size]) {
      return kind;
    }
  }

  return MPPL_SYNTAX_ERROR;
}

const char *mppl_syntax_kind_static_lexeme(MpplSyntaxKind kind)
{
  switch (kind) {
  case MPPL_SYNTAX_PLUS_TOKEN:
    return "+";
  case MPPL_SYNTAX_MINUS_TOKEN:
    return "-";
  case MPPL_SYNTAX_STAR_TOKEN:
    return "*";
  case MPPL_SYNTAX_EQUAL_TOKEN:
    return "=";
  case MPPL_SYNTAX_NOTEQ_TOKEN:
    return "<>";
  case MPPL_SYNTAX_LESS_TOKEN:
    return "<";
  case MPPL_SYNTAX_LESSEQ_TOKEN:
    return "<=";
  case MPPL_SYNTAX_GREATER_TOKEN:
    return ">";
  case MPPL_SYNTAX_GREATEREQ_TOKEN:
    return ">=";
  case MPPL_SYNTAX_LPAREN_TOKEN:
    return "(";
  case MPPL_SYNTAX_RPAREN_TOKEN:
    return ")";
  case MPPL_SYNTAX_LBRACKET_TOKEN:
    return "[";
  case MPPL_SYNTAX_RBRACKET_TOKEN:
    return "]";
  case MPPL_SYNTAX_ASSIGN_TOKEN:
    return ":=";
  case MPPL_SYNTAX_DOT_TOKEN:
    return ".";
  case MPPL_SYNTAX_COMMA_TOKEN:
    return ",";
  case MPPL_SYNTAX_COLON_TOKEN:
    return ":";
  case MPPL_SYNTAX_SEMI_TOKEN:
    return ";";
  case MPPL_SYNTAX_PROGRAM_KW:
    return "program";
  case MPPL_SYNTAX_VAR_KW:
    return "var";
  case MPPL_SYNTAX_ARRAY_KW:
    return "array";
  case MPPL_SYNTAX_OF_KW:
    return "of";
  case MPPL_SYNTAX_BEGIN_KW:
    return "begin";
  case MPPL_SYNTAX_END_KW:
    return "end";
  case MPPL_SYNTAX_IF_KW:
    return "if";
  case MPPL_SYNTAX_THEN_KW:
    return "then";
  case MPPL_SYNTAX_ELSE_KW:
    return "else";
  case MPPL_SYNTAX_PROCEDURE_KW:
    return "procedure";
  case MPPL_SYNTAX_RETURN_KW:
    return "return";
  case MPPL_SYNTAX_CALL_KW:
    return "call";
  case MPPL_SYNTAX_WHILE_KW:
    return "while";
  case MPPL_SYNTAX_DO_KW:
    return "do";
  case MPPL_SYNTAX_NOT_KW:
    return "not";
  case MPPL_SYNTAX_OR_KW:
    return "or";
  case MPPL_SYNTAX_DIV_KW:
    return "div";
  case MPPL_SYNTAX_AND_KW:
    return "and";
  case MPPL_SYNTAX_CHAR_KW:
    return "char";
  case MPPL_SYNTAX_INTEGER_KW:
    return "integer";
  case MPPL_SYNTAX_BOOLEAN_KW:
    return "boolean";
  case MPPL_SYNTAX_READ_KW:
    return "read";
  case MPPL_SYNTAX_WRITE_KW:
    return "write";
  case MPPL_SYNTAX_READLN_KW:
    return "readln";
  case MPPL_SYNTAX_WRITELN_KW:
    return "writeln";
  case MPPL_SYNTAX_TRUE_KW:
    return "true";
  case MPPL_SYNTAX_FALSE_KW:
    return "false";
  case MPPL_SYNTAX_BREAK_KW:
    return "break";
  default:
    return NULL;
  }
}

const char *mppl_syntax_kind_to_string(MpplSyntaxKind kind)
{
  switch (kind) {
  case MPPL_SYNTAX_ERROR:
    return "ERROR";
  case MPPL_SYNTAX_EOF_TOKEN:
    return "EOF_TOKEN";
  case MPPL_SYNTAX_IDENT_TOKEN:
    return "IDENT_TOKEN";
  case MPPL_SYNTAX_NUMBER_LIT:
    return "NUMBER_LIT";
  case MPPL_SYNTAX_STRING_LIT:
    return "STRING_LIT";
  case MPPL_SYNTAX_PLUS_TOKEN:
    return "PLUS_TOKEN";
  case MPPL_SYNTAX_MINUS_TOKEN:
    return "MINUS_TOKEN";
  case MPPL_SYNTAX_STAR_TOKEN:
    return "STAR_TOKEN";
  case MPPL_SYNTAX_EQUAL_TOKEN:
    return "EQUAL_TOKEN";
  case MPPL_SYNTAX_NOTEQ_TOKEN:
    return "NOTEQ_TOKEN";
  case MPPL_SYNTAX_LESS_TOKEN:
    return "LESS_TOKEN";
  case MPPL_SYNTAX_LESSEQ_TOKEN:
    return "LESSEQ_TOKEN";
  case MPPL_SYNTAX_GREATER_TOKEN:
    return "GREATER_TOKEN";
  case MPPL_SYNTAX_GREATEREQ_TOKEN:
    return "GREATEREQ_TOKEN";
  case MPPL_SYNTAX_LPAREN_TOKEN:
    return "LPAREN_TOKEN";
  case MPPL_SYNTAX_RPAREN_TOKEN:
    return "RPAREN_TOKEN";
  case MPPL_SYNTAX_LBRACKET_TOKEN:
    return "LBRACKET_TOKEN";
  case MPPL_SYNTAX_RBRACKET_TOKEN:
    return "RBRACKET_TOKEN";
  case MPPL_SYNTAX_ASSIGN_TOKEN:
    return "ASSIGN_TOKEN";
  case MPPL_SYNTAX_DOT_TOKEN:
    return "DOT_TOKEN";
  case MPPL_SYNTAX_COMMA_TOKEN:
    return "COMMA_TOKEN";
  case MPPL_SYNTAX_COLON_TOKEN:
    return "COLON_TOKEN";
  case MPPL_SYNTAX_SEMI_TOKEN:
    return "SEMI_TOKEN";
  case MPPL_SYNTAX_PROGRAM_KW:
    return "PROGRAM_KW";
  case MPPL_SYNTAX_VAR_KW:
    return "VAR_KW";
  case MPPL_SYNTAX_ARRAY_KW:
    return "ARRAY_KW";
  case MPPL_SYNTAX_OF_KW:
    return "OF_KW";
  case MPPL_SYNTAX_BEGIN_KW:
    return "BEGIN_KW";
  case MPPL_SYNTAX_END_KW:
    return "END_KW";
  case MPPL_SYNTAX_IF_KW:
    return "IF_KW";
  case MPPL_SYNTAX_THEN_KW:
    return "THEN_KW";
  case MPPL_SYNTAX_ELSE_KW:
    return "ELSE_KW";
  case MPPL_SYNTAX_PROCEDURE_KW:
    return "PROCEDURE_KW";
  case MPPL_SYNTAX_RETURN_KW:
    return "RETURN_KW";
  case MPPL_SYNTAX_CALL_KW:
    return "CALL_KW";
  case MPPL_SYNTAX_WHILE_KW:
    return "WHILE_KW";
  case MPPL_SYNTAX_DO_KW:
    return "DO_KW";
  case MPPL_SYNTAX_NOT_KW:
    return "NOT_KW";
  case MPPL_SYNTAX_OR_KW:
    return "OR_KW";
  case MPPL_SYNTAX_DIV_KW:
    return "DIV_KW";
  case MPPL_SYNTAX_AND_KW:
    return "AND_KW";
  case MPPL_SYNTAX_CHAR_KW:
    return "CHAR_KW";
  case MPPL_SYNTAX_INTEGER_KW:
    return "INTEGER_KW";
  case MPPL_SYNTAX_BOOLEAN_KW:
    return "BOOLEAN_KW";
  case MPPL_SYNTAX_READ_KW:
    return "READ_KW";
  case MPPL_SYNTAX_WRITE_KW:
    return "WRITE_KW";
  case MPPL_SYNTAX_READLN_KW:
    return "READLN_KW";
  case MPPL_SYNTAX_WRITELN_KW:
    return "WRITELN_KW";
  case MPPL_SYNTAX_TRUE_KW:
    return "TRUE_KW";
  case MPPL_SYNTAX_FALSE_KW:
    return "FALSE_KW";
  case MPPL_SYNTAX_BREAK_KW:
    return "BREAK_KW";
  case MPPL_SYNTAX_SPACE_TRIVIA:
    return "SPACE_TRIVIA";
  case MPPL_SYNTAX_BRACES_COMMENT_TRIVIA:
    return "BRACES_COMMENT_TRIVIA";
  case MPPL_SYNTAX_C_COMMENT_TRIVIA:
    return "C_COMMENT_TRIVIA";
  case MPPL_SYNTAX_EOF:
    return "EOF";
  case MPPL_SYNTAX_PROGRAM:
    return "PROGRAM";
  case MPPL_SYNTAX_BIND_IDENT_LIST_ELEM:
    return "BIND_IDENT_LIST_ELEM";
  case MPPL_SYNTAX_BIND_IDENT_LIST:
    return "BIND_IDENT_LIST";
  case MPPL_SYNTAX_BIND_IDENT:
    return "BIND_IDENT";
  case MPPL_SYNTAX_DECL_PART_LIST:
    return "DECL_PART_LIST";
  case MPPL_SYNTAX_VAR_DECL_PART:
    return "VAR_DECL_PART";
  case MPPL_SYNTAX_VAR_DECL_LIST_ELEM:
    return "VAR_DECL_LIST_ELEM";
  case MPPL_SYNTAX_VAR_DECL_LIST:
    return "VAR_DECL_LIST";
  case MPPL_SYNTAX_VAR_DECL:
    return "VAR_DECL";
  case MPPL_SYNTAX_INTEGER_TYPE:
    return "INTEGER_TYPE";
  case MPPL_SYNTAX_BOOLEAN_TYPE:
    return "BOOLEAN_TYPE";
  case MPPL_SYNTAX_CHAR_TYPE:
    return "CHAR_TYPE";
  case MPPL_SYNTAX_ARRAY_TYPE:
    return "ARRAY_TYPE";
  case MPPL_SYNTAX_PROC_DECL_PART:
    return "PROC_DECL_PART";
  case MPPL_SYNTAX_PROC_HEADING:
    return "PROC_HEADING";
  case MPPL_SYNTAX_PROC_BODY:
    return "PROC_BODY";
  case MPPL_SYNTAX_PROC_DECL:
    return "PROC_DECL";
  case MPPL_SYNTAX_FML_PARAM_LIST_ELEM:
    return "FML_PARAM_LIST_ELEM";
  case MPPL_SYNTAX_FML_PARAM_LIST:
    return "FML_PARAM_LIST";
  case MPPL_SYNTAX_FML_PARAMS:
    return "FML_PARAMS";
  case MPPL_SYNTAX_FML_PARAM_SEC:
    return "FML_PARAM_SEC";
  case MPPL_SYNTAX_STMT_LIST_ELEM:
    return "STMT_LIST_ELEM";
  case MPPL_SYNTAX_STMT_LIST:
    return "STMT_LIST";
  case MPPL_SYNTAX_ASSIGN_STMT:
    return "ASSIGN_STMT";
  case MPPL_SYNTAX_IF_STMT:
    return "IF_STMT";
  case MPPL_SYNTAX_ELSE_CLAUSE:
    return "ELSE_CLAUSE";
  case MPPL_SYNTAX_WHILE_STMT:
    return "WHILE_STMT";
  case MPPL_SYNTAX_BREAK_STMT:
    return "BREAK_STMT";
  case MPPL_SYNTAX_CALL_STMT:
    return "CALL_STMT";
  case MPPL_SYNTAX_ACT_PARAMS:
    return "ACT_PARAMS";
  case MPPL_SYNTAX_RETURN_STMT:
    return "RETURN_STMT";
  case MPPL_SYNTAX_INPUT_STMT:
    return "INPUT_STMT";
  case MPPL_SYNTAX_INPUTS:
    return "INPUTS";
  case MPPL_SYNTAX_OUTPUT_STMT:
    return "OUTPUT_STMT";
  case MPPL_SYNTAX_OUTPUT_LIST_ELEM:
    return "OUTPUT_LIST_ELEM";
  case MPPL_SYNTAX_OUTPUT_LIST:
    return "OUTPUT_LIST";
  case MPPL_SYNTAX_OUTPUTS:
    return "OUTPUTS";
  case MPPL_SYNTAX_OUTPUT_VALUE:
    return "OUTPUT_VALUE";
  case MPPL_SYNTAX_COMP_STMT:
    return "COMP_STMT";
  case MPPL_SYNTAX_EXPR_LIST_ELEM:
    return "EXPR_LIST_ELEM";
  case MPPL_SYNTAX_EXPR_LIST:
    return "EXPR_LIST";
  case MPPL_SYNTAX_REF_IDENT:
    return "REF_IDENT";
  case MPPL_SYNTAX_ENTIRE_VAR:
    return "ENTIRE_VAR";
  case MPPL_SYNTAX_INDEXED_VAR:
    return "INDEXED_VAR";
  case MPPL_SYNTAX_UNARY_EXPR:
    return "UNARY_EXPR";
  case MPPL_SYNTAX_BINARY_EXPR:
    return "BINARY_EXPR";
  case MPPL_SYNTAX_PAREN_EXPR:
    return "PAREN_EXPR";
  case MPPL_SYNTAX_CAST_EXPR:
    return "CAST_EXPR";
  case MPPL_SYNTAX_BOGUS_EOF:
    return "BOGUS_EOF";
  case MPPL_SYNTAX_BOGUS_DECL_PART:
    return "BOGUS_DECL_PART";
  case MPPL_SYNTAX_BOGUS_VAR_DECL:
    return "BOGUS_VAR_DECL";
  case MPPL_SYNTAX_BOGUS_FML_PARAM_SEC:
    return "BOGUS_FML_PARAM_SEC";
  case MPPL_SYNTAX_BOGUS_STMT:
    return "BOGUS_STMT";
  case MPPL_SYNTAX_BOGUS_OUTPUT_VALUE:
    return "BOGUS_OUTPUT_VALUE";
  case MPPL_SYNTAX_BOGUS_EXPR:
    return "BOGUS_EXPR";
  case MPPL_SYNTAX_BOGUS_BIND_IDENT:
    return "BOGUS_BIND_IDENT";
  default:
    unreachable();
  }
}

/* MPPL syntax */

static void *try_cast(SyntaxTree *syntax, int condition)
{
  if (condition) {
    return syntax;
  } else {
    syntax_tree_free(syntax);
    return NULL;
  }
}

MpplRootSyntax *mppl_root_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->node.parent == NULL);
}

MpplProgramSyntax *mppl_program_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PROGRAM);
}

MpplEofSyntax *mppl_eof_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_EOF);
}

MpplDeclPartListSyntax *mppl_decl_part_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_DECL_PART_LIST);
}

MpplBindIdentListElemSyntax *mppl_bind_ident_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT_LIST_ELEM);
}

MpplBindIdentListSyntax *mppl_bind_ident_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT_LIST);
}

MpplBindIdentSyntax *mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT);
}

MpplVarDeclPartSyntax *mppl_var_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_PART);
}

MpplVarDeclListElemSyntax *mppl_var_decl_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_LIST_ELEM);
}

MpplVarDeclListSyntax *mppl_var_decl_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_LIST);
}

MpplVarDeclSyntax *mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL);
}

MpplIntegerTypeSyntax *mppl_integer_type_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_INTEGER_TYPE);
}

MpplCharTypeSyntax *mppl_char_type_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_CHAR_TYPE);
}

MpplBooleanTypeSyntax *mppl_boolean_type_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOOLEAN_TYPE);
}

MpplArrayTypeSyntax *mppl_array_type_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_ARRAY_TYPE);
}

MpplProcDeclPartSyntax *mppl_proc_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PROC_DECL_PART);
}

MpplProcHeadingSyntax *mppl_proc_heading_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PROC_HEADING);
}

MpplProcBodySyntax *mppl_proc_body_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PROC_BODY);
}

MpplProcDeclSyntax *mppl_proc_decl_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PROC_DECL);
}

MpplFmlParamListElemSyntax *mppl_fml_param_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_LIST_ELEM);
}

MpplFmlParamListSyntax *mppl_fml_param_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_LIST);
}

MpplFmlParamsSyntax *mppl_fml_params_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAMS);
}

MpplFmlParamSecSyntax *mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_SEC);
}

MpplStmtListElemSyntax *mppl_stmt_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_STMT_LIST_ELEM);
}

MpplStmtListSyntax *mppl_stmt_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_STMT_LIST);
}

MpplAssignStmtSyntax *mppl_assign_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_ASSIGN_STMT);
}

MpplIfStmtSyntax *mppl_if_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_IF_STMT);
}

MpplElseClauseSyntax *mppl_else_clause_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_ELSE_CLAUSE);
}

MpplWhileStmtSyntax *mppl_while_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_WHILE_STMT);
}

MpplBreakStmtSyntax *mppl_break_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BREAK_STMT);
}

MpplCallStmtSyntax *mppl_call_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_CALL_STMT);
}

MpplActParamsSyntax *mppl_act_params_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_ACT_PARAMS);
}

MpplReturnStmtSyntax *mppl_return_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_RETURN_STMT);
}

MpplInputStmtSyntax *mppl_input_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_INPUT_STMT);
}

MpplInputsSyntax *mppl_inputs_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_INPUTS);
}

MpplOutputStmtSyntax *mppl_output_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_STMT);
}

MpplOutputListElemSyntax *mppl_output_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_LIST_ELEM);
}

MpplOutputListSyntax *mppl_output_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_LIST);
}

MpplOutputsSyntax *mppl_outputs_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_OUTPUTS);
}

MpplOutputValueSyntax *mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_VALUE);
}

MpplCompStmtSyntax *mppl_comp_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_COMP_STMT);
}

MpplExprListElemSyntax *mppl_expr_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_EXPR_LIST_ELEM);
}

MpplExprListSyntax *mppl_expr_list_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_EXPR_LIST);
}

MpplRefIdentSyntax *mppl_ref_ident_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_REF_IDENT);
}

MpplEntireVarSyntax *mppl_entire_var_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_ENTIRE_VAR);
}

MpplIndexedVarSyntax *mppl_indexed_var_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_INDEXED_VAR);
}

MpplUnaryExprSyntax *mppl_unary_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_UNARY_EXPR);
}

MpplBinaryExprSyntax *mppl_binary_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BINARY_EXPR);
}

MpplParenExprSyntax *mppl_paren_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_PAREN_EXPR);
}

MpplCastExprSyntax *mppl_cast_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_CAST_EXPR);
}

void mppl_root_syntax_free(MpplRootSyntax *root)
{
  syntax_tree_free((SyntaxTree *) root);
}

void mppl_program_syntax_free(MpplProgramSyntax *program)
{
  syntax_tree_free((SyntaxTree *) program);
}

void mppl_eof_syntax_free(MpplEofSyntax *eof)
{
  syntax_tree_free((SyntaxTree *) eof);
}

void mppl_decl_part_list_syntax_free(MpplDeclPartListSyntax *decl_part_list)
{
  syntax_tree_free((SyntaxTree *) decl_part_list);
}

void mppl_bind_ident_list_elem_syntax_free(MpplBindIdentListElemSyntax *bind_ident_list_elem)
{
  syntax_tree_free((SyntaxTree *) bind_ident_list_elem);
}

void mppl_bind_ident_list_syntax_free(MpplBindIdentListSyntax *bind_ident_list)
{
  syntax_tree_free((SyntaxTree *) bind_ident_list);
}

void mppl_bind_ident_syntax_free(MpplBindIdentSyntax *bind_ident)
{
  syntax_tree_free((SyntaxTree *) bind_ident);
}

void mppl_var_decl_part_syntax_free(MpplVarDeclPartSyntax *var_decl_part)
{
  syntax_tree_free((SyntaxTree *) var_decl_part);
}

void mppl_var_decl_list_elem_syntax_free(MpplVarDeclListElemSyntax *var_decl_list_elem)
{
  syntax_tree_free((SyntaxTree *) var_decl_list_elem);
}

void mppl_var_decl_list_syntax_free(MpplVarDeclListSyntax *var_decl_list)
{
  syntax_tree_free((SyntaxTree *) var_decl_list);
}

void mppl_var_decl_syntax_free(MpplVarDeclSyntax *var_decl)
{
  syntax_tree_free((SyntaxTree *) var_decl);
}

void mppl_integer_type_syntax_free(MpplIntegerTypeSyntax *integer_type)
{
  syntax_tree_free((SyntaxTree *) integer_type);
}

void mppl_char_type_syntax_free(MpplCharTypeSyntax *char_type)
{
  syntax_tree_free((SyntaxTree *) char_type);
}

void mppl_boolean_type_syntax_free(MpplBooleanTypeSyntax *boolean_type)
{
  syntax_tree_free((SyntaxTree *) boolean_type);
}

void mppl_array_type_syntax_free(MpplArrayTypeSyntax *array_type)
{
  syntax_tree_free((SyntaxTree *) array_type);
}

void mppl_proc_decl_part_syntax_free(MpplProcDeclPartSyntax *proc_decl_part)
{
  syntax_tree_free((SyntaxTree *) proc_decl_part);
}

void mppl_proc_heading_syntax_free(MpplProcHeadingSyntax *proc_heading)
{
  syntax_tree_free((SyntaxTree *) proc_heading);
}

void mppl_proc_body_syntax_free(MpplProcBodySyntax *proc_body)
{
  syntax_tree_free((SyntaxTree *) proc_body);
}

void mppl_proc_decl_syntax_free(MpplProcDeclSyntax *proc_decl)
{
  syntax_tree_free((SyntaxTree *) proc_decl);
}

void mppl_fml_param_list_elem_syntax_free(MpplFmlParamListElemSyntax *fml_param_list_elem)
{
  syntax_tree_free((SyntaxTree *) fml_param_list_elem);
}

void mppl_fml_param_list_syntax_free(MpplFmlParamListSyntax *fml_param_list)
{
  syntax_tree_free((SyntaxTree *) fml_param_list);
}

void mppl_fml_params_syntax_free(MpplFmlParamsSyntax *fml_params)
{
  syntax_tree_free((SyntaxTree *) fml_params);
}

void mppl_fml_param_sec_syntax_free(MpplFmlParamSecSyntax *fml_param_sec)
{
  syntax_tree_free((SyntaxTree *) fml_param_sec);
}

void mppl_stmt_list_elem_syntax_free(MpplStmtListElemSyntax *stmt_list_elem)
{
  syntax_tree_free((SyntaxTree *) stmt_list_elem);
}

void mppl_stmt_list_syntax_free(MpplStmtListSyntax *stmt_list)
{
  syntax_tree_free((SyntaxTree *) stmt_list);
}

void mppl_assign_stmt_syntax_free(MpplAssignStmtSyntax *assign_stmt)
{
  syntax_tree_free((SyntaxTree *) assign_stmt);
}

void mppl_if_stmt_syntax_free(MpplIfStmtSyntax *if_stmt)
{
  syntax_tree_free((SyntaxTree *) if_stmt);
}

void mppl_else_clause_syntax_free(MpplElseClauseSyntax *else_clause)
{
  syntax_tree_free((SyntaxTree *) else_clause);
}

void mppl_while_stmt_syntax_free(MpplWhileStmtSyntax *while_stmt)
{
  syntax_tree_free((SyntaxTree *) while_stmt);
}

void mppl_break_stmt_syntax_free(MpplBreakStmtSyntax *break_stmt)
{
  syntax_tree_free((SyntaxTree *) break_stmt);
}

void mppl_call_stmt_syntax_free(MpplCallStmtSyntax *call_stmt)
{
  syntax_tree_free((SyntaxTree *) call_stmt);
}

void mppl_act_params_syntax_free(MpplActParamsSyntax *act_params)
{
  syntax_tree_free((SyntaxTree *) act_params);
}

void mppl_return_stmt_syntax_free(MpplReturnStmtSyntax *return_stmt)
{
  syntax_tree_free((SyntaxTree *) return_stmt);
}

void mppl_input_stmt_syntax_free(MpplInputStmtSyntax *input_stmt)
{
  syntax_tree_free((SyntaxTree *) input_stmt);
}

void mppl_inputs_syntax_free(MpplInputsSyntax *inputs)
{
  syntax_tree_free((SyntaxTree *) inputs);
}

void mppl_output_stmt_syntax_free(MpplOutputStmtSyntax *output_stmt)
{
  syntax_tree_free((SyntaxTree *) output_stmt);
}

void mppl_output_list_elem_syntax_free(MpplOutputListElemSyntax *output_list_elem)
{
  syntax_tree_free((SyntaxTree *) output_list_elem);
}

void mppl_output_list_syntax_free(MpplOutputListSyntax *output_list)
{
  syntax_tree_free((SyntaxTree *) output_list);
}

void mppl_outputs_syntax_free(MpplOutputsSyntax *outputs)
{
  syntax_tree_free((SyntaxTree *) outputs);
}

void mppl_output_value_syntax_free(MpplOutputValueSyntax *output_value)
{
  syntax_tree_free((SyntaxTree *) output_value);
}

void mppl_comp_stmt_syntax_free(MpplCompStmtSyntax *comp_stmt)
{
  syntax_tree_free((SyntaxTree *) comp_stmt);
}

void mppl_expr_list_elem_syntax_free(MpplExprListElemSyntax *expr_list_elem)
{
  syntax_tree_free((SyntaxTree *) expr_list_elem);
}

void mppl_expr_list_syntax_free(MpplExprListSyntax *expr_list)
{
  syntax_tree_free((SyntaxTree *) expr_list);
}

void mppl_ref_ident_syntax_free(MpplRefIdentSyntax *ref_ident)
{
  syntax_tree_free((SyntaxTree *) ref_ident);
}

void mppl_entire_var_syntax_free(MpplEntireVarSyntax *entire_var)
{
  syntax_tree_free((SyntaxTree *) entire_var);
}

void mppl_indexed_var_syntax_free(MpplIndexedVarSyntax *indexed_var)
{
  syntax_tree_free((SyntaxTree *) indexed_var);
}

void mppl_unary_expr_syntax_free(MpplUnaryExprSyntax *unary_expr)
{
  syntax_tree_free((SyntaxTree *) unary_expr);
}

void mppl_binary_expr_syntax_free(MpplBinaryExprSyntax *binary_expr)
{
  syntax_tree_free((SyntaxTree *) binary_expr);
}

void mppl_paren_expr_syntax_free(MpplParenExprSyntax *paren_expr)
{
  syntax_tree_free((SyntaxTree *) paren_expr);
}

void mppl_cast_expr_syntax_free(MpplCastExprSyntax *cast_expr)
{
  syntax_tree_free((SyntaxTree *) cast_expr);
}

BogusMpplEofSyntax *bogus_mppl_eof_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_EOF);
}

BogusMpplDeclPartSyntax *bogus_mppl_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_DECL_PART);
}

BogusMpplVarDeclSyntax *bogus_mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_VAR_DECL);
}

BogusMpplFmlParamSecSyntax *bogus_mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_FML_PARAM_SEC);
}

BogusMpplStmtSyntax *bogus_mppl_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_STMT);
}

BogusMpplOutputValueSyntax *bogus_mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_OUTPUT_VALUE);
}

BogusMpplExprSyntax *bogus_mppl_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_EXPR);
}

BogusMpplBindIdentSyntax *bogus_mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_BIND_IDENT);
}

void bogus_mppl_eof_syntax_free(BogusMpplEofSyntax *bogus_eof)
{
  syntax_tree_free((SyntaxTree *) bogus_eof);
}

void bogus_mppl_decl_part_syntax_free(BogusMpplDeclPartSyntax *bogus_decl_part)
{
  syntax_tree_free((SyntaxTree *) bogus_decl_part);
}

void bogus_mppl_var_decl_syntax_free(BogusMpplVarDeclSyntax *bogus_var_decl)
{
  syntax_tree_free((SyntaxTree *) bogus_var_decl);
}

void bogus_mppl_fml_param_sec_syntax_free(BogusMpplFmlParamSecSyntax *bogus_fml_param_sec)
{
  syntax_tree_free((SyntaxTree *) bogus_fml_param_sec);
}

void bogus_mppl_stmt_syntax_free(BogusMpplStmtSyntax *bogus_stmt)
{
  syntax_tree_free((SyntaxTree *) bogus_stmt);
}

void bogus_mppl_output_value_syntax_free(BogusMpplOutputValueSyntax *bogus_output_value)
{
  syntax_tree_free((SyntaxTree *) bogus_output_value);
}

void bogus_mppl_expr_syntax_free(BogusMpplExprSyntax *bogus_expr)
{
  syntax_tree_free((SyntaxTree *) bogus_expr);
}

void bogus_mppl_bind_ident_syntax_free(BogusMpplBindIdentSyntax *BOGUS_BIND_IDENT)
{
  syntax_tree_free((SyntaxTree *) BOGUS_BIND_IDENT);
}

static int eof_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_EOF:
    return MPPL_EOF_SYNTAX;
  case MPPL_SYNTAX_BOGUS_EOF:
    return MPPL_EOF_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int decl_part_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_VAR_DECL_PART:
    return MPPL_DECL_PART_SYNTAX_VAR;
  case MPPL_SYNTAX_PROC_DECL_PART:
    return MPPL_DECL_PART_SYNTAX_PROC;
  case MPPL_SYNTAX_BOGUS_DECL_PART:
    return MPPL_DECL_PART_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int type_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_INTEGER_TYPE:
    return MPPL_TYPE_SYNTAX_INTEGER;
  case MPPL_SYNTAX_CHAR_TYPE:
    return MPPL_TYPE_SYNTAX_CHAR;
  case MPPL_SYNTAX_BOOLEAN_TYPE:
    return MPPL_TYPE_SYNTAX_BOOLEAN;
  case MPPL_SYNTAX_ARRAY_TYPE:
    return MPPL_TYPE_SYNTAX_ARRAY;
  default:
    return -1;
  }
}

static int var_decl_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_VAR_DECL:
    return MPPL_VAR_DECL_SYNTAX;
  case MPPL_SYNTAX_BOGUS_VAR_DECL:
    return MPPL_VAR_DECL_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int fml_param_sec_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_FML_PARAM_SEC:
    return MPPL_FML_PARAM_SEC_SYNTAX;
  case MPPL_SYNTAX_BOGUS_FML_PARAM_SEC:
    return MPPL_FML_PARAM_SEC_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int stmt_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_ASSIGN_STMT:
    return MPPL_STMT_SYNTAX_ASSIGN;
  case MPPL_SYNTAX_IF_STMT:
    return MPPL_STMT_SYNTAX_IF;
  case MPPL_SYNTAX_WHILE_STMT:
    return MPPL_STMT_SYNTAX_WHILE;
  case MPPL_SYNTAX_BREAK_STMT:
    return MPPL_STMT_SYNTAX_BREAK;
  case MPPL_SYNTAX_CALL_STMT:
    return MPPL_STMT_SYNTAX_CALL;
  case MPPL_SYNTAX_RETURN_STMT:
    return MPPL_STMT_SYNTAX_RETURN;
  case MPPL_STMT_SYNTAX_INPUT:
    return MPPL_SYNTAX_INPUT_STMT;
  case MPPL_STMT_SYNTAX_OUTPUT:
    return MPPL_SYNTAX_OUTPUT_STMT;
  case MPPL_SYNTAX_COMP_STMT:
    return MPPL_STMT_SYNTAX_COMP;
  case MPPL_SYNTAX_BOGUS_STMT:
    return MPPL_STMT_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int var_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_ENTIRE_VAR:
    return MPPL_VAR_SYNTAX_ENTIRE;
  case MPPL_SYNTAX_INDEXED_VAR:
    return MPPL_VAR_SYNTAX_INDEXED;
  default:
    return -1;
  }
}

static int expr_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_UNARY_EXPR:
    return MPPL_EXPR_SYNTAX_UNARY;
  case MPPL_SYNTAX_BINARY_EXPR:
    return MPPL_EXPR_SYNTAX_BINARY;
  case MPPL_SYNTAX_PAREN_EXPR:
    return MPPL_EXPR_SYNTAX_PAREN;
  case MPPL_SYNTAX_CAST_EXPR:
    return MPPL_EXPR_SYNTAX_CAST;
  case MPPL_SYNTAX_BOGUS_EXPR:
    return MPPL_EXPR_SYNTAX_BOGUS;
  default:
    if (var_kind(syntax) != -1) {
      return MPPL_EXPR_SYNTAX_VAR;
    } else {
      return -1;
    }
  }
}

static int output_value_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_OUTPUT_VALUE:
    return MPPL_OUTPUT_VALUE_SYNTAX;
  case MPPL_SYNTAX_BOGUS_OUTPUT_VALUE:
    return MPPL_OUTPUT_VALUE_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

static int output_kind(const SyntaxTree *syntax)
{
  if (expr_kind(syntax) != -1) {
    return MPPL_OUTPUT_SYNTAX_EXPR;
  } else if (output_value_kind(syntax) != -1) {
    return MPPL_OUTPUT_SYNTAX_OUTPUT_VALUE;
  } else {
    return -1;
  }
}

static int bind_ident_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_BIND_IDENT:
    return MPPL_BIND_IDENT_SYNTAX;
  case MPPL_SYNTAX_BOGUS_BIND_IDENT:
    return MPPL_BIND_IDENT_SYNTAX_BOGUS;
  default:
    return -1;
  }
}

AnyMpplEofSyntax *any_mppl_eof_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, eof_kind(syntax) != -1);
}

AnyMpplDeclPartSyntax *any_mppl_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, decl_part_kind(syntax) != -1);
}

AnyMpplTypeSyntax *any_mppl_type_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, type_kind(syntax) != -1);
}

AnyMpplVarDeclSyntax *any_mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, var_decl_kind(syntax) != -1);
}

AnyMpplFmlParamSecSyntax *any_mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, fml_param_sec_kind(syntax) != -1);
}

AnyMpplStmtSyntax *any_mppl_stmt_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, stmt_kind(syntax) != -1);
}

AnyMpplOutputSyntax *any_mppl_output_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, output_kind(syntax) != -1);
}

AnyMpplOutputValueSyntax *any_mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, output_value_kind(syntax) != -1);
}

AnyMpplVarSyntax *any_mppl_var_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, var_kind(syntax) != -1);
}

AnyMpplExprSyntax *any_mppl_expr_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, expr_kind(syntax) != -1);
}

AnyMpplBindIdentSyntax *any_mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  return try_cast(syntax, bind_ident_kind(syntax) != -1);
}

void any_mppl_eof_syntax_free(AnyMpplEofSyntax *any_eof)
{
  syntax_tree_free((SyntaxTree *) any_eof);
}

void any_mppl_decl_part_syntax_free(AnyMpplDeclPartSyntax *any_decl_part)
{
  syntax_tree_free((SyntaxTree *) any_decl_part);
}

void any_mppl_type_syntax_free(AnyMpplTypeSyntax *any_type)
{
  syntax_tree_free((SyntaxTree *) any_type);
}

void any_mppl_var_decl_syntax_free(AnyMpplVarDeclSyntax *any_var_decl)
{
  syntax_tree_free((SyntaxTree *) any_var_decl);
}

void any_mppl_fml_param_sec_syntax_free(AnyMpplFmlParamSecSyntax *any_fml_param_sec)
{
  syntax_tree_free((SyntaxTree *) any_fml_param_sec);
}

void any_mppl_stmt_syntax_free(AnyMpplStmtSyntax *any_stmt)
{
  syntax_tree_free((SyntaxTree *) any_stmt);
}

void any_mppl_output_syntax_free(AnyMpplOutputSyntax *any_output)
{
  syntax_tree_free((SyntaxTree *) any_output);
}

void any_mppl_output_value_syntax_free(AnyMpplOutputValueSyntax *any_output_value)
{
  syntax_tree_free((SyntaxTree *) any_output_value);
}

void any_mppl_var_syntax_free(AnyMpplVarSyntax *any_var)
{
  syntax_tree_free((SyntaxTree *) any_var);
}

void any_mppl_expr_syntax_free(AnyMpplExprSyntax *any_expr)
{
  syntax_tree_free((SyntaxTree *) any_expr);
}

void any_mppl_bind_ident_syntax_free(AnyMpplBindIdentSyntax *any_bind_ident)
{
  syntax_tree_free((SyntaxTree *) any_bind_ident);
}

MpplEofSyntaxKind mppl_eof_syntax_kind(const AnyMpplEofSyntax *eof)
{
  return eof_kind((const SyntaxTree *) eof);
}

MpplDeclPartSyntaxKind mppl_decl_part_syntax_kind(const AnyMpplDeclPartSyntax *decl_part)
{
  return decl_part_kind((const SyntaxTree *) decl_part);
}

MpplTypeSyntaxKind mppl_type_syntax_kind(const AnyMpplTypeSyntax *type)
{
  return type_kind((const SyntaxTree *) type);
}

MpplVarDeclSyntaxKind mppl_var_decl_syntax_kind(const AnyMpplVarDeclSyntax *var_decl)
{
  return var_decl_kind((const SyntaxTree *) var_decl);
}

MpplFmlParamSecSyntaxKind mppl_fml_param_sec_syntax_kind(const AnyMpplFmlParamSecSyntax *fml_param_sec)
{
  return fml_param_sec_kind((const SyntaxTree *) fml_param_sec);
}

MpplStmtSyntaxKind mppl_stmt_syntax_kind(const AnyMpplStmtSyntax *stmt)
{
  return stmt_kind((const SyntaxTree *) stmt);
}

MpplOutputSyntaxKind mppl_output_syntax_kind(const AnyMpplOutputSyntax *output)
{
  return output_kind((const SyntaxTree *) output);
}

MpplOutputValueSyntaxKind mppl_output_value_syntax_kind(const AnyMpplOutputValueSyntax *output_value)
{
  return output_value_kind((const SyntaxTree *) output_value);
}

MpplVarSyntaxKind mppl_var_syntax_kind(const AnyMpplVarSyntax *var)
{
  return var_kind((const SyntaxTree *) var);
}

MpplExprSyntaxKind mppl_expr_syntax_kind(const AnyMpplExprSyntax *expr)
{
  return expr_kind((const SyntaxTree *) expr);
}

MpplBindIdentSyntaxKind mppl_bind_ident_syntax_kind(const AnyMpplBindIdentSyntax *bind_ident)
{
  return bind_ident_kind((const SyntaxTree *) bind_ident);
}

MpplRootSyntaxFields mppl_root_syntax_fields_alloc(const MpplRootSyntax *root)
{
  MpplRootSyntaxFields fields;
  fields.program = mppl_program_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) root, 0));
  fields.eof     = any_mppl_eof_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) root, 1));
  return fields;
}

MpplProgramSyntaxFields mppl_program_syntax_fields_alloc(const MpplProgramSyntax *program)
{
  MpplProgramSyntaxFields fields;
  fields.program_kw     = syntax_tree_child_token((const SyntaxTree *) program, 0);
  fields.name           = syntax_tree_child_token((const SyntaxTree *) program, 1);
  fields.semi_token     = syntax_tree_child_token((const SyntaxTree *) program, 2);
  fields.decl_part_list = mppl_decl_part_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) program, 3));
  fields.comp_stmt      = mppl_comp_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) program, 4));
  fields.dot_token      = syntax_tree_child_token((const SyntaxTree *) program, 5);
  return fields;
}

MpplEofSyntaxFields mppl_eof_syntax_fields_alloc(const MpplEofSyntax *eof)
{
  MpplEofSyntaxFields fields;
  fields.eof_token = syntax_tree_child_token((const SyntaxTree *) eof, 0);
  return fields;
}

MpplDeclPartListSyntaxFields mppl_decl_part_list_syntax_fields_alloc(const MpplDeclPartListSyntax *decl_part_list)
{
  unsigned long i;

  MpplDeclPartListSyntaxFields fields;
  slice_alloc(&fields.decl_parts, decl_part_list->syntax.raw->children.count);
  for (i = 0; i * 2 < decl_part_list->syntax.raw->children.count; ++i) {
    fields.decl_parts.ptr[i] = any_mppl_decl_part_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) decl_part_list, i));
  }
  return fields;
}

MpplBindIdentListElemSyntaxFields mppl_bind_ident_list_elem_syntax_fields_alloc(const MpplBindIdentListElemSyntax *bind_ident_list_elem)
{
  MpplBindIdentListElemSyntaxFields fields;
  fields.bind_ident  = any_mppl_bind_ident_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) bind_ident_list_elem, 0));
  fields.comma_token = syntax_tree_child_token((const SyntaxTree *) bind_ident_list_elem, 1);
  return fields;
}

MpplBindIdentListSyntaxFields mppl_bind_ident_list_syntax_fields_alloc(const MpplBindIdentListSyntax *bind_ident_list)
{
  unsigned long i;

  MpplBindIdentListSyntaxFields fields;
  slice_alloc(&fields.bind_ident_list_elems, bind_ident_list->syntax.raw->children.count);
  for (i = 0; i * 2 < bind_ident_list->syntax.raw->children.count; ++i) {
    fields.bind_ident_list_elems.ptr[i] = mppl_bind_ident_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) bind_ident_list, i));
  }
  return fields;
}

MpplBindIdentSyntaxFields mppl_bind_ident_syntax_fields_alloc(const MpplBindIdentSyntax *bind_ident)
{
  MpplBindIdentSyntaxFields fields;
  fields.ident = syntax_tree_child_token((const SyntaxTree *) bind_ident, 0);
  return fields;
}

MpplVarDeclPartSyntaxFields mppl_var_decl_part_syntax_fields_alloc(const MpplVarDeclPartSyntax *var_decl_part)
{
  MpplVarDeclPartSyntaxFields fields;
  fields.var_kw        = syntax_tree_child_token((const SyntaxTree *) var_decl_part, 0);
  fields.var_decl_list = mppl_var_decl_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) var_decl_part, 1));
  return fields;
}

MpplVarDeclListElemSyntaxFields mppl_var_decl_list_elem_syntax_fields_alloc(const MpplVarDeclListElemSyntax *var_decl_list_elem)
{
  MpplVarDeclListElemSyntaxFields fields;
  fields.var_decl   = mppl_var_decl_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) var_decl_list_elem, 0));
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) var_decl_list_elem, 1);
  return fields;
}

MpplVarDeclListSyntaxFields mppl_var_decl_list_syntax_fields_alloc(const MpplVarDeclListSyntax *var_decl_list)
{
  unsigned long i;

  MpplVarDeclListSyntaxFields fields;
  slice_alloc(&fields.var_decl_list_elems, var_decl_list->syntax.raw->children.count);
  for (i = 0; i * 2 < var_decl_list->syntax.raw->children.count; ++i) {
    fields.var_decl_list_elems.ptr[i] = mppl_var_decl_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) var_decl_list, i));
  }
  return fields;
}

MpplVarDeclSyntaxFields mppl_var_decl_syntax_fields_alloc(const MpplVarDeclSyntax *var_decl)
{
  MpplVarDeclSyntaxFields fields;
  fields.ident_list  = mppl_bind_ident_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) var_decl, 0));
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) var_decl, 1);
  fields.type        = any_mppl_type_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) var_decl, 2));
  return fields;
}

MpplIntegerTypeSyntaxFields mppl_integer_type_syntax_fields_alloc(const MpplArrayTypeSyntax *integer_type)
{
  MpplIntegerTypeSyntaxFields fields;
  fields.integer_kw = syntax_tree_child_token((const SyntaxTree *) integer_type, 0);
  return fields;
}

MpplCharTypeSyntaxFields mppl_char_type_syntax_fields_alloc(const MpplArrayTypeSyntax *char_type)
{
  MpplCharTypeSyntaxFields fields;
  fields.char_kw = syntax_tree_child_token((const SyntaxTree *) char_type, 0);
  return fields;
}

MpplBooleanTypeSyntaxFields mppl_boolean_type_syntax_fields_alloc(const MpplArrayTypeSyntax *boolean_type)
{
  MpplBooleanTypeSyntaxFields fields;
  fields.boolean_kw = syntax_tree_child_token((const SyntaxTree *) boolean_type, 0);
  return fields;
}

MpplArrayTypeSyntaxFields mppl_array_type_syntax_fields_alloc(const MpplArrayTypeSyntax *array_type)
{
  MpplArrayTypeSyntaxFields fields;
  fields.array_kw       = syntax_tree_child_token((const SyntaxTree *) array_type, 0);
  fields.lbracket_token = syntax_tree_child_token((const SyntaxTree *) array_type, 1);
  fields.number_lit     = syntax_tree_child_token((const SyntaxTree *) array_type, 2);
  fields.rbracket_token = syntax_tree_child_token((const SyntaxTree *) array_type, 3);
  fields.of_kw          = syntax_tree_child_token((const SyntaxTree *) array_type, 4);
  fields.type           = any_mppl_type_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) array_type, 5));
  return fields;
}

MpplProcDeclPartSyntaxFields mppl_proc_decl_part_syntax_fields_alloc(const MpplProcDeclPartSyntax *proc_decl_part)
{
  MpplProcDeclPartSyntaxFields fields;
  fields.proc_decl  = mppl_proc_decl_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_decl_part, 0));
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) proc_decl_part, 1);
  return fields;
}

MpplProcHeadingSyntaxFields mppl_proc_heading_syntax_fields_alloc(const MpplProcHeadingSyntax *proc_heading)
{
  MpplProcHeadingSyntaxFields fields;
  fields.procedure_kw = syntax_tree_child_token((const SyntaxTree *) proc_heading, 0);
  fields.name         = syntax_tree_child_token((const SyntaxTree *) proc_heading, 1);
  fields.fml_params   = mppl_fml_params_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_heading, 2));
  return fields;
}

MpplProcBodySyntaxFields mppl_proc_body_syntax_fields_alloc(const MpplProcBodySyntax *proc_body)
{
  MpplProcBodySyntaxFields fields;
  fields.var_decl_part = mppl_var_decl_part_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_body, 0));
  fields.comp_stmt     = mppl_comp_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_body, 1));
  return fields;
}

MpplProcDeclSyntaxFields mppl_proc_decl_syntax_fields_alloc(const MpplProcDeclSyntax *proc_decl)
{
  MpplProcDeclSyntaxFields fields;
  fields.proc_heading = mppl_proc_heading_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_decl, 0));
  fields.semi_token   = syntax_tree_child_token((const SyntaxTree *) proc_decl, 1);
  fields.proc_body    = mppl_proc_body_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) proc_decl, 2));
  return fields;
}

MpplFmlParamListElemSyntaxFields mppl_fml_param_list_elem_syntax_fields_alloc(const MpplFmlParamListElemSyntax *fml_param_list_elem)
{
  MpplFmlParamListElemSyntaxFields fields;
  fields.fml_param_sec = mppl_fml_param_sec_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) fml_param_list_elem, 0));
  fields.semi_token    = syntax_tree_child_token((const SyntaxTree *) fml_param_list_elem, 1);
  return fields;
}

MpplFmlParamListSyntaxFields mppl_fml_param_list_syntax_fields_alloc(const MpplFmlParamListSyntax *fml_param_list)
{
  unsigned long i;

  MpplFmlParamListSyntaxFields fields;
  slice_alloc(&fields.fml_param_list_elems, fml_param_list->syntax.raw->children.count);
  for (i = 0; i * 2 < fml_param_list->syntax.raw->children.count; ++i) {
    fields.fml_param_list_elems.ptr[i] = mppl_fml_param_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) fml_param_list, i));
  }
  return fields;
}

MpplFmlParamsSyntaxFields mppl_fml_params_syntax_fields_alloc(const MpplFmlParamsSyntax *fml_params)
{
  MpplFmlParamsSyntaxFields fields;
  fields.lparen_token   = syntax_tree_child_token((const SyntaxTree *) fml_params, 0);
  fields.fml_param_list = mppl_fml_param_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) fml_params, 1));
  fields.rparen_token   = syntax_tree_child_token((const SyntaxTree *) fml_params, 2);
  return fields;
}

MpplFmlParamSecSyntaxFields mppl_fml_param_sec_syntax_fields_alloc(const MpplFmlParamSecSyntax *fml_param_sec)
{
  MpplFmlParamSecSyntaxFields fields;
  fields.ident_list  = mppl_bind_ident_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) fml_param_sec, 0));
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) fml_param_sec, 1);
  fields.type        = any_mppl_type_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) fml_param_sec, 2));
  return fields;
}

MpplStmtListElemSyntaxFields mppl_stmt_list_elem_syntax_fields_alloc(const MpplStmtListElemSyntax *stmt_list_elem)
{
  MpplStmtListElemSyntaxFields fields;
  fields.stmt       = any_mppl_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) stmt_list_elem, 0));
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) stmt_list_elem, 1);
  return fields;
}

MpplStmtListSyntaxFields mppl_stmt_list_syntax_fields_alloc(const MpplStmtListSyntax *stmt_list)
{
  unsigned long i;

  MpplStmtListSyntaxFields fields;
  slice_alloc(&fields.stmt_list_elems, stmt_list->syntax.raw->children.count);
  for (i = 0; i * 2 < stmt_list->syntax.raw->children.count; ++i) {
    fields.stmt_list_elems.ptr[i] = mppl_stmt_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) stmt_list, i));
  }
  return fields;
}

MpplAssignStmtSyntaxFields mppl_assign_stmt_syntax_fields_alloc(const MpplAssignStmtSyntax *assign_stmt)
{
  MpplAssignStmtSyntaxFields fields;
  fields.lhs          = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) assign_stmt, 0));
  fields.assign_token = syntax_tree_child_token((const SyntaxTree *) assign_stmt, 1);
  fields.rhs          = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) assign_stmt, 2));
  return fields;
}

MpplIfStmtSyntaxFields mppl_if_stmt_syntax_fields_alloc(const MpplIfStmtSyntax *if_stmt)
{
  MpplIfStmtSyntaxFields fields;
  fields.if_kw       = syntax_tree_child_token((const SyntaxTree *) if_stmt, 0);
  fields.cond        = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) if_stmt, 1));
  fields.then_kw     = syntax_tree_child_token((const SyntaxTree *) if_stmt, 2);
  fields.then_stmt   = any_mppl_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) if_stmt, 3));
  fields.else_clause = mppl_else_clause_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) if_stmt, 4));
  return fields;
}

MpplElseClauseSyntaxFields mppl_else_clause_syntax_fields_alloc(const MpplElseClauseSyntax *else_clause)
{
  MpplElseClauseSyntaxFields fields;
  fields.else_kw   = syntax_tree_child_token((const SyntaxTree *) else_clause, 0);
  fields.else_stmt = any_mppl_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) else_clause, 1));
  return fields;
}

MpplWhileStmtSyntaxFields mppl_while_stmt_syntax_fields_alloc(const MpplWhileStmtSyntax *while_stmt)
{
  MpplWhileStmtSyntaxFields fields;
  fields.while_kw = syntax_tree_child_token((const SyntaxTree *) while_stmt, 0);
  fields.cond     = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) while_stmt, 1));
  fields.do_kw    = syntax_tree_child_token((const SyntaxTree *) while_stmt, 2);
  fields.stmt     = any_mppl_stmt_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) while_stmt, 3));
  return fields;
}

MpplBreakStmtSyntaxFields mppl_break_stmt_syntax_fields_alloc(const MpplBreakStmtSyntax *break_stmt)
{
  MpplBreakStmtSyntaxFields fields;
  fields.break_kw = syntax_tree_child_token((const SyntaxTree *) break_stmt, 0);
  return fields;
}

MpplCallStmtSyntaxFields mppl_call_stmt_syntax_fields_alloc(const MpplCallStmtSyntax *call_stmt)
{
  MpplCallStmtSyntaxFields fields;
  fields.call_kw    = syntax_tree_child_token((const SyntaxTree *) call_stmt, 0);
  fields.name       = syntax_tree_child_token((const SyntaxTree *) call_stmt, 1);
  fields.act_params = mppl_act_params_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) call_stmt, 2));
  return fields;
}

MpplActParamsSyntaxFields mppl_act_params_syntax_fields_alloc(const MpplActParamsSyntax *act_params)
{
  MpplActParamsSyntaxFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) act_params, 0);
  fields.expr_list    = mppl_expr_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) act_params, 1));
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) act_params, 2);
  return fields;
}

MpplReturnStmtSyntaxFields mppl_return_stmt_syntax_fields_alloc(const MpplReturnStmtSyntax *return_stmt)
{
  MpplReturnStmtSyntaxFields fields;
  fields.return_kw = syntax_tree_child_token((const SyntaxTree *) return_stmt, 0);
  return fields;
}

MpplInputStmtSyntaxFields mppl_input_stmt_syntax_fields_alloc(const MpplInputStmtSyntax *input_stmt)
{
  MpplInputStmtSyntaxFields fields;
  fields.read_op_token = syntax_tree_child_token((const SyntaxTree *) input_stmt, 0);
  fields.inputs        = mppl_inputs_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) input_stmt, 1));
  return fields;
}

MpplInputsSyntaxFields mppl_inputs_syntax_fields_alloc(const MpplInputsSyntax *inputs)
{
  MpplInputsSyntaxFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) inputs, 0);
  fields.expr_list    = mppl_expr_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) inputs, 1));
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) inputs, 2);
  return fields;
}

MpplOutputStmtSyntaxFields mppl_output_stmt_syntax_fields_alloc(const MpplOutputStmtSyntax *output_stmt)
{
  MpplOutputStmtSyntaxFields fields;
  fields.write_op_token = syntax_tree_child_token((const SyntaxTree *) output_stmt, 0);
  fields.outputs        = mppl_outputs_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) output_stmt, 1));
  return fields;
}

MpplOutputListElemSyntaxFields mppl_output_list_elem_syntax_fields_alloc(const MpplOutputListElemSyntax *output_list_elem)
{
  MpplOutputListElemSyntaxFields fields;
  fields.output      = any_mppl_output_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) output_list_elem, 0));
  fields.comma_token = syntax_tree_child_token((const SyntaxTree *) output_list_elem, 1);
  return fields;
}

MpplOutputListSyntaxFields mppl_output_list_syntax_fields_alloc(const MpplOutputListSyntax *output_list)
{
  unsigned long i;

  MpplOutputListSyntaxFields fields;
  slice_alloc(&fields.output_list_elems, output_list->syntax.raw->children.count);
  for (i = 0; i * 2 < output_list->syntax.raw->children.count; ++i) {
    fields.output_list_elems.ptr[i] = mppl_output_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) output_list, i));
  }
  return fields;
}

MpplOutputsSyntaxFields mppl_outputs_syntax_fields_alloc(const MpplOutputsSyntax *outputs)
{
  MpplOutputsSyntaxFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) outputs, 0);
  fields.output_list  = mppl_output_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) outputs, 1));
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) outputs, 2);
  return fields;
}

MpplOutputValueSyntaxFields mppl_output_value_syntax_fields_alloc(const MpplOutputValueSyntax *output_value)
{
  MpplOutputValueSyntaxFields fields;
  fields.expr        = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) output_value, 0));
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) output_value, 1);
  fields.number_lit  = syntax_tree_child_token((const SyntaxTree *) output_value, 2);
  return fields;
}

MpplCompStmtSyntaxFields mppl_comp_stmt_syntax_fields_alloc(const MpplCompStmtSyntax *comp_stmt)
{
  MpplCompStmtSyntaxFields fields;
  fields.begin_kw  = syntax_tree_child_token((const SyntaxTree *) comp_stmt, 0);
  fields.stmt_list = mppl_stmt_list_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) comp_stmt, 1));
  fields.end_kw    = syntax_tree_child_token((const SyntaxTree *) comp_stmt, 2);
  return fields;
}

MpplExprListElemSyntaxFields mppl_expr_list_elem_syntax_fields_alloc(const MpplExprListElemSyntax *expr_list_elem)
{
  MpplExprListElemSyntaxFields fields;
  fields.expr        = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) expr_list_elem, 0));
  fields.comma_token = syntax_tree_child_token((const SyntaxTree *) expr_list_elem, 1);
  return fields;
}

MpplExprListSyntaxFields mppl_expr_list_syntax_fields_alloc(const MpplExprListSyntax *expr_list)
{
  unsigned long i;

  MpplExprListSyntaxFields fields;
  slice_alloc(&fields.expr_list_elems, expr_list->syntax.raw->children.count);
  for (i = 0; i * 2 < expr_list->syntax.raw->children.count; ++i) {
    fields.expr_list_elems.ptr[i] = mppl_expr_list_elem_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) expr_list, i));
  }
  return fields;
}

MpplRefIdentSyntaxFields mppl_ref_ident_syntax_fields_alloc(const MpplRefIdentSyntax *ref_ident)
{
  MpplRefIdentSyntaxFields fields;
  fields.ident = syntax_tree_child_token((const SyntaxTree *) ref_ident, 0);
  return fields;
}

MpplEntireVarSyntaxFields mppl_entire_var_syntax_fields_alloc(const MpplEntireVarSyntax *entire_var)
{
  MpplEntireVarSyntaxFields fields;
  fields.name = mppl_ref_ident_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) entire_var, 0));
  return fields;
}

MpplIndexedVarSyntaxFields mppl_indexed_var_syntax_fields_alloc(const MpplIndexedVarSyntax *indexed_var)
{
  MpplIndexedVarSyntaxFields fields;
  fields.name           = mppl_ref_ident_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) indexed_var, 0));
  fields.lbracket_token = syntax_tree_child_token((const SyntaxTree *) indexed_var, 1);
  fields.index          = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) indexed_var, 2));
  fields.rbracket_token = syntax_tree_child_token((const SyntaxTree *) indexed_var, 3);
  return fields;
}

MpplUnaryExprSyntaxFields mppl_unary_expr_syntax_fields_alloc(const MpplUnaryExprSyntax *unary_expr)
{
  MpplUnaryExprSyntaxFields fields;
  fields.op_token = syntax_tree_child_token((const SyntaxTree *) unary_expr, 0);
  fields.expr     = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) unary_expr, 1));
  return fields;
}

MpplBinaryExprSyntaxFields mppl_binary_expr_syntax_fields_alloc(const MpplBinaryExprSyntax *binary_expr)
{
  MpplBinaryExprSyntaxFields fields;
  fields.lhs      = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) binary_expr, 0));
  fields.op_token = syntax_tree_child_token((const SyntaxTree *) binary_expr, 1);
  fields.rhs      = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) binary_expr, 2));
  return fields;
}

MpplParenExprSyntaxFields mppl_paren_expr_syntax_fields_alloc(const MpplParenExprSyntax *paren_expr)
{
  MpplParenExprSyntaxFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) paren_expr, 0);
  fields.expr         = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) paren_expr, 1));
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) paren_expr, 2);
  return fields;
}

MpplCastExprSyntaxFields mppl_cast_expr_syntax_fields_alloc(const MpplCastExprSyntax *cast_expr)
{
  MpplCastExprSyntaxFields fields;
  fields.type         = any_mppl_type_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) cast_expr, 0));
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) cast_expr, 1);
  fields.expr         = any_mppl_expr_syntax_alloc(syntax_tree_child_tree((const SyntaxTree *) cast_expr, 2));
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) cast_expr, 3);
  return fields;
}

void mppl_root_syntax_fields_free(MpplRootSyntaxFields *fields)
{
  if (fields) {
    mppl_program_syntax_free(fields->program);
    any_mppl_eof_syntax_free(fields->eof);
  }
}

void mppl_eof_syntax_fields_free(MpplEofSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->eof_token);
  }
}

void mppl_program_syntax_fields_free(MpplProgramSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->program_kw);
    syntax_token_free(fields->name);
    syntax_token_free(fields->semi_token);
    mppl_decl_part_list_syntax_free(fields->decl_part_list);
    mppl_comp_stmt_syntax_free(fields->comp_stmt);
    syntax_token_free(fields->dot_token);
  }
}

void mppl_decl_part_list_syntax_fields_free(MpplDeclPartListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->decl_parts.count; ++i) {
      any_mppl_decl_part_syntax_free(fields->decl_parts.ptr[i]);
    }
    slice_free(&fields->decl_parts);
  }
}

void mppl_bind_ident_list_elem_syntax_fields_free(MpplBindIdentListElemSyntaxFields *fields)
{
  if (fields) {
    any_mppl_bind_ident_syntax_free(fields->bind_ident);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_bind_ident_list_syntax_fields_free(MpplBindIdentListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->bind_ident_list_elems.count; ++i) {
      mppl_bind_ident_list_elem_syntax_free(fields->bind_ident_list_elems.ptr[i]);
    }
    slice_free(&fields->bind_ident_list_elems);
  }
}

void mppl_bind_ident_syntax_fields_free(MpplBindIdentSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->ident);
  }
}

void mppl_var_decl_part_syntax_fields_free(MpplVarDeclPartSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->var_kw);
    mppl_var_decl_list_syntax_free(fields->var_decl_list);
  }
}

void mppl_var_decl_list_elem_syntax_fields_free(MpplVarDeclListElemSyntaxFields *fields)
{
  if (fields) {
    mppl_var_decl_syntax_free(fields->var_decl);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_var_decl_list_syntax_fields_free(MpplVarDeclListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->var_decl_list_elems.count; ++i) {
      mppl_var_decl_list_elem_syntax_free(fields->var_decl_list_elems.ptr[i]);
    }
    slice_free(&fields->var_decl_list_elems);
  }
}

void mppl_var_decl_syntax_fields_free(MpplVarDeclSyntaxFields *fields)
{
  if (fields) {
    mppl_bind_ident_list_syntax_free(fields->ident_list);
    syntax_token_free(fields->colon_token);
    any_mppl_type_syntax_free(fields->type);
  }
}

void mppl_integer_type_syntax_fields_free(MpplIntegerTypeSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->integer_kw);
  }
}

void mppl_char_type_syntax_fields_free(MpplCharTypeSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->char_kw);
  }
}

void mppl_boolean_type_syntax_fields_free(MpplBooleanTypeSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->boolean_kw);
  }
}

void mppl_array_type_syntax_fields_free(MpplArrayTypeSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->array_kw);
    syntax_token_free(fields->lbracket_token);
    syntax_token_free(fields->number_lit);
    syntax_token_free(fields->rbracket_token);
    syntax_token_free(fields->of_kw);
    any_mppl_type_syntax_free(fields->type);
  }
}

void mppl_proc_decl_part_syntax_fields_free(MpplProcDeclPartSyntaxFields *fields)
{
  if (fields) {
    mppl_proc_decl_syntax_free(fields->proc_decl);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_proc_heading_syntax_fields_free(MpplProcHeadingSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->procedure_kw);
    syntax_token_free(fields->name);
    mppl_fml_params_syntax_free(fields->fml_params);
  }
}

void mppl_proc_body_syntax_fields_free(MpplProcBodySyntaxFields *fields)
{
  if (fields) {
    mppl_var_decl_part_syntax_free(fields->var_decl_part);
    mppl_comp_stmt_syntax_free(fields->comp_stmt);
  }
}

void mppl_proc_decl_syntax_fields_free(MpplProcDeclSyntaxFields *fields)
{
  if (fields) {
    mppl_proc_heading_syntax_free(fields->proc_heading);
    syntax_token_free(fields->semi_token);
    mppl_proc_body_syntax_free(fields->proc_body);
  }
}

void mppl_fml_param_list_elem_syntax_fields_free(MpplFmlParamListElemSyntaxFields *fields)
{
  if (fields) {
    mppl_fml_param_sec_syntax_free(fields->fml_param_sec);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_fml_param_list_syntax_fields_free(MpplFmlParamListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->fml_param_list_elems.count; ++i) {
      mppl_fml_param_list_elem_syntax_free(fields->fml_param_list_elems.ptr[i]);
    }
    slice_free(&fields->fml_param_list_elems);
  }
}

void mppl_fml_params_syntax_fields_free(MpplFmlParamsSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    mppl_fml_param_list_syntax_free(fields->fml_param_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_fml_param_sec_syntax_fields_free(MpplFmlParamSecSyntaxFields *fields)
{
  if (fields) {
    mppl_bind_ident_list_syntax_free(fields->ident_list);
    syntax_token_free(fields->colon_token);
    any_mppl_type_syntax_free(fields->type);
  }
}

void mppl_stmt_list_elem_syntax_fields_free(MpplStmtListElemSyntaxFields *fields)
{
  if (fields) {
    any_mppl_stmt_syntax_free(fields->stmt);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_stmt_list_syntax_fields_free(MpplStmtListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->stmt_list_elems.count; ++i) {
      mppl_stmt_list_elem_syntax_free(fields->stmt_list_elems.ptr[i]);
    }
    slice_free(&fields->stmt_list_elems);
  }
}

void mppl_assign_stmt_syntax_fields_free(MpplAssignStmtSyntaxFields *fields)
{
  if (fields) {
    any_mppl_expr_syntax_free(fields->lhs);
    syntax_token_free(fields->assign_token);
    any_mppl_expr_syntax_free(fields->rhs);
  }
}

void mppl_if_stmt_syntax_fields_free(MpplIfStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->if_kw);
    any_mppl_expr_syntax_free(fields->cond);
    syntax_token_free(fields->then_kw);
    any_mppl_stmt_syntax_free(fields->then_stmt);
    mppl_else_clause_syntax_free(fields->else_clause);
  }
}

void mppl_else_clause_syntax_fields_free(MpplElseClauseSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->else_kw);
    any_mppl_stmt_syntax_free(fields->else_stmt);
  }
}

void mppl_while_stmt_syntax_fields_free(MpplWhileStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->while_kw);
    any_mppl_expr_syntax_free(fields->cond);
    syntax_token_free(fields->do_kw);
    any_mppl_stmt_syntax_free(fields->stmt);
  }
}

void mppl_break_stmt_syntax_fields_free(MpplBreakStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->break_kw);
  }
}

void mppl_call_stmt_syntax_fields_free(MpplCallStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->call_kw);
    syntax_token_free(fields->name);
    mppl_act_params_syntax_free(fields->act_params);
  }
}

void mppl_act_params_syntax_fields_free(MpplActParamsSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    mppl_expr_list_syntax_free(fields->expr_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_return_stmt_syntax_fields_free(MpplReturnStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->return_kw);
  }
}

void mppl_input_stmt_syntax_fields_free(MpplInputStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->read_op_token);
    mppl_inputs_syntax_free(fields->inputs);
  }
}

void mppl_inputs_syntax_fields_free(MpplInputsSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    mppl_expr_list_syntax_free(fields->expr_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_output_stmt_syntax_fields_free(MpplOutputStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->write_op_token);
    mppl_outputs_syntax_free(fields->outputs);
  }
}

void mppl_output_list_elem_syntax_fields_free(MpplOutputListElemSyntaxFields *fields)
{
  if (fields) {
    any_mppl_output_syntax_free(fields->output);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_output_list_syntax_fields_free(MpplOutputListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->output_list_elems.count; ++i) {
      mppl_output_list_elem_syntax_free(fields->output_list_elems.ptr[i]);
    }
    slice_free(&fields->output_list_elems);
  }
}

void mppl_outputs_syntax_fields_free(MpplOutputsSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    mppl_output_list_syntax_free(fields->output_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_output_value_syntax_fields_free(MpplOutputValueSyntaxFields *fields)
{
  if (fields) {
    any_mppl_expr_syntax_free(fields->expr);
    syntax_token_free(fields->colon_token);
    syntax_token_free(fields->number_lit);
  }
}

void mppl_comp_stmt_syntax_fields_free(MpplCompStmtSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->begin_kw);
    mppl_stmt_list_syntax_free(fields->stmt_list);
    syntax_token_free(fields->end_kw);
  }
}

void mppl_expr_list_elem_syntax_fields_free(MpplExprListElemSyntaxFields *fields)
{
  if (fields) {
    any_mppl_expr_syntax_free(fields->expr);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_expr_list_syntax_fields_free(MpplExprListSyntaxFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->expr_list_elems.count; ++i) {
      mppl_expr_list_elem_syntax_free(fields->expr_list_elems.ptr[i]);
    }
    slice_free(&fields->expr_list_elems);
  }
}

void mppl_ref_ident_syntax_fields_free(MpplRefIdentSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->ident);
  }
}

void mppl_entire_var_syntax_fields_free(MpplEntireVarSyntaxFields *fields)
{
  if (fields) {
    mppl_ref_ident_syntax_free(fields->name);
  }
}

void mppl_indexed_var_syntax_fields_free(MpplIndexedVarSyntaxFields *fields)
{
  if (fields) {
    mppl_ref_ident_syntax_free(fields->name);
    syntax_token_free(fields->lbracket_token);
    any_mppl_expr_syntax_free(fields->index);
    syntax_token_free(fields->rbracket_token);
  }
}

void mppl_unary_expr_syntax_fields_free(MpplUnaryExprSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->op_token);
    any_mppl_expr_syntax_free(fields->expr);
  }
}

void mppl_binary_expr_syntax_fields_free(MpplBinaryExprSyntaxFields *fields)
{
  if (fields) {
    any_mppl_expr_syntax_free(fields->lhs);
    syntax_token_free(fields->op_token);
    any_mppl_expr_syntax_free(fields->rhs);
  }
}

void mppl_paren_expr_syntax_fields_free(MpplParenExprSyntaxFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    any_mppl_expr_syntax_free(fields->expr);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_cast_expr_syntax_fields_free(MpplCastExprSyntaxFields *fields)
{
  if (fields) {
    any_mppl_type_syntax_free(fields->type);
    syntax_token_free(fields->lparen_token);
    any_mppl_expr_syntax_free(fields->expr);
    syntax_token_free(fields->rparen_token);
  }
}
