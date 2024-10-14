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
  case MPPL_SYNTAX_INTEGER_LIT:
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
  case MPPL_SYNTAX_OUTPUT_VALUE_FIELD_WIDTH:
    return "OUTPUT_VALUE_FIELD_WIDTH";
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
  case MPPL_SYNTAX_INTEGER_LIT_EXPR:
    return "INTEGER_LIT_EXPR";
  case MPPL_SYNTAX_BOOLEAN_LIT_EXPR:
    return "BOOLEAN_LIT_EXPR";
  case MPPL_SYNTAX_STRING_LIT_EXPR:
    return "STRING_LIT_EXPR";
  case MPPL_SYNTAX_ENTIRE_VAR_EXPR:
    return "ENTIRE_VAR_EXPR";
  case MPPL_SYNTAX_INDEXED_VAR_EXPR:
    return "INDEXED_VAR_EXPR";
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

static int expr_kind(const SyntaxTree *syntax)
{
  switch (syntax->raw->node.kind) {
  case MPPL_SYNTAX_INTEGER_LIT_EXPR:
    return MPPL_EXPR_SYNTAX_INTEGER_LIT;
  case MPPL_SYNTAX_BOOLEAN_LIT_EXPR:
    return MPPL_EXPR_SYNTAX_BOOLEAN_LIT;
  case MPPL_SYNTAX_STRING_LIT_EXPR:
    return MPPL_EXPR_SYNTAX_STRING_LIT;
  case MPPL_SYNTAX_ENTIRE_VAR_EXPR:
    return MPPL_EXPR_SYNTAX_ENTIRE_VAR;
  case MPPL_SYNTAX_INDEXED_VAR_EXPR:
    return MPPL_EXPR_SYNTAX_INDEXED_VAR;
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
    return -1;
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

MpplEofKind mppl_eof_kind(const AnyMpplEof *eof)
{
  return eof_kind((const SyntaxTree *) eof);
}

MpplDeclPartKind mppl_decl_part_kind(const AnyMpplDeclPart *decl_part)
{
  return decl_part_kind((const SyntaxTree *) decl_part);
}

MpplTypeKind mppl_type_kind(const AnyMpplType *type)
{
  return type_kind((const SyntaxTree *) type);
}

MpplVarDeclKind mppl_var_decl_kind(const AnyMpplVarDecl *var_decl)
{
  return var_decl_kind((const SyntaxTree *) var_decl);
}

MpplFmlParamSecKind mppl_fml_param_sec_kind(const AnyMpplFmlParamSec *fml_param_sec)
{
  return fml_param_sec_kind((const SyntaxTree *) fml_param_sec);
}

MpplStmtKind mppl_stmt_kind(const AnyMpplStmt *stmt)
{
  return stmt_kind((const SyntaxTree *) stmt);
}

MpplOutputValueKind mppl_output_value_kind(const AnyMpplOutputValue *output_value)
{
  return output_value_kind((const SyntaxTree *) output_value);
}

MpplExprKind mppl_expr_kind(const AnyMpplExpr *expr)
{
  return expr_kind((const SyntaxTree *) expr);
}

MpplBindIdentKind mppl_bind_ident_kind(const AnyMpplBindIdent *bind_ident)
{
  return bind_ident_kind((const SyntaxTree *) bind_ident);
}

MpplRootFields mppl_root_fields_alloc(const MpplRoot *root)
{
  MpplRootFields fields;
  fields.program = (void *) syntax_tree_child_tree((const SyntaxTree *) root, 0);
  fields.eof     = (void *) syntax_tree_child_tree((const SyntaxTree *) root, 1);
  return fields;
}

MpplProgramFields mppl_program_fields_alloc(const MpplProgram *program)
{
  MpplProgramFields fields;
  fields.program_kw     = syntax_tree_child_token((const SyntaxTree *) program, 0);
  fields.name           = (void *) syntax_tree_child_tree((const SyntaxTree *) program, 1);
  fields.semi_token     = syntax_tree_child_token((const SyntaxTree *) program, 2);
  fields.decl_part_list = (void *) syntax_tree_child_tree((const SyntaxTree *) program, 3);
  fields.comp_stmt      = (void *) syntax_tree_child_tree((const SyntaxTree *) program, 4);
  fields.dot_token      = syntax_tree_child_token((const SyntaxTree *) program, 5);
  return fields;
}

MpplEofFields mppl_eof_fields_alloc(const MpplEof *eof)
{
  MpplEofFields fields;
  fields.eof_token = syntax_tree_child_token((const SyntaxTree *) eof, 0);
  return fields;
}

MpplDeclPartListFields mppl_decl_part_list_fields_alloc(const MpplDeclPartList *decl_part_list)
{
  unsigned long i;

  MpplDeclPartListFields fields;
  slice_alloc(&fields, decl_part_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) decl_part_list, i);
  }
  return fields;
}

