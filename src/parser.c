#include <string.h>

#include "lexer.h"
#include "lexical_tree.h"
#include "parser.h"
#include "symbol.h"

static void bump(Parser *parser)
{
  lexer_next_token(&parser->_lexer, &parser->_token);
  parser->_offset += parser->_token.size;
}

static int check(Parser *parser, LexerTokenKind kind)
{
  return parser->_token.kind == kind;
}

static int check_keyword(Parser *parser, Symbol keyword)
{
  return check(parser, LEXER_TOKEN_KIND_IDENTIFIER)
    && !strncmp(parser->_source + parser->_offset - parser->_token.size, symbol_string(keyword), parser->_token.size);
}

static int check_identifier(Parser *parser)
{
  Symbol token = symbol_from(parser->_source + parser->_offset - parser->_token.size, parser->_token.size);
  return check(parser, LEXER_TOKEN_KIND_IDENTIFIER) && !symbol_is_keyword(token);
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

static void node_start(Parser *parser, SyntaxKind kind)
{
}

static void node_end(Parser *parser)
{
}

static void parse_compound_statement(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_COMPOUND_STATEMENT);

  node_end(parser);
}

static void parse_variable_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_VARIABLE_DECLARATOIN);

  node_end(parser);
}

static void parse_subprogram_declaration(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_SUBPROGRAM_DECLARATION);
  expect_keyword(parser, SYMBOL_KEYWORD_PROCEDURE);
  expect_identifier(parser);

  node_end(parser);
}

static void parse_block(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_BLOCK);
  while (1) {
    if (check_keyword(parser, SYMBOL_KEYWORD_VAR)) {
      parse_variable_declaration(parser);
    } else if (check_keyword(parser, SYMBOL_KEYWORD_PROCEDURE)) {
      parse_subprogram_declaration(parser);
    } else {
      break;
    }
  }
  parse_compound_statement(parser);
  node_end(parser);
}

static void parse_program(Parser *parser)
{
  node_start(parser, SYNTAX_KIND_PROGRAM);
  expect_keyword(parser, SYMBOL_KEYWORD_PROGRAM);
  expect_identifier(parser);
  expect(parser, LEXER_TOKEN_KIND_SEMICOLON);
  parse_block(parser);
  expect(parser, LEXER_TOKEN_KIND_DOT);
  node_end(parser);
}

void parser_init(Parser *parser, const char *source, unsigned long size)
{
  parser->_source = source;
  parser->_size   = size;
  parser->_offset = 0;
  lexer_init(&parser->_lexer, source, size);
  bump(parser);
}
