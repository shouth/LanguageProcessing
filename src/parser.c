#include "parser.h"
#include "syntax_kind.h"
#include "token.h"
#include "token_cursor.h"
#include "vector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Bookmark Bookmark;
typedef struct Parser   Parser;

struct Bookmark {
  SyntaxKind    kind;
  unsigned long checkpoint;
};

struct Parser {
  TokenCursor cursor;
  TokenNode   token;
  Vector      parents;
  Vector      children;
};

static void bump(Parser *parser)
{
  vector_push(&parser->children, &parser->token);
  token_cursor_next(&parser->cursor, &parser->token.token);
}

static int check(Parser *parser, SyntaxKind kind)
{
  return parser->token.token.kind == kind;
}

static int eat(Parser *parser, SyntaxKind kind)
{
  int result = check(parser, kind);
  if (result) {
    bump(parser);
  }
  return result;
}

static int expect(Parser *parser, SyntaxKind kind)
{
  int result = eat(parser, kind);
  if (!result) {
    printf("expected: %d, actual: %d (%s)\n", kind, parser->token.token.kind, parser->token.token.text);
    exit(EXIT_FAILURE);
    /* TODO: create an error object and push to parser */
  }
  return result;
}

static unsigned long node_checkpoint(Parser *parser)
{
  return vector_length(&parser->children);
}

static void node_start_at(Parser *parser, SyntaxKind kind, unsigned long checkpoint)
{
  Bookmark bookmark;
  bookmark.kind       = kind;
  bookmark.checkpoint = checkpoint;
  vector_push(&parser->parents, &bookmark);
}

static void node_start(Parser *parser, SyntaxKind kind)
{
  unsigned long checkpoint = node_checkpoint(parser);
  node_start_at(parser, kind, checkpoint);
}

static void node_finish(Parser *parser)
{
  TokenNode tree;
  Bookmark  bookmark = *(Bookmark *) vector_back(&parser->parents);

  assert(vector_length(&parser->children) >= bookmark.checkpoint);
  vector_pop(&parser->parents);

  token_tree_init(&tree.tree, bookmark.kind,
    vector_at(&parser->children, bookmark.checkpoint),
    vector_length(&parser->children) - bookmark.checkpoint);

  while (vector_length(&parser->children) > bookmark.checkpoint) {
    vector_pop(&parser->children);
  }

  vector_push(&parser->children, &tree.tree);
}

static int check_standard_type(Parser *parser)
{
  return check(parser, SYNTAX_KIND_KEYWORD_INTEGER)
    || check(parser, SYNTAX_KIND_KEYWORD_BOOLEAN)
    || check(parser, SYNTAX_KIND_KEYWORD_CHAR);
}

static void parse_standard_type(Parser *parser)
{
  if (!(eat(parser, SYNTAX_KIND_KEYWORD_INTEGER) || eat(parser, SYNTAX_KIND_KEYWORD_BOOLEAN) || eat(parser, SYNTAX_KIND_KEYWORD_CHAR))) {
    /* TODO: make error */
  }
}

static void parse_array_type(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_ARRAY_TYPE);
  expect(parser, SYNTAX_KIND_KEYWORD_ARRAY);
  expect(parser, SYNTAX_KIND_LEFT_BRACKET);
  expect(parser, SYNTAX_KIND_INTEGER);
  expect(parser, SYNTAX_KIND_RIGHT_BRACKET);
  expect(parser, SYNTAX_KIND_KEYWORD_OF);
  parse_standard_type(parser);
  node_finish(parser);
}

static void parse_type(Parser *parser)
{
  if (check_standard_type(parser)) {
    parse_standard_type(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_ARRAY)) {
    parse_array_type(parser);
  } else {
    /* TODO: make error */
  }
}

static void parse_factor(Parser *parser);
static void parse_expression(Parser *parser);

static void parse_variable(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE);
  expect(parser, SYNTAX_KIND_IDENTIFIER);
  if (eat(parser, SYNTAX_KIND_LEFT_BRACKET)) {
    parse_expression(parser);
    expect(parser, SYNTAX_KIND_RIGHT_BRACKET);
  }
  node_finish(parser);
}

static void parse_parenthesized_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PARENTHESIZED_EXPRESSION);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_not_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_NOT_EXPRESSION);
  expect(parser, SYNTAX_KIND_KEYWORD_NOT);
  parse_factor(parser);
  node_finish(parser);
}

static void parse_cast_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_CAST_EXPRESSION);
  parse_standard_type(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_empty_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_EMPTY_EXPRESSION);
  node_finish(parser);
}