MpplBindIdentListElemFields mppl_bind_ident_list_elem_fields_alloc(const MpplBindIdentListElem *bind_ident_list_elem)
{
  MpplBindIdentListElemFields fields;
  fields.bind_ident  = (void *) syntax_tree_child_tree((const SyntaxTree *) bind_ident_list_elem, 0);
  fields.comma_token = syntax_tree_child_token((const SyntaxTree *) bind_ident_list_elem, 1);
  return fields;
}

MpplBindIdentListFields mppl_bind_ident_list_fields_alloc(const MpplBindIdentList *bind_ident_list)
{
  unsigned long i;

  MpplBindIdentListFields fields;
  slice_alloc(&fields, bind_ident_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) bind_ident_list, i);
  }
  return fields;
}

MpplBindIdentFields mppl_bind_ident_fields_alloc(const MpplBindIdent *bind_ident)
{
  MpplBindIdentFields fields;
  fields.ident = syntax_tree_child_token((const SyntaxTree *) bind_ident, 0);
  return fields;
}

MpplVarDeclPartFields mppl_var_decl_part_fields_alloc(const MpplVarDeclPart *var_decl_part)
{
  MpplVarDeclPartFields fields;
  fields.var_kw        = syntax_tree_child_token((const SyntaxTree *) var_decl_part, 0);
  fields.var_decl_list = (void *) syntax_tree_child_tree((const SyntaxTree *) var_decl_part, 1);
  return fields;
}

MpplVarDeclListElemFields mppl_var_decl_list_elem_fields_alloc(const MpplVarDeclListElem *var_decl_list_elem)
{
  MpplVarDeclListElemFields fields;
  fields.var_decl   = (void *) syntax_tree_child_tree((const SyntaxTree *) var_decl_list_elem, 0);
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) var_decl_list_elem, 1);
  return fields;
}

MpplVarDeclListFields mppl_var_decl_list_fields_alloc(const MpplVarDeclList *var_decl_list)
{
  unsigned long i;

  MpplVarDeclListFields fields;
  slice_alloc(&fields, var_decl_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) var_decl_list, i);
  }
  return fields;
}

MpplVarDeclFields mppl_var_decl_fields_alloc(const MpplVarDecl *var_decl)
{
  MpplVarDeclFields fields;
  fields.ident_list  = (void *) syntax_tree_child_tree((const SyntaxTree *) var_decl, 0);
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) var_decl, 1);
  fields.type        = (void *) syntax_tree_child_tree((const SyntaxTree *) var_decl, 2);
  return fields;
}

MpplIntegerTypeFields mppl_integer_type_fields_alloc(const MpplArrayType *integer_type)
{
  MpplIntegerTypeFields fields;
  fields.integer_kw = syntax_tree_child_token((const SyntaxTree *) integer_type, 0);
  return fields;
}

MpplCharTypeFields mppl_char_type_fields_alloc(const MpplArrayType *char_type)
{
  MpplCharTypeFields fields;
  fields.char_kw = syntax_tree_child_token((const SyntaxTree *) char_type, 0);
  return fields;
}

MpplBooleanTypeFields mppl_boolean_type_fields_alloc(const MpplArrayType *boolean_type)
{
  MpplBooleanTypeFields fields;
  fields.boolean_kw = syntax_tree_child_token((const SyntaxTree *) boolean_type, 0);
  return fields;
}

