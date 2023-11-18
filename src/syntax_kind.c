#include "syntax_kind.h"
#include <string.h>

typedef struct Keyword Keyword;

struct Keyword {
  const char *keyword;
  SyntaxKind  kind;
};

static Keyword KEYWORDS[] = {
  { "and", SYNTAX_KIND_AND_KEYWORD },
  { "array", SYNTAX_KIND_ARRAY_KEYWORD },
  { "begin", SYNTAX_KIND_BEGIN_KEYWORD },
  { "boolean", SYNTAX_KIND_BOOLEAN_KEYWORD },
  { "break", SYNTAX_KIND_BREAK_KEYWORD },
  { "call", SYNTAX_KIND_CALL_KEYWORD },
  { "char", SYNTAX_KIND_CHAR_KEYWORD },
  { "div", SYNTAX_KIND_DIV_KEYWORD },
  { "do", SYNTAX_KIND_DO_KEYWORD },
  { "else", SYNTAX_KIND_ELSE_KEYWORD },
  { "end", SYNTAX_KIND_END_KEYWORD },
  { "false", SYNTAX_KIND_FALSE_KEYWORD },
  { "if", SYNTAX_KIND_IF_KEYWORD },
  { "integer", SYNTAX_KIND_INTEGER_KEYWORD },
  { "not", SYNTAX_KIND_NOT_KEYWORD },
  { "of", SYNTAX_KIND_OF_KEYWORD },
  { "or", SYNTAX_KIND_OR_KEYWORD },
  { "procedure", SYNTAX_KIND_PROCEDURE_KEYWORD },
  { "program", SYNTAX_KIND_PROGRAM_KEYWORD },
  { "read", SYNTAX_KIND_READ_KEYWORD },
  { "readln", SYNTAX_KIND_READLN_KEYWORD },
  { "return", SYNTAX_KIND_RETURN_KEYWORD },
  { "then", SYNTAX_KIND_THEN_KEYWORD },
  { "true", SYNTAX_KIND_TRUE_KEYWORD },
  { "var", SYNTAX_KIND_VAR_KEYWORD },
  { "while", SYNTAX_KIND_WHILE_KEYWORD },
  { "write", SYNTAX_KIND_WRITE_KEYWORD },
  { "writeln", SYNTAX_KIND_WRITELN_KEYWORD },
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
  return kind <= SYNTAX_KIND_EOF_TOKEN;
}

int syntax_kind_is_trivia(SyntaxKind kind)
{
  return kind >= SYNTAX_KIND_SPACE_TRIVIA && kind <= SYNTAX_KIND_C_COMMENT_TRIVIA;
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
