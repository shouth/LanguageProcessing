/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include "mppl_syntax.h"
#include "syntax_tree.h"

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

MpplRootSyntax *mppl_root_syntax_alloc(SyntaxTree *syntax)
{
  if (!syntax->node.parent) {
    return (MpplRootSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplProgramSyntax *mppl_program_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PROGRAM) {
    return (MpplProgramSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplEofSyntax *mppl_eof_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_EOF) {
    return (MpplEofSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplDeclPartListSyntax *mppl_decl_part_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_DECL_PART_LIST) {
    return (MpplDeclPartListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBindIdentListElemSyntax *mppl_bind_ident_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT_LIST_ELEM) {
    return (MpplBindIdentListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBindIdentListSyntax *mppl_bind_ident_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT_LIST) {
    return (MpplBindIdentListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBindIdentSyntax *mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT) {
    return (MpplBindIdentSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplVarDeclPartSyntax *mppl_var_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_PART) {
    return (MpplVarDeclPartSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplVarDeclListElemSyntax *mppl_var_decl_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_LIST_ELEM) {
    return (MpplVarDeclListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplVarDeclListSyntax *mppl_var_decl_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL_LIST) {
    return (MpplVarDeclListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplVarDeclSyntax *mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_VAR_DECL) {
    return (MpplVarDeclSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplIntegerTypeSyntax *mppl_integer_type_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_INTEGER_TYPE) {
    return (MpplIntegerTypeSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplCharTypeSyntax *mppl_char_type_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_CHAR_TYPE) {
    return (MpplCharTypeSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBooleanTypeSyntax *mppl_boolean_type_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOOLEAN_TYPE) {
    return (MpplBooleanTypeSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplArrayTypeSyntax *mppl_array_type_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_ARRAY_TYPE) {
    return (MpplArrayTypeSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplProcDeclPartSyntax *mppl_proc_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PROC_DECL_PART) {
    return (MpplProcDeclPartSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplProcHeadingSyntax *mppl_proc_heading_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PROC_HEADING) {
    return (MpplProcHeadingSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplProcBodySyntax *mppl_proc_body_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PROC_BODY) {
    return (MpplProcBodySyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplProcDeclSyntax *mppl_proc_decl_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PROC_DECL) {
    return (MpplProcDeclSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplFmlParamListElemSyntax *mppl_fml_param_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_LIST_ELEM) {
    return (MpplFmlParamListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplFmlParamListSyntax *mppl_fml_param_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_LIST) {
    return (MpplFmlParamListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplFmlParamsSyntax *mppl_fml_params_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAMS) {
    return (MpplFmlParamsSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplFmlParamSecSyntax *mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_SEC) {
    return (MpplFmlParamSecSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplStmtListElemSyntax *mppl_stmt_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_STMT_LIST_ELEM) {
    return (MpplStmtListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplStmtListSyntax *mppl_stmt_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_STMT_LIST) {
    return (MpplStmtListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplAssignStmtSyntax *mppl_assign_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_ASSIGN_STMT) {
    return (MpplAssignStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplIfStmtSyntax *mppl_if_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_IF_STMT) {
    return (MpplIfStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplElseClauseSyntax *mppl_else_clause_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_ELSE_CLAUSE) {
    return (MpplElseClauseSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplWhileStmtSyntax *mppl_while_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_WHILE_STMT) {
    return (MpplWhileStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBreakStmtSyntax *mppl_break_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BREAK_STMT) {
    return (MpplBreakStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplCallStmtSyntax *mppl_call_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_CALL_STMT) {
    return (MpplCallStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplActParamsSyntax *mppl_act_params_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_ACT_PARAMS) {
    return (MpplActParamsSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplReturnStmtSyntax *mppl_return_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_RETURN_STMT) {
    return (MpplReturnStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplInputStmtSyntax *mppl_input_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_INPUT_STMT) {
    return (MpplInputStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplInputsSyntax *mppl_inputs_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_INPUTS) {
    return (MpplInputsSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplOutputStmtSyntax *mppl_output_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_STMT) {
    return (MpplOutputStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplOutputListElemSyntax *mppl_output_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_LIST_ELEM) {
    return (MpplOutputListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplOutputListSyntax *mppl_output_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_LIST) {
    return (MpplOutputListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplOutputsSyntax *mppl_outputs_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_OUTPUTS) {
    return (MpplOutputsSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplOutputValueSyntax *mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_OUTPUT_VALUE) {
    return (MpplOutputValueSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplCompStmtSyntax *mppl_comp_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_COMP_STMT) {
    return (MpplCompStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplExprListElemSyntax *mppl_expr_list_elem_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_EXPR_LIST_ELEM) {
    return (MpplExprListElemSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplExprListSyntax *mppl_expr_list_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_EXPR_LIST) {
    return (MpplExprListSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplRefIdentSyntax *mppl_ref_ident_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_REF_IDENT) {
    return (MpplRefIdentSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplEntireVarSyntax *mppl_entire_var_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_ENTIRE_VAR) {
    return (MpplEntireVarSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplIndexedVarSyntax *mppl_indexed_var_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_INDEXED_VAR) {
    return (MpplIndexedVarSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplUnaryExprSyntax *mppl_unary_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_UNARY_EXPR) {
    return (MpplUnaryExprSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplBinaryExprSyntax *mppl_binary_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BINARY_EXPR) {
    return (MpplBinaryExprSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplParenExprSyntax *mppl_paren_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_PAREN_EXPR) {
    return (MpplParenExprSyntax *) syntax;
  } else {
    return NULL;
  }
}

MpplCastExprSyntax *mppl_cast_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_CAST_EXPR) {
    return (MpplCastExprSyntax *) syntax;
  } else {
    return NULL;
  }
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
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_EOF) {
    return (BogusMpplEofSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplDeclPartSyntax *bogus_mppl_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_DECL_PART) {
    return (BogusMpplDeclPartSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplVarDeclSyntax *bogus_mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_VAR_DECL) {
    return (BogusMpplVarDeclSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplFmlParamSecSyntax *bogus_mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_FML_PARAM_SEC) {
    return (BogusMpplFmlParamSecSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplStmtSyntax *bogus_mppl_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_STMT) {
    return (BogusMpplStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplOutputValueSyntax *bogus_mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_OUTPUT_VALUE) {
    return (BogusMpplOutputValueSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplExprSyntax *bogus_mppl_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_EXPR) {
    return (BogusMpplExprSyntax *) syntax;
  } else {
    return NULL;
  }
}

BogusMpplBindIdentSyntax *bogus_mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  if (syntax->raw->node.kind == MPPL_SYNTAX_BOGUS_BIND_IDENT) {
    return (BogusMpplBindIdentSyntax *) syntax;
  } else {
    return NULL;
  }
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
  if (eof_kind(syntax) != -1) {
    return (AnyMpplEofSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplDeclPartSyntax *any_mppl_decl_part_syntax_alloc(SyntaxTree *syntax)
{
  if (decl_part_kind(syntax) != -1) {
    return (AnyMpplDeclPartSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplTypeSyntax *any_mppl_type_syntax_alloc(SyntaxTree *syntax)
{
  if (type_kind(syntax) != -1) {
    return (AnyMpplTypeSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplVarDeclSyntax *any_mppl_var_decl_syntax_alloc(SyntaxTree *syntax)
{
  if (var_decl_kind(syntax) != -1) {
    return (AnyMpplVarDeclSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplFmlParamSecSyntax *any_mppl_fml_param_sec_syntax_alloc(SyntaxTree *syntax)
{
  if (fml_param_sec_kind(syntax) != -1) {
    return (AnyMpplFmlParamSecSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplStmtSyntax *any_mppl_stmt_syntax_alloc(SyntaxTree *syntax)
{
  if (stmt_kind(syntax) != -1) {
    return (AnyMpplStmtSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplOutputSyntax *any_mppl_output_syntax_alloc(SyntaxTree *syntax)
{
  if (output_kind(syntax) != -1) {
    return (AnyMpplOutputSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplOutputValueSyntax *any_mppl_output_value_syntax_alloc(SyntaxTree *syntax)
{
  if (output_value_kind(syntax) != -1) {
    return (AnyMpplOutputValueSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplVarSyntax *any_mppl_var_syntax_alloc(SyntaxTree *syntax)
{
  if (var_kind(syntax) != -1) {
    return (AnyMpplVarSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplExprSyntax *any_mppl_expr_syntax_alloc(SyntaxTree *syntax)
{
  if (expr_kind(syntax) != -1) {
    return (AnyMpplExprSyntax *) syntax;
  } else {
    return NULL;
  }
}

AnyMpplBindIdentSyntax *any_mppl_bind_ident_syntax_alloc(SyntaxTree *syntax)
{
  if (bind_ident_kind(syntax) != -1) {
    return (AnyMpplBindIdentSyntax *) syntax;
  } else {
    return NULL;
  }
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

MpplEofSyntaxFields               mppl_eof_syntax_fields_alloc(const MpplEofSyntax *eof);
MpplDeclPartListSyntaxFields      mppl_decl_part_list_syntax_fields_alloc(const MpplDeclPartListSyntax *decl_part_list);
MpplBindIdentListElemSyntaxFields mppl_bind_ident_list_elem_syntax_fields_alloc(const MpplBindIdentListElemSyntax *bind_ident_list_elem);
MpplBindIdentListSyntaxFields     mppl_bind_ident_list_syntax_fields_alloc(const MpplBindIdentListSyntax *bind_ident_list);
MpplBindIdentSyntaxFields         mppl_bind_ident_syntax_fields_alloc(const MpplBindIdentSyntax *bind_ident);
MpplVarDeclPartSyntaxFields       mppl_var_decl_part_syntax_fields_alloc(const MpplVarDeclPartSyntax *var_decl_part);
MpplVarDeclListElemSyntaxFields   mppl_var_decl_list_elem_syntax_fields_alloc(const MpplVarDeclListElemSyntax *var_decl_list_elem);
MpplVarDeclListSyntaxFields       mppl_var_decl_list_syntax_fields_alloc(const MpplVarDeclListSyntax *var_decl_list);
MpplVarDeclSyntaxFields           mppl_var_decl_syntax_fields_alloc(const MpplVarDeclSyntax *var_decl);
MpplIntegerTypeSyntaxFields       mppl_integer_type_syntax_fields_alloc(const MpplArrayTypeSyntax *integer_type);
MpplCharTypeSyntaxFields          mppl_char_type_syntax_fields_alloc(const MpplArrayTypeSyntax *char_type);
MpplBooleanTypeSyntaxFields       mppl_boolean_type_syntax_fields_alloc(const MpplArrayTypeSyntax *boolean_type);
MpplArrayTypeSyntaxFields         mppl_array_type_syntax_fields_alloc(const MpplArrayTypeSyntax *array_type);
MpplProcDeclPartSyntaxFields      mppl_proc_decl_part_syntax_fields_alloc(const MpplProcDeclPartSyntax *proc_decl_part);
MpplProcHeadingSyntaxFields       mppl_proc_heading_syntax_fields_alloc(const MpplProcHeadingSyntax *proc_heading);
MpplProcBodySyntaxFields          mppl_proc_body_syntax_fields_alloc(const MpplProcBodySyntax *proc_body);
MpplProcDeclSyntaxFields          mppl_proc_decl_syntax_fields_alloc(const MpplProcDeclSyntax *proc_decl);
MpplFmlParamListElemSyntaxFields  mppl_fml_param_list_elem_syntax_fields_alloc(const MpplFmlParamListElemSyntax *fml_param_list_elem);
MpplFmlParamListSyntaxFields      mppl_fml_param_list_syntax_fields_alloc(const MpplFmlParamListSyntax *fml_param_list);
MpplFmlParamsSyntaxFields         mppl_fml_params_syntax_fields_alloc(const MpplFmlParamsSyntax *fml_params);
MpplFmlParamSecSyntaxFields       mppl_fml_param_sec_syntax_fields_alloc(const MpplFmlParamSecSyntax *fml_param_sec);
MpplStmtListElemSyntaxFields      mppl_stmt_list_elem_syntax_fields_alloc(const MpplStmtListElemSyntax *stmt_list_elem);
MpplStmtListSyntaxFields          mppl_stmt_list_syntax_fields_alloc(const MpplStmtListSyntax *stmt_list);
MpplAssignStmtSyntaxFields        mppl_assign_stmt_syntax_fields_alloc(const MpplAssignStmtSyntax *assign_stmt);
MpplIfStmtSyntaxFields            mppl_if_stmt_syntax_fields_alloc(const MpplIfStmtSyntax *if_stmt);
MpplElseClauseSyntaxFields        mppl_else_clause_syntax_fields_alloc(const MpplElseClauseSyntax *else_clause);
MpplWhileStmtSyntaxFields         mppl_while_stmt_syntax_fields_alloc(const MpplWhileStmtSyntax *while_stmt);
MpplBreakStmtSyntaxFields         mppl_break_stmt_syntax_fields_alloc(const MpplBreakStmtSyntax *break_stmt);
MpplCallStmtSyntaxFields          mppl_call_stmt_syntax_fields_alloc(const MpplCallStmtSyntax *call_stmt);
MpplActParamsSyntaxFields         mppl_act_params_syntax_fields_alloc(const MpplActParamsSyntax *act_params);
MpplReturnStmtSyntaxFields        mppl_return_stmt_syntax_fields_alloc(const MpplReturnStmtSyntax *return_stmt);
MpplInputStmtSyntaxFields         mppl_input_stmt_syntax_fields_alloc(const MpplInputStmtSyntax *input_stmt);
MpplInputsSyntaxFields            mppl_inputs_syntax_fields_alloc(const MpplInputsSyntax *inputs);
MpplOutputStmtSyntaxFields        mppl_output_stmt_syntax_fields_alloc(const MpplOutputStmtSyntax *output_stmt);
MpplOutputListElemSyntaxFields    mppl_output_list_elem_syntax_fields_alloc(const MpplOutputListElemSyntax *output_list_elem);
MpplOutputListSyntaxFields        mppl_output_list_syntax_fields_alloc(const MpplOutputListSyntax *output_list);
MpplOutputsSyntaxFields           mppl_outputs_syntax_fields_alloc(const MpplOutputsSyntax *outputs);
MpplOutputValueSyntaxFields       mppl_output_value_syntax_fields_alloc(const MpplOutputValueSyntax *output_value);
MpplCompStmtSyntaxFields          mppl_comp_stmt_syntax_fields_alloc(const MpplCompStmtSyntax *comp_stmt);
MpplExprListElemSyntaxFields      mppl_expr_list_elem_syntax_fields_alloc(const MpplExprListElemSyntax *expr_list_elem);
MpplExprListSyntaxFields          mppl_expr_list_syntax_fields_alloc(const MpplExprListSyntax *expr_list);
MpplRefIdentSyntaxFields          mppl_ref_ident_syntax_fields_alloc(const MpplRefIdentSyntax *ref_ident);
MpplEntireVarSyntaxFields         mppl_entire_var_syntax_fields_alloc(const MpplEntireVarSyntax *entire_var);
MpplIndexedVarSyntaxFields        mppl_indexed_var_syntax_fields_alloc(const MpplIndexedVarSyntax *indexed_var);
MpplUnaryExprSyntaxFields         mppl_unary_expr_syntax_fields_alloc(const MpplUnaryExprSyntax *unary_expr);
MpplBinaryExprSyntaxFields        mppl_binary_expr_syntax_fields_alloc(const MpplBinaryExprSyntax *binary_expr);
MpplParenExprSyntaxFields         mppl_paren_expr_syntax_fields_alloc(const MpplParenExprSyntax *paren_expr);
MpplCastExprSyntaxFields          mppl_cast_expr_syntax_fields_alloc(const MpplCastExprSyntax *cast_expr);

void mppl_root_syntax_fields_free(MpplRootSyntaxFields *fields);
void mppl_eof_syntax_fields_free(MpplEofSyntaxFields *fields);
void mppl_program_syntax_fields_free(MpplProgramSyntaxFields *fields);
void mppl_decl_part_list_syntax_fields_free(MpplDeclPartListSyntaxFields *fields);
void mppl_bind_ident_list_elem_syntax_fields_free(MpplBindIdentListElemSyntaxFields *fields);
void mppl_bind_ident_list_syntax_fields_free(MpplBindIdentListSyntaxFields *fields);
void mppl_bind_ident_syntax_fields_free(MpplBindIdentSyntaxFields *fields);
void mppl_var_decl_part_syntax_fields_free(MpplVarDeclPartSyntaxFields *fields);
void mppl_var_decl_list_elem_syntax_fields_free(MpplVarDeclListElemSyntaxFields *fields);
void mppl_var_decl_list_syntax_fields_free(MpplVarDeclListSyntaxFields *fields);
void mppl_var_decl_syntax_fields_free(MpplVarDeclSyntaxFields *fields);
void mppl_integer_type_syntax_fields_free(MpplIntegerTypeSyntaxFields *fields);
void mppl_char_type_syntax_fields_free(MpplCharTypeSyntaxFields *fields);
void mppl_boolean_type_syntax_fields_free(MpplArrayTypeSyntaxFields *fields);
void mppl_array_type_syntax_fields_free(MpplBooleanTypeSyntaxFields *fields);
void mppl_proc_decl_part_syntax_fields_free(MpplProcDeclPartSyntaxFields *fields);
void mppl_proc_heading_syntax_fields_free(MpplProcHeadingSyntaxFields *fields);
void mppl_proc_body_syntax_fields_free(MpplProcBodySyntaxFields *fields);
void mppl_proc_decl_syntax_fields_free(MpplProcDeclSyntaxFields *fields);
void mppl_fml_param_list_elem_syntax_fields_free(MpplFmlParamListElemSyntaxFields *fields);
void mppl_fml_param_list_syntax_fields_free(MpplFmlParamListSyntaxFields *fields);
void mppl_fml_params_syntax_fields_free(MpplFmlParamsSyntaxFields *fields);
void mppl_fml_param_sec_syntax_fields_free(MpplFmlParamSecSyntaxFields *fields);
void mppl_stmt_list_elem_syntax_fields_free(MpplStmtListElemSyntaxFields *fields);
void mppl_stmt_list_syntax_fields_free(MpplStmtListSyntaxFields *fields);
void mppl_assign_stmt_syntax_fields_free(MpplAssignStmtSyntaxFields *fields);
void mppl_if_stmt_syntax_fields_free(MpplIfStmtSyntaxFields *fields);
void mppl_else_clause_syntax_fields_free(MpplElseClauseSyntaxFields *fields);
void mppl_while_stmt_syntax_fields_free(MpplWhileStmtSyntaxFields *fields);
void mppl_break_stmt_syntax_fields_free(MpplBreakStmtSyntaxFields *fields);
void mppl_call_stmt_syntax_fields_free(MpplCallStmtSyntaxFields *fields);
void mppl_act_params_syntax_fields_free(MpplActParamsSyntaxFields *fields);
void mppl_return_stmt_syntax_fields_free(MpplReturnStmtSyntaxFields *fields);
void mppl_input_stmt_syntax_fields_free(MpplInputStmtSyntaxFields *fields);
void mppl_inputs_syntax_fields_free(MpplInputsSyntaxFields *fields);
void mppl_output_stmt_syntax_fields_free(MpplOutputStmtSyntaxFields *fields);
void mppl_output_list_elem_syntax_fields_free(MpplOutputListElemSyntaxFields *fields);
void mppl_output_list_syntax_fields_free(MpplOutputListSyntaxFields *fields);
void mppl_outputs_syntax_fields_free(MpplOutputsSyntaxFields *fields);
void mppl_output_value_syntax_fields_free(MpplOutputValueSyntaxFields *fields);
void mppl_comp_stmt_syntax_fields_free(MpplCompStmtSyntaxFields *fields);
void mppl_expr_list_elem_syntax_fields_free(MpplExprListElemSyntaxFields *fields);
void mppl_expr_list_syntax_fields_free(MpplExprListSyntaxFields *fields);
void mppl_ref_ident_syntax_fields_free(MpplRefIdentSyntaxFields *fields);
void mppl_entire_var_syntax_fields_free(MpplEntireVarSyntaxFields *fields);
void mppl_indexed_var_syntax_fields_free(MpplIndexedVarSyntaxFields *fields);
void mppl_unary_expr_syntax_fields_free(MpplUnaryExprSyntaxFields *fields);
void mppl_binary_expr_syntax_fields_free(MpplBinaryExprSyntaxFields *fields);
void mppl_paren_expr_syntax_fields_free(MpplParenExprSyntaxFields *fields);
void mppl_cast_expr_syntax_fields_free(MpplCastExprSyntaxFields *fields);
