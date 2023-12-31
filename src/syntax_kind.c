#include <string.h>

#include "syntax_kind.h"

typedef struct Keyword Keyword;

struct Keyword {
  const char *keyword;
  SyntaxKind  kind;
};

static Keyword KEYWORDS[] = {
  { "and", SYNTAX_AND_KW },
  { "array", SYNTAX_ARRAY_KW },
  { "begin", SYNTAX_BEGIN_KW },
  { "boolean", SYNTAX_BOOLEAN_KW },
  { "break", SYNTAX_BREAK_KW },
  { "call", SYNTAX_CALL_KW },
  { "char", SYNTAX_CHAR_KW },
  { "div", SYNTAX_DIV_KW },
  { "do", SYNTAX_DO_KW },
  { "else", SYNTAX_ELSE_KW },
  { "end", SYNTAX_END_KW },
  { "false", SYNTAX_FALSE_KW },
  { "if", SYNTAX_IF_KW },
  { "integer", SYNTAX_INTEGER_KW },
  { "not", SYNTAX_NOT_KW },
  { "of", SYNTAX_OF_KW },
  { "or", SYNTAX_OR_KW },
  { "procedure", SYNTAX_PROCEDURE_KW },
  { "program", SYNTAX_PROGRAM_KW },
  { "read", SYNTAX_READ_KW },
  { "readln", SYNTAX_READLN_KW },
  { "return", SYNTAX_RETURN_KW },
  { "then", SYNTAX_THEN_KW },
  { "true", SYNTAX_TRUE_KW },
  { "var", SYNTAX_VAR_KW },
  { "while", SYNTAX_WHILE_KW },
  { "write", SYNTAX_WRITE_KW },
  { "writeln", SYNTAX_WRITELN_KW },
};

SyntaxKind syntax_kind_from_keyword(const char *string, unsigned long size)
{
  unsigned long i;
  for (i = 0; i < sizeof(KEYWORDS) / sizeof(Keyword); ++i) {
    if (!strncmp(KEYWORDS[i].keyword, string, size) && !KEYWORDS[i].keyword[size]) {
      return KEYWORDS[i].kind;
    }
  }
  return SYNTAX_BAD_TOKEN;
}

int syntax_kind_is_token(SyntaxKind kind)
{
  return kind <= SYNTAX_EOF_TOKEN;
}

int syntax_kind_is_trivia(SyntaxKind kind)
{
  return kind >= SYNTAX_SPACE_TRIVIA && kind <= SYNTAX_C_COMMENT_TRIVIA;
}

static const char *SYNTAX_TO_STRING[] = {
  "ERROR_TOKEN",
  "IDENTIFIER_TOKEN",
  "NUMBER_LITERAL",
  "STRING_LITERAL",
  "PLUS_TOKEN",
  "MINUS_TOKEN",
  "STAR_TOKEN",
  "EQUAL_TOKEN",
  "NOT_EQUAL_TOKEN",
  "LESS_THAN_TOKEN",
  "LESS_THAN_EQUAL_TOKEN",
  "GREATER_THAN_TOKEN",
  "GREATER_THAN_EQUAL_TOKEN",
  "LEFT_PARENTHESIS_TOKEN",
  "RIGHT_PARENTHESIS_TOKEN",
  "LEFT_BRACKET_TOKEN",
  "RIGHT_BRACKET_TOKEN",
  "ASSIGN_TOKEN",
  "DOT_TOKEN",
  "COMMA_TOKEN",
  "COLON_TOKEN",
  "SEMICOLON_TOKEN",
  "PROGRAM_KW",
  "VAR_KW",
  "ARRAY_KW",
  "OF_KW",
  "BEGIN_KW",
  "END_KW",
  "IF_KW",
  "THEN_KW",
  "ELSE_KW",
  "PROCEDURE_KW",
  "RETURN_KW",
  "CALL_KW",
  "WHILE_KW",
  "DO_KW",
  "NOT_KW",
  "OR_KW",
  "DIV_KW",
  "AND_KW",
  "CHAR_KW",
  "INTEGER_KW",
  "BOOLEAN_KW",
  "READ_KW",
  "WRITE_KW",
  "READLN_KW",
  "WRITELN_KW",
  "TRUE_KW",
  "FALSE_KW",
  "BREAK_KW",
  "EOF_TOKEN",
  "SPACE_TRIVIA",
  "NEWLINE_TRIVIA",
  "BRACES_COMMENT_TRIVIA",
  "C_COMMENT_TRIVIA",
  "PROGRAM",
  "VAR_DECL_PART",
  "VAR_DECL",
  "ARRAY_TYPE",
  "PROCEDURE_DECL",
  "FORMAL_PARAM_LIST",
  "FORMAL_PARAM_SECTION",
  "ASSIGNMENT_STMT",
  "IF_STMT",
  "WHILE_STMT",
  "BREAK_STMT",
  "CALL_STMT",
  "ACTUAL_PARAM_LIST",
  "RETURN_STMT",
  "INPUT_STMT",
  "INPUT_LIST",
  "OUTPUT_STMT",
  "OUTPUT_LIST",
  "OUTPUT_VALUE",
  "COMPOUND_STMT",
  "ENTIRE_VAR",
  "INDEXED_VAR",
  "BINARY_EXPR",
  "PARENTHESIZED_EXPR",
  "NOT_EXPR",
  "CAST_EXPR",
};

const char *syntax_kind_to_string(SyntaxKind kind)
{
  return SYNTAX_TO_STRING[kind];
}