MpplArrayTypeFields mppl_array_type_fields_alloc(const MpplArrayType *array_type)
{
  MpplArrayTypeFields fields;
  fields.array_kw       = syntax_tree_child_token((const SyntaxTree *) array_type, 0);
  fields.lbracket_token = syntax_tree_child_token((const SyntaxTree *) array_type, 1);
  fields.number_lit     = syntax_tree_child_token((const SyntaxTree *) array_type, 2);
  fields.rbracket_token = syntax_tree_child_token((const SyntaxTree *) array_type, 3);
  fields.of_kw          = syntax_tree_child_token((const SyntaxTree *) array_type, 4);
  fields.type           = (void *) syntax_tree_child_tree((const SyntaxTree *) array_type, 5);
  return fields;
}

MpplProcDeclPartFields mppl_proc_decl_part_fields_alloc(const MpplProcDeclPart *proc_decl_part)
{
  MpplProcDeclPartFields fields;
  fields.proc_decl  = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_decl_part, 0);
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) proc_decl_part, 1);
  return fields;
}

MpplProcHeadingFields mppl_proc_heading_fields_alloc(const MpplProcHeading *proc_heading)
{
  MpplProcHeadingFields fields;
  fields.procedure_kw = syntax_tree_child_token((const SyntaxTree *) proc_heading, 0);
  fields.name         = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_heading, 1);
  fields.fml_params   = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_heading, 2);
  return fields;
}

MpplProcBodyFields mppl_proc_body_fields_alloc(const MpplProcBody *proc_body)
{
  MpplProcBodyFields fields;
  fields.var_decl_part = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_body, 0);
  fields.comp_stmt     = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_body, 1);
  return fields;
}

MpplProcDeclFields mppl_proc_decl_fields_alloc(const MpplProcDecl *proc_decl)
{
  MpplProcDeclFields fields;
  fields.proc_heading = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_decl, 0);
  fields.semi_token   = syntax_tree_child_token((const SyntaxTree *) proc_decl, 1);
  fields.proc_body    = (void *) syntax_tree_child_tree((const SyntaxTree *) proc_decl, 2);
  return fields;
}

MpplFmlParamListElemFields mppl_fml_param_list_elem_fields_alloc(const MpplFmlParamListElem *fml_param_list_elem)
{
  MpplFmlParamListElemFields fields;
  fields.fml_param_sec = (void *) syntax_tree_child_tree((const SyntaxTree *) fml_param_list_elem, 0);
  fields.semi_token    = syntax_tree_child_token((const SyntaxTree *) fml_param_list_elem, 1);
  return fields;
}

MpplFmlParamListFields mppl_fml_param_list_fields_alloc(const MpplFmlParamList *fml_param_list)
{
  unsigned long i;

  MpplFmlParamListFields fields;
  slice_alloc(&fields, fml_param_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) fml_param_list, i);
  }
  return fields;
}

MpplFmlParamsFields mppl_fml_params_fields_alloc(const MpplFmlParams *fml_params)
{
  MpplFmlParamsFields fields;
  fields.lparen_token   = syntax_tree_child_token((const SyntaxTree *) fml_params, 0);
  fields.fml_param_list = (void *) syntax_tree_child_tree((const SyntaxTree *) fml_params, 1);
  fields.rparen_token   = syntax_tree_child_token((const SyntaxTree *) fml_params, 2);
  return fields;
}

MpplFmlParamSecFields mppl_fml_param_sec_fields_alloc(const MpplFmlParamSec *fml_param_sec)
{
  MpplFmlParamSecFields fields;
  fields.ident_list  = (void *) syntax_tree_child_tree((const SyntaxTree *) fml_param_sec, 0);
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) fml_param_sec, 1);
  fields.type        = (void *) syntax_tree_child_tree((const SyntaxTree *) fml_param_sec, 2);
  return fields;
}

MpplStmtListElemFields mppl_stmt_list_elem_fields_alloc(const MpplStmtListElem *stmt_list_elem)
{
  MpplStmtListElemFields fields;
  fields.stmt       = (void *) syntax_tree_child_tree((const SyntaxTree *) stmt_list_elem, 0);
  fields.semi_token = syntax_tree_child_token((const SyntaxTree *) stmt_list_elem, 1);
  return fields;
}

