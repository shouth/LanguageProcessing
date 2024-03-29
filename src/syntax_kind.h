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

#ifndef SYNTAX_KIND_H
#define SYNTAX_KIND_H

typedef enum {
  SYNTAX_BAD_TOKEN,
  SYNTAX_IDENT_TOKEN,
  SYNTAX_NUMBER_LIT,
  SYNTAX_STRING_LIT,
  SYNTAX_PLUS_TOKEN,
  SYNTAX_MINUS_TOKEN,
  SYNTAX_STAR_TOKEN,
  SYNTAX_EQUAL_TOKEN,
  SYNTAX_NOTEQ_TOKEN,
  SYNTAX_LESS_TOKEN,
  SYNTAX_LESSEQ_TOKEN,
  SYNTAX_GREATER_TOKEN,
  SYNTAX_GREATEREQ_TOKEN,
  SYNTAX_LPAREN_TOKEN,
  SYNTAX_RPAREN_TOKEN,
  SYNTAX_LBRACKET_TOKEN,
  SYNTAX_RBRACKET_TOKEN,
  SYNTAX_ASSIGN_TOKEN,
  SYNTAX_DOT_TOKEN,
  SYNTAX_COMMA_TOKEN,
  SYNTAX_COLON_TOKEN,
  SYNTAX_SEMI_TOKEN,
  SYNTAX_PROGRAM_KW,
  SYNTAX_VAR_KW,
  SYNTAX_ARRAY_KW,
  SYNTAX_OF_KW,
  SYNTAX_BEGIN_KW,
  SYNTAX_END_KW,
  SYNTAX_IF_KW,
  SYNTAX_THEN_KW,
  SYNTAX_ELSE_KW,
  SYNTAX_PROCEDURE_KW,
  SYNTAX_RETURN_KW,
  SYNTAX_CALL_KW,
  SYNTAX_WHILE_KW,
  SYNTAX_DO_KW,
  SYNTAX_NOT_KW,
  SYNTAX_OR_KW,
  SYNTAX_DIV_KW,
  SYNTAX_AND_KW,
  SYNTAX_CHAR_KW,
  SYNTAX_INTEGER_KW,
  SYNTAX_BOOLEAN_KW,
  SYNTAX_READ_KW,
  SYNTAX_WRITE_KW,
  SYNTAX_READLN_KW,
  SYNTAX_WRITELN_KW,
  SYNTAX_TRUE_KW,
  SYNTAX_FALSE_KW,
  SYNTAX_BREAK_KW,
  SYNTAX_EOF_TOKEN,
  SYNTAX_SPACE_TRIVIA,
  SYNTAX_BRACES_COMMENT_TRIVIA,
  SYNTAX_C_COMMENT_TRIVIA,
  SYNTAX_PROGRAM,
  SYNTAX_VAR_DECL_PART,
  SYNTAX_VAR_DECL,
  SYNTAX_ARRAY_TYPE,
  SYNTAX_PROC_DECL,
  SYNTAX_FML_PARAM_LIST,
  SYNTAX_FML_PARAM_SEC,
  SYNTAX_ASSIGN_STMT,
  SYNTAX_IF_STMT,
  SYNTAX_WHILE_STMT,
  SYNTAX_BREAK_STMT,
  SYNTAX_CALL_STMT,
  SYNTAX_ACT_PARAM_LIST,
  SYNTAX_RETURN_STMT,
  SYNTAX_INPUT_STMT,
  SYTANX_INPUT_LIST,
  SYNTAX_OUTPUT_STMT,
  SYNTAX_OUTPUT_LIST,
  SYNTAX_OUTPUT_VALUE,
  SYNTAX_COMP_STMT,
  SYNTAX_ENTIRE_VAR,
  SYNTAX_INDEXED_VAR,
  SYNTAX_BINARY_EXPR,
  SYNTAX_PAREN_EXPR,
  SYNTAX_NOT_EXPR,
  SYNTAX_CAST_EXPR
} SyntaxKind;

SyntaxKind  syntax_kind_from_keyword(const char *string, unsigned long size);
int         syntax_kind_is_token(SyntaxKind kind);
int         syntax_kind_is_trivia(SyntaxKind kind);
const char *syntax_kind_to_string(SyntaxKind kind);

#endif
