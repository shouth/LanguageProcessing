/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef MPPL_SYNTAX_KIND_H
#define MPPL_SYNTAX_KIND_H

#include "utility.h"

/* mppl syntax kind */

#define MPPL_SYNTAX_FOR_EACH(F)               \
  F(ERROR, TOKEN, NONE)                       \
  F(IDENT_TOKEN, TOKEN, NONE)                 \
  F(NUMBER_LIT, TOKEN, NONE)                  \
  F(STRING_LIT, TOKEN, NONE)                  \
  F(PLUS_TOKEN, PUNCT, SOME("+"))             \
  F(MINUS_TOKEN, PUNCT, SOME("-"))            \
  F(STAR_TOKEN, PUNCT, SOME("*"))             \
  F(EQUAL_TOKEN, PUNCT, SOME("="))            \
  F(NOTEQ_TOKEN, PUNCT, SOME("<>"))           \
  F(LESS_TOKEN, PUNCT, SOME("<"))             \
  F(LESSEQ_TOKEN, PUNCT, SOME("<="))          \
  F(GREATER_TOKEN, PUNCT, SOME(">"))          \
  F(GREATEREQ_TOKEN, PUNCT, SOME(">="))       \
  F(LPAREN_TOKEN, PUNCT, SOME("("))           \
  F(RPAREN_TOKEN, PUNCT, SOME(")"))           \
  F(LBRACKET_TOKEN, PUNCT, SOME("["))         \
  F(RBRACKET_TOKEN, PUNCT, SOME("]"))         \
  F(ASSIGN_TOKEN, PUNCT, SOME("="))           \
  F(DOT_TOKEN, PUNCT, SOME("."))              \
  F(COMMA_TOKEN, PUNCT, SOME(","))            \
  F(COLON_TOKEN, PUNCT, SOME(":"))            \
  F(SEMI_TOKEN, PUNCT, SOME(";"))             \
  F(PROGRAM_KW, KEYWORD, SOME("program"))     \
  F(VAR_KW, KEYWORD, SOME("var"))             \
  F(ARRAY_KW, KEYWORD, SOME("array"))         \
  F(OF_KW, KEYWORD, SOME("of"))               \
  F(BEGIN_KW, KEYWORD, SOME("begin"))         \
  F(END_KW, KEYWORD, SOME("end"))             \
  F(IF_KW, KEYWORD, SOME("if"))               \
  F(THEN_KW, KEYWORD, SOME("then"))           \
  F(ELSE_KW, KEYWORD, SOME("else"))           \
  F(PROCEDURE_KW, KEYWORD, SOME("procedure")) \
  F(RETURN_KW, KEYWORD, SOME("return"))       \
  F(CALL_KW, KEYWORD, SOME("call"))           \
  F(WHILE_KW, KEYWORD, SOME("while"))         \
  F(DO_KW, KEYWORD, SOME("do"))               \
  F(NOT_KW, KEYWORD, SOME("not"))             \
  F(OR_KW, KEYWORD, SOME("or"))               \
  F(DIV_KW, KEYWORD, SOME("div"))             \
  F(AND_KW, KEYWORD, SOME("and"))             \
  F(CHAR_KW, KEYWORD, SOME("char"))           \
  F(INTEGER_KW, KEYWORD, SOME("integer"))     \
  F(BOOLEAN_KW, KEYWORD, SOME("boolean"))     \
  F(READ_KW, KEYWORD, SOME("read"))           \
  F(WRITE_KW, KEYWORD, SOME("write"))         \
  F(READLN_KW, KEYWORD, SOME("readln"))       \
  F(WRITELN_KW, KEYWORD, SOME("writeln"))     \
  F(TRUE_KW, KEYWORD, SOME("true"))           \
  F(FALSE_KW, KEYWORD, SOME("false"))         \
  F(BREAK_KW, KEYWORD, SOME("break"))         \
  F(EOF_TOKEN, TOKEN, NONE)                   \
  F(SPACE_TRIVIA, TRIVIA, NONE)               \
  F(BRACES_COMMENT_TRIVIA, TRIVIA, NONE)      \
  F(C_COMMENT_TRIVIA, TRIVIA, NONE)           \
  F(PROGRAM, SYNTAX, NONE)                    \
  F(VAR_DECL_PART, SYNTAX, NONE)              \
  F(VAR_DECL, SYNTAX, NONE)                   \
  F(ARRAY_TYPE, SYNTAX, NONE)                 \
  F(PROC_DECL, SYNTAX, NONE)                  \
  F(FML_PARAM_LIST, SYNTAX, NONE)             \
  F(FML_PARAM_SEC, SYNTAX, NONE)              \
  F(ASSIGN_STMT, SYNTAX, NONE)                \
  F(IF_STMT, SYNTAX, NONE)                    \
  F(WHILE_STMT, SYNTAX, NONE)                 \
  F(BREAK_STMT, SYNTAX, NONE)                 \
  F(CALL_STMT, SYNTAX, NONE)                  \
  F(ACT_PARAM_LIST, SYNTAX, NONE)             \
  F(RETURN_STMT, SYNTAX, NONE)                \
  F(INPUT_STMT, SYNTAX, NONE)                 \
  F(INPUT_LIST, SYNTAX, NONE)                 \
  F(OUTPUT_STMT, SYNTAX, NONE)                \
  F(OUTPUT_LIST, SYNTAX, NONE)                \
  F(OUTPUT_VALUE, SYNTAX, NONE)               \
  F(COMP_STMT, SYNTAX, NONE)                  \
  F(ENTIRE_VAR, SYNTAX, NONE)                 \
  F(INDEXED_VAR, SYNTAX, NONE)                \
  F(BINARY_EXPR, SYNTAX, NONE)                \
  F(PAREN_EXPR, SYNTAX, NONE)                 \
  F(NOT_EXPR, SYNTAX, NONE)                   \
  F(CAST_EXPR, SYNTAX, NONE)

#define F(name, kind, string) MPPL_SYNTAX_##name,

typedef enum {
  MPPL_SYNTAX_FOR_EACH(F) SENTINEL_MPPL_SYNTAX
} MpplSyntaxKind;

#undef F

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size);
int            mppl_syntax_kind_is_token(MpplSyntaxKind kind);
int            mppl_syntax_kind_is_trivia(MpplSyntaxKind kind);
const char    *mppl_syntax_kind_to_string(MpplSyntaxKind kind);

/* mppl syntax kind set */

typedef BITSET(MpplSyntaxKindSet, SENTINEL_MPPL_SYNTAX);

#endif
