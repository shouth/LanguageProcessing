/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include "mppl_syntax.h"

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
  case MPPL_SYNTAX_END_OF_FILE:
    return "END_OF_FILE";
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
  case MPPL_SYNTAX_PROGRAM:
    return "PROGRAM";
  case MPPL_SYNTAX_DECL_LIST:
    return "DECL_LIST";
  case MPPL_SYNTAX_VAR_DECL_PART:
    return "VAR_DECL_PART";
  case MPPL_SYNTAX_VAR_DECL_LIST_ELEM:
    return "VAR_DECL_LIST_ELEM";
  case MPPL_SYNTAX_VAR_DECL_LIST:
    return "VAR_DECL_LIST";
  case MPPL_SYNTAX_VAR_DECL:
    return "VAR_DECL";
  case MPPL_SYNTAX_ARRAY_TYPE:
    return "ARRAY_TYPE";
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
  case MPPL_SYNTAX_IDENT_LIST_ELEM:
    return "IDENT_LIST_ELEM";
  case MPPL_SYNTAX_IDENT_LIST:
    return "IDENT_LIST";
  case MPPL_SYNTAX_BOGUS:
    return "BOGUS";
  default:
    unreachable();
  }
}