static void parse_factor(Parser *parser)
{
  if (check(parser, SYNTAX_KIND_IDENTIFIER)) {
    parse_variable(parser);
  } else if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS)) {
    parse_parenthesized_expression(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_NOT)) {
    parse_not_expression(parser);
  } else if (check_standard_type(parser)) {
    parse_cast_expression(parser);
  } else if (!(eat(parser, SYNTAX_KIND_INTEGER) || eat(parser, SYNTAX_KIND_KEYWORD_TRUE) || eat(parser, SYNTAX_KIND_KEYWORD_FALSE) || eat(parser, SYNTAX_KIND_STRING))) {
    /* make error */
  }
}

static int check_multicative_operator(Parser *parser)
{
  return check(parser, SYNTAX_KIND_STAR)
    || check(parser, SYNTAX_KIND_KEYWORD_DIV)
    || check(parser, SYNTAX_KIND_KEYWORD_AND);
}

static int eat_multicative_operator(Parser *parser)
{
  int result = check_multicative_operator(parser);
  if (result) {
    bump(parser);
  }
  return result;
}

static void parse_term(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_factor(parser);
  while (eat_multicative_operator(parser)) {
    node_start_at(parser, SYNTAX_KIND_BINARY_EXPRESSION, checkpoint);
    parse_factor(parser);
    node_finish(parser);
  }
}

static int check_additive_operator(Parser *parser)
{
  return check(parser, SYNTAX_KIND_PLUS)
    || check(parser, SYNTAX_KIND_MINUS)
    || check(parser, SYNTAX_KIND_KEYWORD_OR);
}

static int eat_additive_operator(Parser *parser)
{
  int result = check_additive_operator(parser);
  if (result) {
    bump(parser);
  }
  return result;
}

static void parse_simple_expression(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  if (check_additive_operator(parser)) {
    parse_empty_expression(parser);
  } else {
    parse_term(parser);
  }
  while (eat_additive_operator(parser)) {
    node_start_at(parser, SYNTAX_KIND_BINARY_EXPRESSION, checkpoint);
    parse_term(parser);
    node_finish(parser);
  }
}

static int check_relational_operator(Parser *parser)
{
  return check(parser, SYNTAX_KIND_EQUAL)
    || check(parser, SYNTAX_KIND_NOT_EQUAL)
    || check(parser, SYNTAX_KIND_LESS_THAN)
    || check(parser, SYNTAX_KIND_LESS_THAN_EQUAL)
    || check(parser, SYNTAX_KIND_GREATER_THAN)
    || check(parser, SYNTAX_KIND_GREATER_THAN_EQUAL);
}

static int eat_relational_operator(Parser *parser)
{
  int result = check_relational_operator(parser);
  if (result) {
    bump(parser);
  }
  return result;
}

static void parse_expression(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_simple_expression(parser);
  while (eat_relational_operator(parser)) {
    node_start_at(parser, SYNTAX_KIND_BINARY_EXPRESSION, checkpoint);
    parse_simple_expression(parser);
    node_finish(parser);
  }
}

static void parse_statement(Parser *parser);

static void parse_assignment_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_ASSIGNMENT_STATEMENT);
  parse_variable(parser);
  expect(parser, SYNTAX_KIND_ASSIGN);
  parse_expression(parser);
  node_finish(parser);
}

static void parse_if_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_IF_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_IF);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_KEYWORD_THEN);
  parse_statement(parser);
  if (eat(parser, SYNTAX_KIND_KEYWORD_ELSE)) {
    parse_statement(parser);
  }
  node_finish(parser);
}

static void parse_while_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_WHILE_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_WHILE);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_KEYWORD_DO);
  parse_statement(parser);
  node_finish(parser);
}

static void parse_break_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_RETURN_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_BREAK);
  node_finish(parser);
}

static void parse_actual_parameter_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_ACTUAL_PARAMETER_LIST);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  do {
    parse_expression(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_call_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_CALL_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_CALL);
  expect(parser, SYNTAX_KIND_IDENTIFIER);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS)) {
    parse_actual_parameter_list(parser);
  }
  node_finish(parser);
}

static void parse_return_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_RETURN_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_RETURN);
  node_finish(parser);
}

static void parse_input_list(Parser *parser)
{
  node_start(parser, SYTANX_KIND_INPUT_LIST);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  do {
    parse_variable(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_input_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_INPUT_STATEMENT);
  if (!(eat(parser, SYNTAX_KIND_KEYWORD_READ) || eat(parser, SYNTAX_KIND_KEYWORD_READLN))) {
    /* TODO: make error */
  }
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS)) {
    parse_input_list(parser);
  }
  node_finish(parser);
}

static void parse_output_value(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_VALUE);
  parse_expression(parser);
  if (eat(parser, SYNTAX_KIND_COLON)) {
    expect(parser, SYNTAX_KIND_INTEGER);
  }
  node_finish(parser);
}

