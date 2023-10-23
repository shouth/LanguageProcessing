#include "lexer.h"
#include "lexical_tree.h"
#include "parser.h"
#include "symbol.h"
#include "vector.h"

static void bump(Parser *parser)
{
  lexer_next_token(&parser->_lexer, &parser->_token);
  parser->_offset += parser->_token.size;
  parser->_symbol = symbol_from(parser->_source + parser->_offset - parser->_token.size, parser->_token.size);
}

static int check(Parser *parser, LexerTokenKind kind)
{
  return parser->_token.kind == kind;
}

static int check_keyword(Parser *parser, Symbol keyword)
{
  return check(parser, LEXER_TOKEN_KIND_IDENTIFIER) && keyword == parser->_symbol;
}

static int check_identifier(Parser *parser)
{
  return check(parser, LEXER_TOKEN_KIND_IDENTIFIER) && !symbol_is_keyword(parser->_symbol);
}

static int eat(Parser *parser, LexerTokenKind kind)
{
  int result = check(parser, kind);
  if (result) {
    bump(parser);
  }
  return result;
}

static int eat_keyword(Parser *parser, Symbol keyword)
{
  int result = check_keyword(parser, keyword);
  if (result) {
    bump(parser);
  }
  return result;
}

static int eat_identifier(Parser *parser)
{
  int result = check_identifier(parser);
  if (result) {
    bump(parser);
  }
  return result;
}

static int expect(Parser *parser, LexerTokenKind kind)
{
  int result = eat(parser, kind);
  if (!result) {
    /* TODO: create an error object and push to parser */
  }
  return result;
}

static int expect_keyword(Parser *parser, Symbol keyword)
{
  int result = eat_keyword(parser, keyword);
  if (!result) {
    /* TODO: create an error object and push to parser */
  }
  return result;
}

static int expect_identifier(Parser *parser)
{
  int result = eat_identifier(parser);
  if (!result) {
    /* TODO: create an error object and push to parser */
  }
  return result;
}

static unsigned long node_checkpoint(Parser *parser)
{

}

static void node_start(Parser *parser, SyntaxKind kind)
{
}

static void node_start_at(Parser *parser, SyntaxKind kind, unsigned long checkpoint)
{
}

static void node_finish(Parser *parser)
{
}

static int check_standard_type(Parser *parser)
{
  return check_keyword(parser, SYMBOL_KEYWORD_INTEGER)
    || check_keyword(parser, SYMBOL_KEYWORD_BOOLEAN)
    || check_keyword(parser, SYMBOL_KEYWORD_CHAR);
}

static int eat_standard_type(Parser *parser)
{
  int result = check_standard_type(parser);
  if (result) {
    bump(parser);
  }
  return result;
}

static void parse_standard_type(Parser *parser)
{
  if (!eat_standard_type(parser)) {
    /* TODO: make error */
  }
}

static void parse_array_type(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_ARRAY_TYPE);
  expect_keyword(parser, SYMBOL_KEYWORD_ARRAY);
  expect(parser, LEXER_TOKEN_KIND_LEFT_BRACKET);
  expect(parser, LEXER_TOKEN_KIND_INTEGER);
  expect(parser, LEXER_TOKEN_KIND_RIGHT_BRACKET);
  expect_keyword(parser, SYMBOL_KEYWORD_OF);
  parse_standard_type(parser);
  node_finish(parser);
}

static void parse_type(Parser *parser)
{
  if (check_standard_type(parser)) {
    parse_standard_type(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_ARRAY)) {
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
  expect_identifier(parser);
  if (eat(parser, LEXER_TOKEN_KIND_LEFT_BRACKET)) {
    parse_expression(parser);
    expect(parser, LEXER_TOKEN_KIND_RIGHT_BRACKET);
  }
  node_finish(parser);
}

static void parse_parenthesized_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PARENTHESIZED_EXPRESSION);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  parse_expression(parser);
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_not_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_NOT_EXPRESSION);
  expect_keyword(parser, SYMBOL_KEYWORD_NOT);
  parse_factor(parser);
  node_finish(parser);
}

