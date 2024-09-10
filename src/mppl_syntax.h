/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_SYNTAX_KIND_H
#define MPPL_SYNTAX_KIND_H

#include "utility.h"

/* mppl syntax kind */

#define MPPL_SYNTAX_FOR_EACH(F)         \
  F(ERROR, TOKEN, "")                   \
  F(END_OF_FILE, TOKEN, "")             \
  F(IDENT_TOKEN, TOKEN, "")             \
  F(NUMBER_LIT, TOKEN, "")              \
  F(STRING_LIT, TOKEN, "")              \
  F(PLUS_TOKEN, PUNCT, "+")             \
  F(MINUS_TOKEN, PUNCT, "-")            \
  F(STAR_TOKEN, PUNCT, "*")             \
  F(EQUAL_TOKEN, PUNCT, "=")            \
  F(NOTEQ_TOKEN, PUNCT, "<>")           \
  F(LESS_TOKEN, PUNCT, "<")             \
  F(LESSEQ_TOKEN, PUNCT, "<=")          \
  F(GREATER_TOKEN, PUNCT, ">")          \
  F(GREATEREQ_TOKEN, PUNCT, ">=")       \
  F(LPAREN_TOKEN, PUNCT, "(")           \
  F(RPAREN_TOKEN, PUNCT, ")")           \
  F(LBRACKET_TOKEN, PUNCT, "[")         \
  F(RBRACKET_TOKEN, PUNCT, "]")         \
  F(ASSIGN_TOKEN, PUNCT, "=")           \
  F(DOT_TOKEN, PUNCT, ".")              \
  F(COMMA_TOKEN, PUNCT, ",")            \
  F(COLON_TOKEN, PUNCT, ":")            \
  F(SEMI_TOKEN, PUNCT, ";")             \
  F(PROGRAM_KW, KEYWORD, "program")     \
  F(VAR_KW, KEYWORD, "var")             \
  F(ARRAY_KW, KEYWORD, "array")         \
  F(OF_KW, KEYWORD, "of")               \
  F(BEGIN_KW, KEYWORD, "begin")         \
  F(END_KW, KEYWORD, "end")             \
  F(IF_KW, KEYWORD, "if")               \
  F(THEN_KW, KEYWORD, "then")           \
  F(ELSE_KW, KEYWORD, "else")           \
  F(PROCEDURE_KW, KEYWORD, "procedure") \
  F(RETURN_KW, KEYWORD, "return")       \
  F(CALL_KW, KEYWORD, "call")           \
  F(WHILE_KW, KEYWORD, "while")         \
  F(DO_KW, KEYWORD, "do")               \
  F(NOT_KW, KEYWORD, "not")             \
  F(OR_KW, KEYWORD, "or")               \
  F(DIV_KW, KEYWORD, "div")             \
  F(AND_KW, KEYWORD, "and")             \
  F(CHAR_KW, KEYWORD, "char")           \
  F(INTEGER_KW, KEYWORD, "integer")     \
  F(BOOLEAN_KW, KEYWORD, "boolean")     \
  F(READ_KW, KEYWORD, "read")           \
  F(WRITE_KW, KEYWORD, "write")         \
  F(READLN_KW, KEYWORD, "readln")       \
  F(WRITELN_KW, KEYWORD, "writeln")     \
  F(TRUE_KW, KEYWORD, "true")           \
  F(FALSE_KW, KEYWORD, "false")         \
  F(BREAK_KW, KEYWORD, "break")         \
  F(SPACE_TRIVIA, TRIVIA, "")           \
  F(BRACES_COMMENT_TRIVIA, TRIVIA, "")  \
  F(C_COMMENT_TRIVIA, TRIVIA, "")       \
  F(PROGRAM, SYNTAX, "")                \
  F(DECL_LIST, SYNTAX, "")              \
  F(VAR_DECL_PART, SYNTAX, "")          \
  F(VAR_DECL_LIST_ELEM, SYNTAX, "")     \
  F(VAR_DECL_LIST, SYNTAX, "")          \
  F(VAR_DECL, SYNTAX, "")               \
  F(ARRAY_TYPE, SYNTAX, "")             \
  F(PROC_DECL, SYNTAX, "")              \
  F(FML_PARAM_LIST_ELEM, SYNTAX, "")    \
  F(FML_PARAM_LIST, SYNTAX, "")         \
  F(FML_PARAMS, SYNTAX, "")             \
  F(FML_PARAM_SEC, SYNTAX, "")          \
  F(STMT_LIST_ELEM, SYNTAX, "")         \
  F(STMT_LIST, SYNTAX, "")              \
  F(ASSIGN_STMT, SYNTAX, "")            \
  F(IF_STMT, SYNTAX, "")                \
  F(ELSE_CLAUSE, SYNTAX, "")            \
  F(WHILE_STMT, SYNTAX, "")             \
  F(BREAK_STMT, SYNTAX, "")             \
  F(CALL_STMT, SYNTAX, "")              \
  F(ACT_PARAMS, SYNTAX, "")             \
  F(RETURN_STMT, SYNTAX, "")            \
  F(INPUT_STMT, SYNTAX, "")             \
  F(INPUTS, SYNTAX, "")                 \
  F(OUTPUT_STMT, SYNTAX, "")            \
  F(OUTPUT_LIST_ELEM, SYNTAX, "")       \
  F(OUTPUT_LIST, SYNTAX, "")            \
  F(OUTPUTS, SYNTAX, "")                \
  F(OUTPUT_VALUE, SYNTAX, "")           \
  F(COMP_STMT, SYNTAX, "")              \
  F(EXPR_LIST_ELEM, SYNTAX, "")         \
  F(EXPR_LIST, SYNTAX, "")              \
  F(ENTIRE_VAR, SYNTAX, "")             \
  F(INDEXED_VAR, SYNTAX, "")            \
  F(UNARY_EXPR, SYNTAX, "")             \
  F(BINARY_EXPR, SYNTAX, "")            \
  F(PAREN_EXPR, SYNTAX, "")             \
  F(CAST_EXPR, SYNTAX, "")              \
  F(IDENT_LIST_ELEM, SYNTAX, "")        \
  F(IDENT_LIST, SYNTAX, "")             \
  F(BOGUS, BOGUS, "")

#define F(name, kind, string) MPPL_SYNTAX_##name,

typedef enum {
  MPPL_SYNTAX_FOR_EACH(F) SENTINEL_MPPL_SYNTAX
} MpplSyntaxKind;

#undef F

typedef BITSET(SENTINEL_MPPL_SYNTAX) MpplSyntaxKindSet;

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size);
int            mppl_syntax_kind_is_token(MpplSyntaxKind kind);
int            mppl_syntax_kind_is_trivia(MpplSyntaxKind kind);
const char    *mppl_syntax_kind_to_string(MpplSyntaxKind kind);

/* mppl syntax */

#endif