static void parse_output_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_LIST);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  do {
    parse_output_value(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_output_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_STATEMENT);
  if (!(eat(parser, SYNTAX_KIND_KEYWORD_WRITE) || eat(parser, SYNTAX_KIND_KEYWORD_WRITELN))) {
    /* TODO: make error */
  }
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS)) {
    parse_output_list(parser);
  }
  node_finish(parser);
}

static void parse_compound_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_COMPOUND_STATEMENT);
  expect(parser, SYNTAX_KIND_KEYWORD_BEGIN);
  do {
    parse_statement(parser);
  } while (eat(parser, SYNTAX_KIND_SEMICOLON));
  expect(parser, SYNTAX_KIND_KEYWORD_END);
  node_finish(parser);
}

static void parse_empty_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_EMPTY_STATEMENT);
  node_finish(parser);
}

static void parse_statement(Parser *parser)
{
  if (check(parser, SYNTAX_KIND_IDENTIFIER)) {
    parse_assignment_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_IF)) {
    parse_if_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_WHILE)) {
    parse_while_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_BREAK)) {
    parse_break_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_CALL)) {
    parse_call_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_RETURN)) {
    parse_return_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_READ) || check(parser, SYNTAX_KIND_KEYWORD_READLN)) {
    parse_input_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_WRITE) || check(parser, SYNTAX_KIND_KEYWORD_WRITELN)) {
    parse_output_statement(parser);
  } else if (check(parser, SYNTAX_KIND_KEYWORD_BEGIN)) {
    parse_compound_statement(parser);
  } else {
    parse_empty_statement(parser);
  }
}

static void parse_variable_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE_DECLARATION);
  do {
    expect(parser, SYNTAX_KIND_IDENTIFIER);
  } while (eat(parser, SYNTAX_KIND_COMMA));
  expect(parser, SYNTAX_KIND_COLON);
  parse_type(parser);
  expect(parser, SYNTAX_KIND_SEMICOLON);
  node_finish(parser);
}

static void parse_variable_declaration_part(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE_DECLARATION_PART);
  expect(parser, SYNTAX_KIND_KEYWORD_VAR);
  do {
    parse_variable_declaration(parser);
  } while (check(parser, SYNTAX_KIND_IDENTIFIER));
  node_finish(parser);
}

static void parse_formal_parameter_section(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_FORMAL_PARAMETER_SECTION);
  do {
    expect(parser, SYNTAX_KIND_IDENTIFIER);
  } while (eat(parser, SYNTAX_KIND_COMMA));
  expect(parser, SYNTAX_KIND_COLON);
  parse_type(parser);
  node_finish(parser);
}

static void parse_formal_parameter_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_FORMAL_PARAMETER_LIST);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS);
  do {
    parse_formal_parameter_section(parser);
  } while (eat(parser, SYNTAX_KIND_SEMICOLON));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_procedure_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PROCEDURE_DECLARATION);
  expect(parser, SYNTAX_KIND_KEYWORD_PROCEDURE);
  expect(parser, SYNTAX_KIND_IDENTIFIER);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS)) {
    parse_formal_parameter_list(parser);
  }
  expect(parser, SYNTAX_KIND_SEMICOLON);
  if (check(parser, SYNTAX_KIND_KEYWORD_VAR)) {
    parse_variable_declaration_part(parser);
  }
  parse_compound_statement(parser);
  expect(parser, SYNTAX_KIND_SEMICOLON);
  node_finish(parser);
}

static void parse_program(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PROGRAM);
  expect(parser, SYNTAX_KIND_KEYWORD_PROGRAM);
  expect(parser, SYNTAX_KIND_IDENTIFIER);
  expect(parser, SYNTAX_KIND_SEMICOLON);
  while (1) {
    if (check(parser, SYNTAX_KIND_KEYWORD_VAR)) {
      parse_variable_declaration_part(parser);
    } else if (check(parser, SYNTAX_KIND_KEYWORD_PROCEDURE)) {
      parse_procedure_declaration(parser);
    } else {
      break;
    }
  }
  parse_compound_statement(parser);
  expect(parser, SYNTAX_KIND_DOT);
  expect(parser, SYNTAX_KIND_EOF);
  node_finish(parser);
}

int parser_parse(const char *source, unsigned long size, TokenTree *tree)
{
  Parser parser;
  vector_init(&parser.parents, sizeof(Bookmark));
  vector_init(&parser.children, sizeof(TokenNode));
  token_cursor_init(&parser.cursor, source, size);
  token_cursor_next(&parser.cursor, &parser.token.token);

  parse_program(&parser);
  *tree = *(TokenTree *) vector_data(&parser.children);

  vector_deinit(&parser.parents);
  vector_deinit(&parser.children);
  return 1;
}