static void parse_cast_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_CAST_EXPRESSION);
  parse_standard_type(parser);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  parse_expression(parser);
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_empty_expression(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_EMPTY_EXPRESSION);
  node_finish(parser);
}

static void parse_factor(Parser *parser)
{
  if (check_identifier(parser)) {
    parse_variable(parser);
  } else if (check(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS)) {
    parse_parenthesized_expression(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_NOT)) {
    parse_not_expression(parser);
  } else if (check_standard_type(parser)) {
    parse_cast_expression(parser);
  } else if (!(eat(parser, LEXER_TOKEN_KIND_INTEGER) || eat_keyword(parser, SYMBOL_KEYWORD_TRUE) || eat_keyword(parser, SYMBOL_KEYWORD_FALSE) || eat(parser, LEXER_TOKEN_KIND_STRING))) {
    /* make error */
  }
}

static int check_multicative_operator(Parser *parser)
{
  return check(parser, LEXER_TOKEN_KIND_STAR)
    || check_keyword(parser, SYMBOL_KEYWORD_DIV)
    || check_keyword(parser, SYMBOL_KEYWORD_AND);
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
  return check(parser, LEXER_TOKEN_KIND_PLUS)
    || check(parser, LEXER_TOKEN_KIND_MINUS)
    || check_keyword(parser, SYMBOL_KEYWORD_OR);
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
  return check(parser, LEXER_TOKEN_KIND_EQUAL)
    || check(parser, LEXER_TOKEN_KIND_NOT_EQUAL)
    || check(parser, LEXER_TOKEN_KIND_LESS_THAN)
    || check(parser, LEXER_TOKEN_KIND_LESS_THAN_EQUAL)
    || check(parser, LEXER_TOKEN_KIND_GREATER_THAN)
    || check(parser, LEXER_TOKEN_KIND_GREATER_THAN_EQUAL);
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
  expect(parser, LEXER_TOKEN_KIND_ASSIGN);
  parse_expression(parser);
  node_finish(parser);
}

static void parse_if_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_IF_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_IF);
  parse_expression(parser);
  expect_keyword(parser, SYMBOL_KEYWORD_THEN);
  parse_statement(parser);
  if (eat_keyword(parser, SYMBOL_KEYWORD_ELSE)) {
    parse_statement(parser);
  }
  node_finish(parser);
}

static void parse_while_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_WHILE_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_WHILE);
  parse_expression(parser);
  expect_keyword(parser, SYMBOL_KEYWORD_DO);
  parse_statement(parser);
  node_finish(parser);
}

static void parse_break_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_RETURN_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_BREAK);
  node_finish(parser);
}

static void parse_actual_parameter_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_ACTUAL_PARAMETER_LIST);
  do {
    parse_expression(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_COMMA));
  node_finish(parser);
}

static void parse_call_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_CALL_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_CALL);
  expect_identifier(parser);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  parse_actual_parameter_list(parser);
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_return_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_RETURN_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_RETURN);
  node_finish(parser);
}

static void parse_input_list(Parser *parser)
{
  node_start(parser, SYTANX_KIND_INPUT_LIST);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  do {
    parse_variable(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_COMMA));
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_input_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_INPUT_STATEMENT);
  if (!(eat_keyword(parser, SYMBOL_KEYWORD_READ) || eat_keyword(parser, SYMBOL_KEYWORD_READLN))) {
    /* TODO: make error */
  }
  parse_input_list(parser);
  node_finish(parser);
}

static void parse_output_value(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_VALUE);
  parse_expression(parser);
  if (eat(parser, LEXER_TOKEN_KIND_COLON)) {
    expect(parser, LEXER_TOKEN_KIND_INTEGER);
  }
  node_finish(parser);
}