MpplStmtListFields mppl_stmt_list_fields_alloc(const MpplStmtList *stmt_list)
{
  unsigned long i;

  MpplStmtListFields fields;
  slice_alloc(&fields, stmt_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) stmt_list, i);
  }
  return fields;
}

MpplAssignStmtFields mppl_assign_stmt_fields_alloc(const MpplAssignStmt *assign_stmt)
{
  MpplAssignStmtFields fields;
  fields.lhs          = (void *) syntax_tree_child_tree((const SyntaxTree *) assign_stmt, 0);
  fields.assign_token = syntax_tree_child_token((const SyntaxTree *) assign_stmt, 1);
  fields.rhs          = (void *) syntax_tree_child_tree((const SyntaxTree *) assign_stmt, 2);
  return fields;
}

MpplIfStmtFields mppl_if_stmt_fields_alloc(const MpplIfStmt *if_stmt)
{
  MpplIfStmtFields fields;
  fields.if_kw       = syntax_tree_child_token((const SyntaxTree *) if_stmt, 0);
  fields.cond        = (void *) syntax_tree_child_tree((const SyntaxTree *) if_stmt, 1);
  fields.then_kw     = syntax_tree_child_token((const SyntaxTree *) if_stmt, 2);
  fields.then_stmt   = (void *) syntax_tree_child_tree((const SyntaxTree *) if_stmt, 3);
  fields.else_clause = (void *) syntax_tree_child_tree((const SyntaxTree *) if_stmt, 4);
  return fields;
}

MpplElseClauseFields mppl_else_clause_fields_alloc(const MpplElseClause *else_clause)
{
  MpplElseClauseFields fields;
  fields.else_kw   = syntax_tree_child_token((const SyntaxTree *) else_clause, 0);
  fields.else_stmt = (void *) syntax_tree_child_tree((const SyntaxTree *) else_clause, 1);
  return fields;
}

MpplWhileStmtFields mppl_while_stmt_fields_alloc(const MpplWhileStmt *while_stmt)
{
  MpplWhileStmtFields fields;
  fields.while_kw = syntax_tree_child_token((const SyntaxTree *) while_stmt, 0);
  fields.cond     = (void *) syntax_tree_child_tree((const SyntaxTree *) while_stmt, 1);
  fields.do_kw    = syntax_tree_child_token((const SyntaxTree *) while_stmt, 2);
  fields.stmt     = (void *) syntax_tree_child_tree((const SyntaxTree *) while_stmt, 3);
  return fields;
}

MpplBreakStmtFields mppl_break_stmt_fields_alloc(const MpplBreakStmt *break_stmt)
{
  MpplBreakStmtFields fields;
  fields.break_kw = syntax_tree_child_token((const SyntaxTree *) break_stmt, 0);
  return fields;
}

MpplCallStmtFields mppl_call_stmt_fields_alloc(const MpplCallStmt *call_stmt)
{
  MpplCallStmtFields fields;
  fields.call_kw    = syntax_tree_child_token((const SyntaxTree *) call_stmt, 0);
  fields.name       = (void *) syntax_tree_child_tree((const SyntaxTree *) call_stmt, 1);
  fields.act_params = (void *) syntax_tree_child_tree((const SyntaxTree *) call_stmt, 2);
  return fields;
}

MpplActParamsFields mppl_act_params_fields_alloc(const MpplActParams *act_params)
{
  MpplActParamsFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) act_params, 0);
  fields.expr_list    = (void *) syntax_tree_child_tree((const SyntaxTree *) act_params, 1);
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) act_params, 2);
  return fields;
}

MpplReturnStmtFields mppl_return_stmt_fields_alloc(const MpplReturnStmt *return_stmt)
{
  MpplReturnStmtFields fields;
  fields.return_kw = syntax_tree_child_token((const SyntaxTree *) return_stmt, 0);
  return fields;
}

MpplInputStmtFields mppl_input_stmt_fields_alloc(const MpplInputStmt *input_stmt)
{
  MpplInputStmtFields fields;
  fields.read_op_token = syntax_tree_child_token((const SyntaxTree *) input_stmt, 0);
  fields.inputs        = (void *) syntax_tree_child_tree((const SyntaxTree *) input_stmt, 1);
  return fields;
}

