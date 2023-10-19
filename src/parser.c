#include <string.h>

#include "lexer.h"
#include "parser.h"

void parser_init(Parser *parser, const char *source, unsigned long size)
{
  parser->_source = source;
  parser->_size   = size;
  parser->_offset = 0;
  lexer_init(&parser->_lexer, source, size);
  parser_bump(parser);
}

void parser_token(Parser *parser, LexerToken *token)
{
  *token = parser->_token;
}

void parser_bump(Parser *parser)
{
  lexer_next_token(&parser->_lexer, &parser->_token);
  parser->_offset += parser->_token.size;
}

int parser_check(Parser *parser, LexerTokenKind kind)
{
  return parser->_token.kind == kind;
}

int parser_check_keyword(Parser *parser, const char *keyword)
{
  return parser_check(parser, LEXER_TOKEN_KIND_IDENTIFIER)
    && !strncmp(parser->_source + parser->_offset - parser->_token.size, keyword, parser->_token.size);
}

int parser_eat(Parser *parser, LexerTokenKind kind)
{
  int result = parser_check(parser, kind);
  if (result) {
    parser_bump(parser);
  }
  return result;
}

int parser_eat_keyword(Parser *parser, const char *keyword)
{
  int result = parser_check_keyword(parser, keyword);
  if (result) {
    parser_bump(parser);
  }
  return result;
}

int parser_expect(Parser *parser, LexerTokenKind kind);

int parser_expect_keyword(Parser *parser, const char *keyword);
