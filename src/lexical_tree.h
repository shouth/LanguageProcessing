#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

typedef enum {
  SYNTAX_KIND_IDENTIFIER,
  SYNTAX_KIND_INTEGER,
  SYNTAX_KIND_STRING,

  SYNTAX_KIND_PLUS,
  SYNTAX_KIND_MINUS,
  SYNTAX_KIND_STAR,
  SYNTAX_KIND_EQUAL,
  SYNTAX_KIND_NOT_EQUAL,
  SYNTAX_KIND_LESS_THAN,
  SYNTAX_KIND_LESS_THAN_EQUAL,
  SYNTAX_KIND_GREATER_THAN,
  SYNTAX_KIND_GREATER_THAN_EQUAL,
  SYNTAX_KIND_LEFT_PARENTHESIS,
  SYNTAX_KIND_RIGHT_PARENTHESIS,
  SYNTAX_KIND_LEFT_BRACKET,
  SYNTAX_KIND_RIGHT_BRACKET,
  SYNTAX_KIND_ASSIGN,
  SYNTAX_KIND_DOT,
  SYNTAX_KIND_COMMA,
  SYNTAX_KIND_COLON,
  SYNTAX_KIND_SEMICOLON,

  SYNTAX_KIND_KEYWORD_PROGRAM,
  SYNTAX_KIND_KEYWORD_VAR,
  SYNTAX_KIND_KEYWORD_ARRAY,
  SYNTAX_KIND_KEYWORD_OF,
  SYNTAX_KIND_KEYWORD_BEGIN,
  SYNTAX_KIND_KEYWORD_END,
  SYNTAX_KIND_KEYWORD_IF,
  SYNTAX_KIND_KEYWORD_THEN,
  SYNTAX_KIND_KEYWORD_ELSE,
  SYNTAX_KIND_KEYWORD_PROCEDURE,
  SYNTAX_KIND_KEYWORD_RETURN,
  SYNTAX_KIND_KEYWORD_CALL,
  SYNTAX_KIND_KEYWORD_WHILE,
  SYNTAX_KIND_KEYWORD_DO,
  SYNTAX_KIND_KEYWORD_NOT,
  SYNTAX_KIND_KEYWORD_OR,
  SYNTAX_KIND_KEYWORD_DIV,
  SYNTAX_KIND_KEYWORD_AND,
  SYNTAX_KIND_KEYWORD_CHAR,
  SYNTAX_KIND_KEYWORD_INTEGER,
  SYNTAX_KIND_KEYWORD_BOOLEAN,
  SYNTAX_KIND_KEYWORD_READ,
  SYNTAX_KIND_KEYWORD_WRITE,
  SYNTAX_KIND_KEYWORD_READLN,
  SYNTAX_KIND_KEYWORD_WRITELN,
  SYNTAX_KIND_KEYWORD_TRUE,
  SYNTAX_KIND_KEYWORD_FALSE,
  SYNTAX_KIND_KEYWORD_BREAK,

  SYNTAX_KIND_PROGRAM,
  SYNTAX_KIND_DECLARATION_PART_VARIABLE,
  SYNTAX_KIND_DECLARATION_PART_PROCEDURE,
  SYNTAX_KIND_DECLARATION_VARIABLE,
  SYNTAX_KIND_DECLARATION_PARAMETER,
  SYNTAX_KIND_STATEMENT_ASSIGN,
  SYNTAX_KIND_STATEMENT_IF,
  SYNTAX_KIND_STATEMENT_WHILE,
  SYNTAX_KIND_STATEMENT_BREAK,
  SYNTAX_KIND_STATEMENT_CALL,
  SYNTAX_KIND_STATEMENT_RETURN,
  SYNTAX_KIND_STATEMENT_READ,
  SYNTAX_KIND_STATEMENT_WRITE,
  SYNTAX_KIND_STATEMENT_COMPOUND,
  SYNTAX_KIND_STATEMENT_EMPTY,
  SYNTAX_KIND_EXPRESSION_REFERENCE,
  SYNTAX_KIND_EXPRESSION_LITERAL,
  SYNTAX_KIND_EXPRESSION_INDEX,
  SYNTAX_KIND_EXPRESSION_BINARY,
  SYNTAX_KIND_EXPRESSION_NOT,
  SYNTAX_KIND_EXPRESSION_PARENTHESIS,
  SYNTAX_KIND_EXPRESSION_CAST,
  SYNTAX_KIND_EXPRESSION_EMPTY,
  SYNTAX_KIND_OUTPUT_FORMAT,
  SYNTAX_KIND_TYPE_STANDARD,
  SYNTAX_KIND_TYPE_ARRAY,

  SYNTAX_KIND_SPACE,
  SYNTAX_KIND_NEWLINE,
  SYNTAX_KIND_BRACES_COMMENT,
  SYNTAX_KIND_C_COMMENT,

  SYNTAX_KIND_ERROR
} SyntaxKind;

typedef struct LexicalTreeNode LexicalTreeNode;
typedef struct LexicalTree     LexicalTree;

struct LexicalTreeNode {
  SyntaxKind       kind;
  long             size;
  LexicalTreeNode *children;
  const char      *string;
};

struct LexicalTree {
  LexicalTreeNode *root;
};

void lexical_tree_init(LexicalTree *tree);
void lexical_tree_deinit(LexicalTree *tree);

#endif