MpplInputsFields mppl_inputs_fields_alloc(const MpplInputs *inputs)
{
  MpplInputsFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) inputs, 0);
  fields.expr_list    = (void *) syntax_tree_child_tree((const SyntaxTree *) inputs, 1);
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) inputs, 2);
  return fields;
}

MpplOutputStmtFields mppl_output_stmt_fields_alloc(const MpplOutputStmt *output_stmt)
{
  MpplOutputStmtFields fields;
  fields.write_op_token = syntax_tree_child_token((const SyntaxTree *) output_stmt, 0);
  fields.outputs        = (void *) syntax_tree_child_tree((const SyntaxTree *) output_stmt, 1);
  return fields;
}

MpplOutputListElemFields mppl_output_list_elem_fields_alloc(const MpplOutputListElem *output_list_elem)
{
  MpplOutputListElemFields fields;
  fields.output_value = (void *) syntax_tree_child_tree((const SyntaxTree *) output_list_elem, 0);
  fields.comma_token  = syntax_tree_child_token((const SyntaxTree *) output_list_elem, 1);
  return fields;
}

MpplOutputListFields mppl_output_list_fields_alloc(const MpplOutputList *output_list)
{
  unsigned long i;

  MpplOutputListFields fields;
  slice_alloc(&fields, output_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) output_list, i);
  }
  return fields;
}

MpplOutputsFields mppl_outputs_fields_alloc(const MpplOutputs *outputs)
{
  MpplOutputsFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) outputs, 0);
  fields.output_list  = (void *) syntax_tree_child_tree((const SyntaxTree *) outputs, 1);
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) outputs, 2);
  return fields;
}

MpplOutputValueFieldWidthFields mppl_output_value_field_width_alloc(const MpplOutputValue *output_value)
{
  MpplOutputValueFieldWidthFields fields;
  fields.colon_token = syntax_tree_child_token((const SyntaxTree *) output_value, 0);
  fields.field_width = syntax_tree_child_token((const SyntaxTree *) output_value, 1);
  return fields;
}

MpplOutputValueFields mppl_output_value_fields_alloc(const MpplOutputValue *output_value)
{
  MpplOutputValueFields fields;
  fields.expr        = (void *) syntax_tree_child_tree((const SyntaxTree *) output_value, 0);
  fields.field_width = (void *) syntax_tree_child_tree((const SyntaxTree *) output_value, 1);
  return fields;
}

MpplCompStmtFields mppl_comp_stmt_fields_alloc(const MpplCompStmt *comp_stmt)
{
  MpplCompStmtFields fields;
  fields.begin_kw  = syntax_tree_child_token((const SyntaxTree *) comp_stmt, 0);
  fields.stmt_list = (void *) syntax_tree_child_tree((const SyntaxTree *) comp_stmt, 1);
  fields.end_kw    = syntax_tree_child_token((const SyntaxTree *) comp_stmt, 2);
  return fields;
}

MpplExprListElemFields mppl_expr_list_elem_fields_alloc(const MpplExprListElem *expr_list_elem)
{
  MpplExprListElemFields fields;
  fields.expr        = (void *) syntax_tree_child_tree((const SyntaxTree *) expr_list_elem, 0);
  fields.comma_token = syntax_tree_child_token((const SyntaxTree *) expr_list_elem, 1);
  return fields;
}

MpplExprListFields mppl_expr_list_fields_alloc(const MpplExprList *expr_list)
{
  unsigned long i;

  MpplExprListFields fields;
  slice_alloc(&fields, expr_list->syntax.raw->children.count / 2 + 1);
  for (i = 0; i < fields.count; ++i) {
    fields.ptr[i] = (void *) syntax_tree_child_tree((const SyntaxTree *) expr_list, i);
  }
  return fields;
}

MpplRefIdentFields mppl_ref_ident_fields_alloc(const MpplRefIdent *ref_ident)
{
  MpplRefIdentFields fields;
  fields.ident = syntax_tree_child_token((const SyntaxTree *) ref_ident, 0);
  return fields;
}

