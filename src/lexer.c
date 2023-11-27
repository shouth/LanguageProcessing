#include <stdio.h>
#include <string.h>

#include "report.h"
#include "source.h"
#include "syntax_kind.h"
#include "token.h"

typedef struct Lexer Lexer;

struct Lexer {
  const Source *source;
  unsigned long offset;
  unsigned long index;
};

static void bump(Lexer *lexer)
{
  if (lexer->offset + lexer->index < lexer->source->text_length) {
    ++lexer->index;
  }
}

static int first(Lexer *lexer)
{
  return lexer->offset + lexer->index < lexer->source->text_length
    ? lexer->source->text[lexer->offset + lexer->index]
    : EOF;
}

static int eat(Lexer *lexer, int c)
{
  int result = first(lexer) == c;
  if (result) {
    bump(lexer);
  }
  return result;
}

static int eat_if(Lexer *lexer, int (*predicate)(int))
{
  int result = predicate(first(lexer));
  if (result) {
    bump(lexer);
  }
  return result;
}

static int is_alphabet(int c)
{
  return !!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c);
}

static int is_number(int c)
{
  return c >= '0' && c <= '9';
}

static int is_space(int c)
{
  return !!strchr(" \t", c);
}

static int is_newline(int c)
{
  return !!strchr("\r\n", c);
}

static int is_graphic(int c)
{
  return is_alphabet(c) || is_number(c) || is_space(c) || is_newline(c) || !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c);
}

static int tokenize(Lexer *lexer, SyntaxKind kind, TokenInfo *info)
{
  token_info_init(info, kind, lexer->source->text + lexer->offset, lexer->index);
  lexer->offset += lexer->index;
  lexer->index = 0;
  return 1;
}

static int token_error(Lexer *lexer, TokenInfo *info, Report *report, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_init_with_args(report, REPORT_KIND_ERROR, lexer->offset, format, args);
  va_end(args);
  return tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
}

static int token_unexpected(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (is_graphic(lexer->source->text[lexer->offset])) {
    return token_error(lexer, info, report, "stray '%c' in program", lexer->source->text[lexer->offset]);
  } else {
    return token_error(lexer, info, report, "stray '\\%o' in program", (int) lexer->source->text[lexer->offset]);
  }
}

static int token_identifier_and_keyword(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat_if(lexer, &is_alphabet)) {
    SyntaxKind kind;
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    kind = syntax_kind_from_keyword(lexer->source->text + lexer->offset, lexer->index);
    return tokenize(lexer, kind != SYNTAX_KIND_BAD_TOKEN ? kind : SYNTAX_KIND_IDENTIFIER_TOKEN, info);
  } else {
    return token_unexpected(lexer, info, report);
  }
}

static int token_integer(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    return tokenize(lexer, SYNTAX_KIND_INTEGER_LITERAL, info);
  } else {
    return token_unexpected(lexer, info, report);
  }
}

static int token_string(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat(lexer, '\'')) {
    int contain_non_graphic = 0;
    while (1) {
      if (eat(lexer, '\'') && !eat(lexer, '\'')) {
        if (contain_non_graphic) {
          return token_error(lexer, info, report, "string contains non-graphic character");
        } else {
          return tokenize(lexer, SYNTAX_KIND_STRING_LITERAL, info);
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOF) {
        return token_error(lexer, info, report, "string is unterminated");
      } else {
        contain_non_graphic |= !eat_if(lexer, &is_graphic);
      }
    }
  } else {
    return token_unexpected(lexer, info, report);
  }
}

static int token_whitespace(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    return tokenize(lexer, SYNTAX_KIND_SPACE_TRIVIA, info);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, info);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, info);
  } else {
    return token_unexpected(lexer, info, report);
  }
}

static int token_comment(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat(lexer, '{')) {
    while (1) {
      if (eat(lexer, '}')) {
        return tokenize(lexer, SYNTAX_KIND_BRACES_COMMENT_TRIVIA, info);
      } else if (first(lexer) == EOF) {
        return token_error(lexer, info, report, "comment is unterminated");
      } else {
        bump(lexer);
      }
    }
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      while (1) {
        if (eat(lexer, '*') && eat(lexer, '/')) {
          return tokenize(lexer, SYNTAX_KIND_C_COMMENT_TRIVIA, info);
        } else if (first(lexer) == EOF) {
          return token_error(lexer, info, report, "comment is unterminated");
        } else {
          bump(lexer);
        }
      }
    } else {
      return token_unexpected(lexer, info, report);
    }
  } else {
    return token_unexpected(lexer, info, report);
  }
}

static int token_symbol(Lexer *lexer, TokenInfo *info, Report *report)
{
  if (eat(lexer, '+')) {
    return tokenize(lexer, SYNTAX_KIND_PLUS_TOKEN, info);
  } else if (eat(lexer, '-')) {
    return tokenize(lexer, SYNTAX_KIND_MINUS_TOKEN, info);
  } else if (eat(lexer, '*')) {
    return tokenize(lexer, SYNTAX_KIND_STAR_TOKEN, info);
  } else if (eat(lexer, '=')) {
    return tokenize(lexer, SYNTAX_KIND_EQUAL_TOKEN, info);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return tokenize(lexer, SYNTAX_KIND_NOT_EQUAL_TOKEN, info);
    } else if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_EQUAL_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_TOKEN, info);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_EQUAL_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_TOKEN, info);
    }
  } else if (eat(lexer, '(')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN, info);
  } else if (eat(lexer, ')')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN, info);
  } else if (eat(lexer, '[')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_BRACKET_TOKEN, info);
  } else if (eat(lexer, ']')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_BRACKET_TOKEN, info);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_ASSIGN_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_COLON_TOKEN, info);
    }
  } else if (eat(lexer, '.')) {
    return tokenize(lexer, SYNTAX_KIND_DOT_TOKEN, info);
  } else if (eat(lexer, ',')) {
    return tokenize(lexer, SYNTAX_KIND_COMMA_TOKEN, info);
  } else if (eat(lexer, ';')) {
    return tokenize(lexer, SYNTAX_KIND_SEMICOLON_TOKEN, info);
  } else {
    return token_unexpected(lexer, info, report);
  }
}

int mppl_lex(const Source *source, unsigned long offset, TokenInfo *info, Report *report)
{
  Lexer lexer;
  lexer.source = source;
  lexer.offset = offset;
  lexer.index  = 0;

  if (first(&lexer) == EOF) {
    return tokenize(&lexer, SYNTAX_KIND_EOF_TOKEN, info);
  } else if (is_alphabet(first(&lexer))) {
    return token_identifier_and_keyword(&lexer, info, report);
  } else if (is_number(first(&lexer))) {
    return token_integer(&lexer, info, report);
  } else if (first(&lexer) == '\'') {
    return token_string(&lexer, info, report);
  } else if (is_space(first(&lexer)) || is_newline(first(&lexer))) {
    return token_whitespace(&lexer, info, report);
  } else if (first(&lexer) == '{' || first(&lexer) == '/') {
    return token_comment(&lexer, info, report);
  } else {
    return token_symbol(&lexer, info, report);
  }
}