static void parse_output_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_LIST);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  do {
    parse_output_value(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_COMMA));
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_output_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_OUTPUT_STATEMENT);
  if (!(eat_keyword(parser, SYMBOL_KEYWORD_WRITE) || eat_keyword(parser, SYMBOL_KEYWORD_WRITELN))) {
    /* TODO: make error */
  }
  parse_output_list(parser);
  node_finish(parser);
}

static void parse_compound_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_COMPOUND_STATEMENT);
  expect_keyword(parser, SYMBOL_KEYWORD_BEGIN);
  do {
    parse_statement(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_SEMICOLON));
  expect_keyword(parser, SYMBOL_KEYWORD_END);
  node_finish(parser);
}

static void parse_empty_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_EMPTY_STATEMENT);
  node_finish(parser);
}

static void parse_statement(Parser *parser)
{
  if (check_identifier(parser)) {
    parse_assignment_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_IF)) {
    parse_if_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_WHILE)) {
    parse_while_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_BREAK)) {
    parse_break_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_CALL)) {
    parse_call_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_RETURN)) {
    parse_return_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_READ) || check_keyword(parser, SYMBOL_KEYWORD_READLN)) {
    parse_input_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_WRITE) || check_keyword(parser, SYMBOL_KEYWORD_WRITELN)) {
    parse_output_statement(parser);
  } else if (check_keyword(parser, SYMBOL_KEYWORD_BEGIN)) {
    parse_compound_statement(parser);
  } else {
    parse_empty_statement(parser);
  }
}

static void parse_variable_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE_DECLARATION);
  do {
    expect_identifier(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_COMMA));
  expect(parser, LEXER_TOKEN_KIND_COLON);
  parse_type(parser);
  node_finish(parser);
}

static void parse_variable_declaration_part(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE_DECLARATION);
  expect_keyword(parser, SYMBOL_KEYWORD_VAR);
  do {
    parse_variable_declaration(parser);
  } while (check_identifier(parser));
  node_finish(parser);
}

static void parse_formal_parameter_section(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_FORMAL_PARAMETER_SECTION);
  do {
    expect_identifier(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_COMMA));
  expect(parser, LEXER_TOKEN_KIND_COLON);
  parse_type(parser);
  node_finish(parser);
}

static void parse_formal_parameter_list(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_FORMAL_PARAMETER_LIST);
  expect(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS);
  do {
    parse_formal_parameter_section(parser);
  } while (eat(parser, LEXER_TOKEN_KIND_SEMICOLON));
  expect(parser, LEXER_TOKEN_KIND_RIGHT_PARENTHESIS);
  node_finish(parser);
}

static void parse_procedure_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PROCEDURE_DECLARATION);
  expect_keyword(parser, SYMBOL_KEYWORD_PROCEDURE);
  expect_identifier(parser);
  if (check(parser, LEXER_TOKEN_KIND_LEFT_PARENTHESIS)) {
    parse_formal_parameter_list(parser);
  }
  expect(parser, LEXER_TOKEN_KIND_SEMICOLON);
  if (check_keyword(parser, SYMBOL_KEYWORD_VAR)) {
    parse_variable_declaration_part(parser);
  }
  parse_compound_statement(parser);
  node_finish(parser);
}

static void parse_program(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PROGRAM);
  expect_keyword(parser, SYMBOL_KEYWORD_PROGRAM);
  expect_identifier(parser);
  expect(parser, LEXER_TOKEN_KIND_SEMICOLON);
  while (1) {
    if (check_keyword(parser, SYMBOL_KEYWORD_VAR)) {
      parse_variable_declaration_part(parser);
    } else if (check_keyword(parser, SYMBOL_KEYWORD_PROCEDURE)) {
      parse_procedure_declaration(parser);
    } else {
      break;
    }
  }
  parse_compound_statement(parser);
  expect(parser, LEXER_TOKEN_KIND_DOT);
  node_finish(parser);
}

void parser_init(Parser *parser, const char *source, unsigned long size)
{
  parser->_source = source;
  parser->_size   = size;
  parser->_offset = 0;
  lexer_init(&parser->_lexer, source, size);
  bump(parser);
}