MpplIntegerLitExprFields mppl_integer_lit_expr_fields_alloc(const MpplIntegerLitExpr *integer_lit_expr)
{
  MpplIntegerLitExprFields fields;
  fields.integer_lit = syntax_tree_child_token((const SyntaxTree *) integer_lit_expr, 0);
  return fields;
}

MpplBooleanLitExprFields mppl_boolean_lit_expr_fields_alloc(const MpplBooleanLitExpr *boolean_lit_expr)
{
  MpplBooleanLitExprFields fields;
  fields.boolean_lit = syntax_tree_child_token((const SyntaxTree *) boolean_lit_expr, 0);
  return fields;
}

MpplStringLitExprFields mppl_string_lit_expr_fields_alloc(const MpplStringLitExpr *string_lit_expr)
{
  MpplStringLitExprFields fields;
  fields.string_lit = syntax_tree_child_token((const SyntaxTree *) string_lit_expr, 0);
  return fields;
}

MpplEntireVarExprFields mppl_entire_var_expr_fields_alloc(const MpplEntireVarExpr *entire_var)
{
  MpplEntireVarExprFields fields;
  fields.name = (void *) syntax_tree_child_tree((const SyntaxTree *) entire_var, 0);
  return fields;
}

MpplIndexedVarExprFields mppl_indexed_var_expr_fields_alloc(const MpplIndexedVarExpr *indexed_var)
{
  MpplIndexedVarExprFields fields;
  fields.name           = (void *) syntax_tree_child_tree((const SyntaxTree *) indexed_var, 0);
  fields.lbracket_token = syntax_tree_child_token((const SyntaxTree *) indexed_var, 1);
  fields.index          = (void *) syntax_tree_child_tree((const SyntaxTree *) indexed_var, 2);
  fields.rbracket_token = syntax_tree_child_token((const SyntaxTree *) indexed_var, 3);
  return fields;
}

MpplUnaryExprFields mppl_unary_expr_fields_alloc(const MpplUnaryExpr *unary_expr)
{
  MpplUnaryExprFields fields;
  fields.op_token = syntax_tree_child_token((const SyntaxTree *) unary_expr, 0);
  fields.expr     = (void *) syntax_tree_child_tree((const SyntaxTree *) unary_expr, 1);
  return fields;
}

MpplBinaryExprFields mppl_binary_expr_fields_alloc(const MpplBinaryExpr *binary_expr)
{
  MpplBinaryExprFields fields;
  fields.lhs      = (void *) syntax_tree_child_tree((const SyntaxTree *) binary_expr, 0);
  fields.op_token = syntax_tree_child_token((const SyntaxTree *) binary_expr, 1);
  fields.rhs      = (void *) syntax_tree_child_tree((const SyntaxTree *) binary_expr, 2);
  return fields;
}

MpplParenExprFields mppl_paren_expr_fields_alloc(const MpplParenExpr *paren_expr)
{
  MpplParenExprFields fields;
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) paren_expr, 0);
  fields.expr         = (void *) syntax_tree_child_tree((const SyntaxTree *) paren_expr, 1);
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) paren_expr, 2);
  return fields;
}

MpplCastExprFields mppl_cast_expr_fields_alloc(const MpplCastExpr *cast_expr)
{
  MpplCastExprFields fields;
  fields.type         = (void *) syntax_tree_child_tree((const SyntaxTree *) cast_expr, 0);
  fields.lparen_token = syntax_tree_child_token((const SyntaxTree *) cast_expr, 1);
  fields.expr         = (void *) syntax_tree_child_tree((const SyntaxTree *) cast_expr, 2);
  fields.rparen_token = syntax_tree_child_token((const SyntaxTree *) cast_expr, 3);
  return fields;
}

void mppl_root_fields_free(MpplRootFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->program);
    syntax_tree_free((void *) fields->eof);
  }
}

void mppl_eof_fields_free(MpplEofFields *fields)
{
  if (fields) {
    syntax_token_free(fields->eof_token);
  }
}

