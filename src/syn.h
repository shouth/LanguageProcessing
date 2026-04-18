/*
 * syn.h -- syntax
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYN_H
#define SYN_H

#include <stddef.h>

#define DEF_SYN(TOK) \
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
  TOK(SYN_SLASH_STAR_COMMENT, NULL)

#define KW_BEGIN SYN_PROGRAM_KW
#define KW_END SYN_BREAK_KW

#define TOK(KIND, LEXEME) KIND,

enum syn_kind {
  DEF_SYN(TOK)
  SIZE_SYN
};

#undef TOK

char const *syn_kind_to_lexeme(enum syn_kind kind);

char const *syn_kind_to_string(enum syn_kind kind);

#endif /* SYN_H */
