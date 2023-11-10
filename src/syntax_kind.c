#include "syntax_kind.h"
#include <string.h>

typedef struct Keyword Keyword;

struct Keyword {
  const char *keyword;
  SyntaxKind  kind;
};

static Keyword KEYWORDS[] = {
  { "and", SYNTAX_KIND_KEYWORD_AND },
  { "array", SYNTAX_KIND_KEYWORD_ARRAY },
  { "begin", SYNTAX_KIND_KEYWORD_BEGIN },
  { "boolean", SYNTAX_KIND_KEYWORD_BOOLEAN },
  { "break", SYNTAX_KIND_KEYWORD_BREAK },
  { "call", SYNTAX_KIND_KEYWORD_CALL },
  { "char", SYNTAX_KIND_KEYWORD_CHAR },
  { "div", SYNTAX_KIND_KEYWORD_DIV },
  { "do", SYNTAX_KIND_KEYWORD_DO },
  { "else", SYNTAX_KIND_KEYWORD_ELSE },
  { "end", SYNTAX_KIND_KEYWORD_END },
  { "false", SYNTAX_KIND_KEYWORD_FALSE },
  { "if", SYNTAX_KIND_KEYWORD_IF },
  { "integer", SYNTAX_KIND_KEYWORD_INTEGER },
  { "not", SYNTAX_KIND_KEYWORD_NOT },
  { "of", SYNTAX_KIND_KEYWORD_OF },
  { "or", SYNTAX_KIND_KEYWORD_OR },
  { "procedure", SYNTAX_KIND_KEYWORD_PROCEDURE },
  { "program", SYNTAX_KIND_KEYWORD_PROGRAM },
  { "read", SYNTAX_KIND_KEYWORD_READ },
  { "readln", SYNTAX_KIND_KEYWORD_READLN },
  { "return", SYNTAX_KIND_KEYWORD_RETURN },
  { "then", SYNTAX_KIND_KEYWORD_THEN },
  { "true", SYNTAX_KIND_KEYWORD_TRUE },
  { "var", SYNTAX_KIND_KEYWORD_VAR },
  { "while", SYNTAX_KIND_KEYWORD_WHILE },
  { "write", SYNTAX_KIND_KEYWORD_WRITE },
  { "writeln", SYNTAX_KIND_KEYWORD_WRITELN },
};

SyntaxKind syntax_kind_from_keyword(const char *string, unsigned long size)
{
  unsigned long i;
  for (i = 0; i < sizeof(KEYWORDS) / sizeof(Keyword); ++i) {
    if (!strncmp(KEYWORDS[i].keyword, string, size) && !KEYWORDS[i].keyword[size]) {
      return KEYWORDS[i].kind;
    }
  }
  return SYNTAX_KIND_ERROR;
}

int syntax_kind_is_token(SyntaxKind kind)
{
  return kind <= SYNTAX_KIND_EOF;
}

int syntax_kind_is_trivia(SyntaxKind kind)
{
  return kind >= SYNTAX_KIND_SPACE && kind <= SYNTAX_KIND_C_COMMENT;
}

static const char *SYNTAX_KIND_TO_STRING[] = {
  "IDENTIFIER",
  "INTEGER",
  "STRING",
  "PLUS",
  "MINUS",
  "STAR",
  "EQUAL",
  "NOT_EQUAL",
  "LESS_THAN",
  "LESS_THAN_EQUAL",
  "GREATER_THAN",
  "GREATER_THAN_EQUAL",
  "LEFT_PARENTHESIS",
  "RIGHT_PARENTHESIS",
  "LEFT_BRACKET",
  "RIGHT_BRACKET",
  "ASSIGN",
  "DOT",
  "COMMA",
  "COLON",
  "SEMICOLON",
  "KEYWORD_PROGRAM",
  "KEYWORD_VAR",
  "KEYWORD_ARRAY",
  "KEYWORD_OF",
  "KEYWORD_BEGIN",
  "KEYWORD_END",
  "KEYWORD_IF",
  "KEYWORD_THEN",
  "KEYWORD_ELSE",
  "KEYWORD_PROCEDURE",
  "KEYWORD_RETURN",
  "KEYWORD_CALL",
  "KEYWORD_WHILE",
  "KEYWORD_DO",
  "KEYWORD_NOT",
  "KEYWORD_OR",
  "KEYWORD_DIV",
  "KEYWORD_AND",
  "KEYWORD_CHAR",
  "KEYWORD_INTEGER",
  "KEYWORD_BOOLEAN",
  "KEYWORD_READ",
  "KEYWORD_WRITE",
  "KEYWORD_READLN",
  "KEYWORD_WRITELN",
  "KEYWORD_TRUE",
  "KEYWORD_FALSE",
  "KEYWORD_BREAK",
  "SPACE",
  "NEWLINE",
  "BRACES_COMMENT",
  "C_COMMENT",
  "EOF",
  "PROGRAM",
  "VARIABLE_DECLARATION_PART",
  "VARIABLE_DECLARATION",
  "ARRAY_TYPE",
  "PROCEDURE_DECLARATION",
  "FORMAL_PARAMETER_LIST",
  "FORMAL_PARAMETER_SECTION",
  "ASSIGNMENT_STATEMENT",
  "IF_STATEMENT",
  "WHILE_STATEMENT",
  "BREAK_STATEMENT",
  "CALL_STATEMENT",
  "ACTUAL_PARAMETER_LIST",
  "RETURN_STATEMENT",
  "INPUT_STATEMENT",
  "INPUT_LIST",
  "OUTPUT_STATEMENT",
  "OUTPUT_LIST",
  "OUTPUT_VALUE",
  "COMPOUND_STATEMENT",
  "ENTIRE_VARIABLE",
  "INDEXED_VARIABLE",
  "BINARY_EXPRESSION",
  "PARENTHESIZED_EXPRESSION",
  "NOT_EXPRESSION",
  "CAST_EXPRESSION",
  "ERROR",
};

const char *syntax_kind_to_string(SyntaxKind kind)
{
  return SYNTAX_KIND_TO_STRING[kind];
}