void mppl_program_fields_free(MpplProgramFields *fields)
{
  if (fields) {
    syntax_token_free(fields->program_kw);
    syntax_tree_free((void *) fields->name);
    syntax_token_free(fields->semi_token);
    syntax_tree_free((void *) fields->decl_part_list);
    syntax_tree_free((void *) fields->comp_stmt);
    syntax_token_free(fields->dot_token);
  }
}

void mppl_decl_part_list_fields_free(MpplDeclPartListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_bind_ident_list_elem_fields_free(MpplBindIdentListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->bind_ident);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_bind_ident_list_fields_free(MpplBindIdentListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_bind_ident_fields_free(MpplBindIdentFields *fields)
{
  if (fields) {
    syntax_token_free(fields->ident);
  }
}

void mppl_var_decl_part_fields_free(MpplVarDeclPartFields *fields)
{
  if (fields) {
    syntax_token_free(fields->var_kw);
    syntax_tree_free((void *) fields->var_decl_list);
  }
}

void mppl_var_decl_list_elem_fields_free(MpplVarDeclListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->var_decl);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_var_decl_list_fields_free(MpplVarDeclListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_var_decl_fields_free(MpplVarDeclFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->ident_list);
    syntax_token_free(fields->colon_token);
    syntax_tree_free((void *) fields->type);
  }
}

void mppl_integer_type_fields_free(MpplIntegerTypeFields *fields)
{
  if (fields) {
    syntax_token_free(fields->integer_kw);
  }
}

void mppl_char_type_fields_free(MpplCharTypeFields *fields)
{
  if (fields) {
    syntax_token_free(fields->char_kw);
  }
}

void mppl_boolean_type_fields_free(MpplBooleanTypeFields *fields)
{
  if (fields) {
    syntax_token_free(fields->boolean_kw);
  }
}

void mppl_array_type_fields_free(MpplArrayTypeFields *fields)
{
  if (fields) {
    syntax_token_free(fields->array_kw);
    syntax_token_free(fields->lbracket_token);
    syntax_token_free(fields->number_lit);
    syntax_token_free(fields->rbracket_token);
    syntax_token_free(fields->of_kw);
    syntax_tree_free((void *) fields->type);
  }
}

void mppl_proc_decl_part_fields_free(MpplProcDeclPartFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->proc_decl);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_proc_heading_fields_free(MpplProcHeadingFields *fields)
{
  if (fields) {
    syntax_token_free(fields->procedure_kw);
    syntax_tree_free((void *) fields->name);
    syntax_tree_free((void *) fields->fml_params);
  }
}

void mppl_proc_body_fields_free(MpplProcBodyFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->var_decl_part);
    syntax_tree_free((void *) fields->comp_stmt);
  }
}

void mppl_proc_decl_fields_free(MpplProcDeclFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->proc_heading);
    syntax_token_free(fields->semi_token);
    syntax_tree_free((void *) fields->proc_body);
  }
}

void mppl_fml_param_list_elem_fields_free(MpplFmlParamListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->fml_param_sec);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_fml_param_list_fields_free(MpplFmlParamListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_fml_params_fields_free(MpplFmlParamsFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->fml_param_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_fml_param_sec_fields_free(MpplFmlParamSecFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->ident_list);
    syntax_token_free(fields->colon_token);
    syntax_tree_free((void *) fields->type);
  }
}

void mppl_stmt_list_elem_fields_free(MpplStmtListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->stmt);
    syntax_token_free(fields->semi_token);
  }
}

void mppl_stmt_list_fields_free(MpplStmtListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_assign_stmt_fields_free(MpplAssignStmtFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->lhs);
    syntax_token_free(fields->assign_token);
    syntax_tree_free((void *) fields->rhs);
  }
}

void mppl_if_stmt_fields_free(MpplIfStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->if_kw);
    syntax_tree_free((void *) fields->cond);
    syntax_token_free(fields->then_kw);
    syntax_tree_free((void *) fields->then_stmt);
    syntax_tree_free((void *) fields->else_clause);
  }
}

void mppl_else_clause_fields_free(MpplElseClauseFields *fields)
{
  if (fields) {
    syntax_token_free(fields->else_kw);
    syntax_tree_free((void *) fields->else_stmt);
  }
}

void mppl_while_stmt_fields_free(MpplWhileStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->while_kw);
    syntax_tree_free((void *) fields->cond);
    syntax_token_free(fields->do_kw);
    syntax_tree_free((void *) fields->stmt);
  }
}

void mppl_break_stmt_fields_free(MpplBreakStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->break_kw);
  }
}

void mppl_call_stmt_fields_free(MpplCallStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->call_kw);
    syntax_tree_free((void *) fields->name);
    syntax_tree_free((void *) fields->act_params);
  }
}

void mppl_act_params_fields_free(MpplActParamsFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->expr_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_return_stmt_fields_free(MpplReturnStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->return_kw);
  }
}

void mppl_input_stmt_fields_free(MpplInputStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->read_op_token);
    syntax_tree_free((void *) fields->inputs);
  }
}

void mppl_inputs_fields_free(MpplInputsFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->expr_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_output_stmt_fields_free(MpplOutputStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->write_op_token);
    syntax_tree_free((void *) fields->outputs);
  }
}

void mppl_output_list_elem_fields_free(MpplOutputListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->output_value);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_output_list_fields_free(MpplOutputListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_outputs_fields_free(MpplOutputsFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->output_list);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_output_value_field_width_fields_free(MpplOutputValueFieldWidthFields *fields)
{
  if (fields) {
    syntax_token_free(fields->colon_token);
    syntax_token_free(fields->field_width);
  }
}

void mppl_output_value_fields_free(MpplOutputValueFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->expr);
    syntax_tree_free((void *) fields->field_width);
  }
}

void mppl_comp_stmt_fields_free(MpplCompStmtFields *fields)
{
  if (fields) {
    syntax_token_free(fields->begin_kw);
    syntax_tree_free((void *) fields->stmt_list);
    syntax_token_free(fields->end_kw);
  }
}

void mppl_expr_list_elem_fields_free(MpplExprListElemFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->expr);
    syntax_token_free(fields->comma_token);
  }
}

void mppl_expr_list_fields_free(MpplExprListFields *fields)
{
  if (fields) {
    unsigned long i;
    for (i = 0; i < fields->count; ++i) {
      syntax_tree_free((void *) fields->ptr[i]);
    }
    slice_free(fields);
  }
}

void mppl_ref_ident_fields_free(MpplRefIdentFields *fields)
{
  if (fields) {
    syntax_token_free(fields->ident);
  }
}

void mppl_integer_lit_expr_fields_free(MpplIntegerLitExprFields *fields)
{
  if (fields) {
    syntax_token_free(fields->integer_lit);
  }
}

void mppl_boolean_lit_expr_fields_free(MpplBooleanLitExprFields *fields)
{
  if (fields) {
    syntax_token_free(fields->boolean_lit);
  }
}

void mppl_string_lit_expr_fields_free(MpplStringLitExprFields *fields)
{
  if (fields) {
    syntax_token_free(fields->string_lit);
  }
}

void mppl_entire_var_expr_fields_free(MpplEntireVarExprFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->name);
  }
}

void mppl_indexed_var_expr_fields_free(MpplIndexedVarExprFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->name);
    syntax_token_free(fields->lbracket_token);
    syntax_tree_free((void *) fields->index);
    syntax_token_free(fields->rbracket_token);
  }
}

void mppl_unary_expr_fields_free(MpplUnaryExprFields *fields)
{
  if (fields) {
    syntax_token_free(fields->op_token);
    syntax_tree_free((void *) fields->expr);
  }
}

void mppl_binary_expr_fields_free(MpplBinaryExprFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->lhs);
    syntax_token_free(fields->op_token);
    syntax_tree_free((void *) fields->rhs);
  }
}

void mppl_paren_expr_fields_free(MpplParenExprFields *fields)
{
  if (fields) {
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->expr);
    syntax_token_free(fields->rparen_token);
  }
}

void mppl_cast_expr_fields_free(MpplCastExprFields *fields)
{
  if (fields) {
    syntax_tree_free((void *) fields->type);
    syntax_token_free(fields->lparen_token);
    syntax_tree_free((void *) fields->expr);
    syntax_token_free(fields->rparen_token);
  }
}
