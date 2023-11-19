#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "bit_set.h"
#include "parser.h"
#include "report.h"
#include "source.h"
#include "syntax_kind.h"
#include "token.h"
#include "token_cursor.h"
#include "utility.h"
#include "vector.h"

typedef struct Parser Parser;

struct Parser {
  TokenCursor   cursor;
  unsigned long offset;
  Token        *token;
  Vector        parents;
  Vector        children;
  BitSet        expected[bit_set_bits_to_buckets(SYNTAX_KIND_EOF_TOKEN + 1)];
  Vector        errors;
  int           alive;
};

static unsigned long node_checkpoint(Parser *parser)
{
  return vector_count(&parser->children);
}

static void node_start_at(Parser *parser, unsigned long checkpoint)
{
  vector_push(&parser->parents, &checkpoint);
}

static void node_start(Parser *parser)
{
  node_start_at(parser, node_checkpoint(parser));
}

static void node_finish(Parser *parser, SyntaxKind kind)
{
  TokenTree    *tree       = xmalloc(sizeof(TokenTree));
  unsigned long checkpoint = *(unsigned long *) vector_back(&parser->parents);

  vector_pop(&parser->parents);
  token_tree_init(tree, kind, vector_at(&parser->children, checkpoint), vector_count(&parser->children) - checkpoint);
  while (vector_count(&parser->children) > checkpoint) {
    vector_pop(&parser->children);
  }
  vector_push(&parser->children, &tree);
}

static void node_null(Parser *parser)
{
  TokenNode *node = NULL;
  vector_push(&parser->children, &node);
}

static Token *token(Parser *parser)
{
  Token  token;
  Report report;

  if (parser->token) {
    return parser->token;
  }

  parser->offset = token_cursor_offset(&parser->cursor);
  if (token_cursor_next(&parser->cursor, &token, &report)) {
    if (token.kind == SYNTAX_KIND_BAD_TOKEN) {
      vector_push(&parser->errors, &report);
    }
    parser->token  = xmalloc(sizeof(Token));
    *parser->token = token;
    return parser->token;
  } else {
    return NULL;
  }
}

static void bump(Parser *parser)
{
  if (token(parser)) {
    bit_set_zero(parser->expected, SYNTAX_KIND_EOF_TOKEN + 1);
    vector_push(&parser->children, &parser->token);
    parser->token = NULL;
  }
}

static int check_any(Parser *parser, const SyntaxKind *kinds, unsigned long count)
{
  if (!parser->alive) {
    return 0;
  } else {
    unsigned long i;
    for (i = 0; i < count; ++i) {
      bit_set_set(parser->expected, kinds[i], 1);
    }
    for (i = 0; i < count; ++i) {
      if (token(parser)->kind == kinds[i]) {
        return 1;
      }
    }
    return 0;
  }
}

static int check(Parser *parser, SyntaxKind kind)
{
  return check_any(parser, &kind, 1);
}

static int eat_any(Parser *parser, const SyntaxKind *kinds, unsigned long count)
{
  if (!parser->alive) {
    return 0;
  } else if (check_any(parser, kinds, count)) {
    bump(parser);
    return 1;
  } else {
    return 0;
  }
}

static int eat(Parser *parser, SyntaxKind kind)
{
  return eat_any(parser, &kind, 1);
}

static const char *SYNTAX_KIND_DISPLAY_STRING[] = {
  "",
  "identifier",
  "integer",
  "string",
  "`+`",
  "`-`",
  "`*`",
  "`=`",
  "`<>`",
  "`<`",
  "`<=`",
  "`>`",
  "`>=`",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "`:=`",
  "`.`",
  "`,`",
  "`:`",
  "`;`",
  "`program`",
  "`var`",
  "`array`",
  "`of`",
  "`begin`",
  "`end`",
  "`if`",
  "`then`",
  "`else`",
  "`procedure`",
  "`return`",
  "`call`",
  "`while`",
  "`do`",
  "`not`",
  "`or`",
  "`div`",
  "`and`",
  "`char`",
  "`integer`",
  "`boolean`",
  "`read`",
  "`write`",
  "`readln`",
  "`writeln`",
  "`true`",
  "`false`",
  "`break`",
  "EOF",
};

static void error_unexpected(Parser *parser)
{
  if (token(parser)->kind != SYNTAX_KIND_BAD_TOKEN) {
    Report        report;
    char          expected[1024];
    unsigned long cursor = 0;
    SyntaxKind    kind;

    for (kind = 0; kind <= SYNTAX_KIND_EOF_TOKEN; ++kind) {
      if (bit_set_get(parser->expected, kind)) {
        if (cursor > 0) {
          if (bit_set_count(parser->expected, SYNTAX_KIND_EOF_TOKEN + 1) > 1) {
            cursor += sprintf(expected + cursor, ", ");
          } else {
            cursor += sprintf(expected + cursor, " and ");
          }
        }
        cursor += sprintf(expected + cursor, "%s", SYNTAX_KIND_DISPLAY_STRING[kind]);
        bit_set_set(parser->expected, kind, 0);
      }
    }
    report_init(&report, REPORT_KIND_ERROR, parser->offset, parser->offset + token(parser)->text_length,
      "expected %s, actual `%s`", expected, token(parser)->text);
    vector_push(&parser->errors, &report);
  }

  parser->alive = 0;
  bump(parser);
}

static int expect_any(Parser *parser, const SyntaxKind *kinds, unsigned long count)
{
  if (!parser->alive) {
    node_null(parser);
    return 0;
  } else if (eat_any(parser, kinds, count)) {
    return 1;
  } else {
    error_unexpected(parser);
    return 0;
  }
}

static int expect(Parser *parser, SyntaxKind kind)
{
  return expect_any(parser, &kind, 1);
}

static const SyntaxKind FIRST_STANDARD_TYPE[] = {
  SYNTAX_KIND_INTEGER_KEYWORD,
  SYNTAX_KIND_BOOLEAN_KEYWORD,
  SYNTAX_KIND_CHAR_KEYWORD,
};

static const unsigned long FIRST_STANDARD_TYPE_COUNT = sizeof(FIRST_STANDARD_TYPE) / sizeof(SyntaxKind);

static void parse_standard_type(Parser *parser)
{
  expect_any(parser, FIRST_STANDARD_TYPE, FIRST_STANDARD_TYPE_COUNT);
}

static void parse_array_type(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_ARRAY_KEYWORD);
  expect(parser, SYNTAX_KIND_LEFT_BRACKET_TOKEN);
  expect(parser, SYNTAX_KIND_INTEGER_LITERAL);
  expect(parser, SYNTAX_KIND_RIGHT_BRACKET_TOKEN);
  expect(parser, SYNTAX_KIND_OF_KEYWORD);
  parse_standard_type(parser);
  node_finish(parser, SYNTAX_KIND_ARRAY_TYPE);
}

static void parse_type(Parser *parser)
{
  if (check_any(parser, FIRST_STANDARD_TYPE, FIRST_STANDARD_TYPE_COUNT)) {
    parse_standard_type(parser);
  } else {
    parse_array_type(parser);
  }
}

static void parse_factor(Parser *parser);
static void parse_expression(Parser *parser);

static void parse_variable(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  if (eat(parser, SYNTAX_KIND_LEFT_BRACKET_TOKEN)) {
    parse_expression(parser);
    expect(parser, SYNTAX_KIND_RIGHT_BRACKET_TOKEN);
    node_finish(parser, SYNTAX_KIND_INDEXED_VARIABLE);
  } else {
    node_finish(parser, SYNTAX_KIND_ENTIRE_VARIABLE);
  }
}

static void parse_parenthesized_expression(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYNTAX_KIND_PARENTHESIZED_EXPRESSION);
}

static void parse_not_expression(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_NOT_KEYWORD);
  parse_factor(parser);
  node_finish(parser, SYNTAX_KIND_NOT_EXPRESSION);
}

static void parse_cast_expression(Parser *parser)
{
  node_start(parser);
  parse_standard_type(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYNTAX_KIND_CAST_EXPRESSION);
}

static const SyntaxKind FIRST_CONSTANT[] = {
  SYNTAX_KIND_INTEGER_LITERAL,
  SYNTAX_KIND_TRUE_KEYWORD,
  SYNTAX_KIND_FALSE_KEYWORD,
  SYNTAX_KIND_STRING_LITERAL,
};

static const unsigned long FIRST_CONSTANT_COUNT = sizeof(FIRST_CONSTANT) / sizeof(SyntaxKind);

static void parse_factor(Parser *parser)
{
  if (check(parser, SYNTAX_KIND_IDENTIFIER_TOKEN)) {
    parse_variable(parser);
  } else if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN)) {
    parse_parenthesized_expression(parser);
  } else if (check(parser, SYNTAX_KIND_NOT_KEYWORD)) {
    parse_not_expression(parser);
  } else if (check_any(parser, FIRST_STANDARD_TYPE, FIRST_STANDARD_TYPE_COUNT)) {
    parse_cast_expression(parser);
  } else {
    expect_any(parser, FIRST_CONSTANT, FIRST_CONSTANT_COUNT);
  }
}

static const SyntaxKind FIRST_MULTIPLICATIVE_OPERATOR[] = {
  SYNTAX_KIND_STAR_TOKEN,
  SYNTAX_KIND_DIV_KEYWORD,
  SYNTAX_KIND_AND_KEYWORD,
};

static const unsigned long FIRST_MULTIPLICATIVE_OPERATOR_COUNT = sizeof(FIRST_MULTIPLICATIVE_OPERATOR) / sizeof(SyntaxKind);

static void parse_term(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_factor(parser);
  while (eat_any(parser, FIRST_MULTIPLICATIVE_OPERATOR, FIRST_MULTIPLICATIVE_OPERATOR_COUNT)) {
    node_start_at(parser, checkpoint);
    parse_factor(parser);
    node_finish(parser, SYNTAX_KIND_BINARY_EXPRESSION);
  }
}

static const SyntaxKind FIRST_ADDITIVE_OPERATOR[] = {
  SYNTAX_KIND_PLUS_TOKEN,
  SYNTAX_KIND_MINUS_TOKEN,
  SYNTAX_KIND_OR_KEYWORD,
};

static const unsigned long FIRST_ADDITIVE_OPERATOR_COUNT = sizeof(FIRST_ADDITIVE_OPERATOR) / sizeof(SyntaxKind);

static void parse_simple_expression(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  if (check_any(parser, FIRST_ADDITIVE_OPERATOR, FIRST_ADDITIVE_OPERATOR_COUNT)) {
    node_null(parser);
  } else {
    parse_term(parser);
  }
  while (eat_any(parser, FIRST_ADDITIVE_OPERATOR, FIRST_ADDITIVE_OPERATOR_COUNT)) {
    node_start_at(parser, checkpoint);
    parse_term(parser);
    node_finish(parser, SYNTAX_KIND_BINARY_EXPRESSION);
  }
}

static const SyntaxKind FIRST_RELATIONAL_OPERATOR[] = {
  SYNTAX_KIND_EQUAL_TOKEN,
  SYNTAX_KIND_NOT_EQUAL_TOKEN,
  SYNTAX_KIND_LESS_THAN_TOKEN,
  SYNTAX_KIND_LESS_THAN_EQUAL_TOKEN,
  SYNTAX_KIND_GREATER_THAN_TOKEN,
  SYNTAX_KIND_GREATER_THAN_EQUAL_TOKEN,
};

static const unsigned long FIRST_RELATIONAL_OPERATOR_COUNT = sizeof(FIRST_RELATIONAL_OPERATOR) / sizeof(SyntaxKind);

static void parse_expression(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_simple_expression(parser);
  while (eat_any(parser, FIRST_RELATIONAL_OPERATOR, FIRST_RELATIONAL_OPERATOR_COUNT)) {
    node_start_at(parser, checkpoint);
    parse_simple_expression(parser);
    node_finish(parser, SYNTAX_KIND_BINARY_EXPRESSION);
  }
}

static void parse_statement(Parser *parser);

static void parse_assignment_statement(Parser *parser)
{
  node_start(parser);
  parse_variable(parser);
  expect(parser, SYNTAX_KIND_ASSIGN_TOKEN);
  parse_expression(parser);
  node_finish(parser, SYNTAX_KIND_ASSIGNMENT_STATEMENT);
}

static void parse_if_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_IF_KEYWORD);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_THEN_KEYWORD);
  parse_statement(parser);
  if (eat(parser, SYNTAX_KIND_ELSE_KEYWORD)) {
    parse_statement(parser);
  } else {
    node_null(parser);
    node_null(parser);
  }
  node_finish(parser, SYNTAX_KIND_IF_STATEMENT);
}

static void parse_while_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_WHILE_KEYWORD);
  parse_expression(parser);
  expect(parser, SYNTAX_KIND_DO_KEYWORD);
  parse_statement(parser);
  node_finish(parser, SYNTAX_KIND_WHILE_STATEMENT);
}

static void parse_break_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_BREAK_KEYWORD);
  node_finish(parser, SYNTAX_KIND_RETURN_STATEMENT);
}

static void parse_actual_parameter_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  do {
    parse_expression(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA_TOKEN));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYNTAX_KIND_ACTUAL_PARAMETER_LIST);
}

static void parse_call_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_CALL_KEYWORD);
  expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN)) {
    parse_actual_parameter_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_KIND_CALL_STATEMENT);
}

static void parse_return_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_RETURN_KEYWORD);
  node_finish(parser, SYNTAX_KIND_RETURN_STATEMENT);
}

static void parse_input_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  do {
    parse_variable(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA_TOKEN));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYTANX_KIND_INPUT_LIST);
}

static const SyntaxKind FIRST_INPUT_STATEMENT[] = {
  SYNTAX_KIND_READ_KEYWORD,
  SYNTAX_KIND_READLN_KEYWORD,
};

static const unsigned long FIRST_INPUT_STATEMENT_COUNT = sizeof(FIRST_INPUT_STATEMENT) / sizeof(SyntaxKind);

static void parse_input_statement(Parser *parser)
{
  node_start(parser);
  expect_any(parser, FIRST_INPUT_STATEMENT, FIRST_INPUT_STATEMENT_COUNT);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN)) {
    parse_input_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_KIND_INPUT_STATEMENT);
}

static void parse_output_value(Parser *parser)
{
  node_start(parser);
  parse_expression(parser);
  if (eat(parser, SYNTAX_KIND_COLON_TOKEN)) {
    expect(parser, SYNTAX_KIND_INTEGER_LITERAL);
  } else {
    node_null(parser);
    node_null(parser);
  }
  node_finish(parser, SYNTAX_KIND_OUTPUT_VALUE);
}

static void parse_output_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  do {
    parse_output_value(parser);
  } while (eat(parser, SYNTAX_KIND_COMMA_TOKEN));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYNTAX_KIND_OUTPUT_LIST);
}

static const SyntaxKind FIRST_OUTPUT_STATEMENT[] = {
  SYNTAX_KIND_WRITE_KEYWORD,
  SYNTAX_KIND_WRITELN_KEYWORD,
};

static const unsigned long FIRST_OUTPUT_STATEMENT_COUNT = sizeof(FIRST_OUTPUT_STATEMENT) / sizeof(SyntaxKind);

static void parse_output_statement(Parser *parser)
{
  node_start(parser);
  expect_any(parser, FIRST_OUTPUT_STATEMENT, FIRST_OUTPUT_STATEMENT_COUNT);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN)) {
    parse_output_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_KIND_OUTPUT_STATEMENT);
}

static void parse_compound_statement(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_BEGIN_KEYWORD);
  do {
    parse_statement(parser);
  } while (eat(parser, SYNTAX_KIND_SEMICOLON_TOKEN));
  expect(parser, SYNTAX_KIND_END_KEYWORD);
  node_finish(parser, SYNTAX_KIND_COMPOUND_STATEMENT);
}

static void parse_statement(Parser *parser)
{
  if (check(parser, SYNTAX_KIND_IDENTIFIER_TOKEN)) {
    parse_assignment_statement(parser);
  } else if (check(parser, SYNTAX_KIND_IF_KEYWORD)) {
    parse_if_statement(parser);
  } else if (check(parser, SYNTAX_KIND_WHILE_KEYWORD)) {
    parse_while_statement(parser);
  } else if (check(parser, SYNTAX_KIND_BREAK_KEYWORD)) {
    parse_break_statement(parser);
  } else if (check(parser, SYNTAX_KIND_CALL_KEYWORD)) {
    parse_call_statement(parser);
  } else if (check(parser, SYNTAX_KIND_RETURN_KEYWORD)) {
    parse_return_statement(parser);
  } else if (check_any(parser, FIRST_INPUT_STATEMENT, FIRST_INPUT_STATEMENT_COUNT)) {
    parse_input_statement(parser);
  } else if (check_any(parser, FIRST_OUTPUT_STATEMENT, FIRST_OUTPUT_STATEMENT_COUNT)) {
    parse_output_statement(parser);
  } else if (check(parser, SYNTAX_KIND_BEGIN_KEYWORD)) {
    parse_compound_statement(parser);
  } else {
    node_null(parser);
  }
}

static void parse_variable_declaration(Parser *parser)
{
  node_start(parser);
  do {
    expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  } while (eat(parser, SYNTAX_KIND_COMMA_TOKEN));
  expect(parser, SYNTAX_KIND_COLON_TOKEN);
  parse_type(parser);
  expect(parser, SYNTAX_KIND_SEMICOLON_TOKEN);
  node_finish(parser, SYNTAX_KIND_VARIABLE_DECLARATION);
}

static void parse_variable_declaration_part(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_VAR_KEYWORD);
  do {
    parse_variable_declaration(parser);
  } while (check(parser, SYNTAX_KIND_IDENTIFIER_TOKEN));
  node_finish(parser, SYNTAX_KIND_VARIABLE_DECLARATION_PART);
}

static void parse_formal_parameter_section(Parser *parser)
{
  node_start(parser);
  do {
    expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  } while (eat(parser, SYNTAX_KIND_COMMA_TOKEN));
  expect(parser, SYNTAX_KIND_COLON_TOKEN);
  parse_type(parser);
  node_finish(parser, SYNTAX_KIND_FORMAL_PARAMETER_SECTION);
}

static void parse_formal_parameter_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN);
  do {
    parse_formal_parameter_section(parser);
  } while (eat(parser, SYNTAX_KIND_SEMICOLON_TOKEN));
  expect(parser, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN);
  node_finish(parser, SYNTAX_KIND_FORMAL_PARAMETER_LIST);
}

static void parse_procedure_declaration(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_PROCEDURE_KEYWORD);
  expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  if (check(parser, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN)) {
    parse_formal_parameter_list(parser);
  } else {
    node_null(parser);
  }
  expect(parser, SYNTAX_KIND_SEMICOLON_TOKEN);
  if (check(parser, SYNTAX_KIND_VAR_KEYWORD)) {
    parse_variable_declaration_part(parser);
  } else {
    node_null(parser);
  }
  parse_compound_statement(parser);
  expect(parser, SYNTAX_KIND_SEMICOLON_TOKEN);
  node_finish(parser, SYNTAX_KIND_PROCEDURE_DECLARATION);
}

static void parse_program(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_KIND_PROGRAM_KEYWORD);
  expect(parser, SYNTAX_KIND_IDENTIFIER_TOKEN);
  expect(parser, SYNTAX_KIND_SEMICOLON_TOKEN);
  while (1) {
    if (check(parser, SYNTAX_KIND_VAR_KEYWORD)) {
      parse_variable_declaration_part(parser);
    } else if (check(parser, SYNTAX_KIND_PROCEDURE_KEYWORD)) {
      parse_procedure_declaration(parser);
    } else {
      break;
    }
  }
  parse_compound_statement(parser);
  expect(parser, SYNTAX_KIND_DOT_TOKEN);
  expect(parser, SYNTAX_KIND_EOF_TOKEN);
  node_finish(parser, SYNTAX_KIND_PROGRAM);
}

int mppl_parse(const Source *source, TokenTree *tree)
{
  Parser parser;
  int    result;
  vector_init(&parser.parents, sizeof(unsigned long));
  vector_init(&parser.children, sizeof(TokenNode *));
  vector_init(&parser.errors, sizeof(Report));
  bit_set_zero(parser.expected, SYNTAX_KIND_EOF_TOKEN + 1);
  token_cursor_init(&parser.cursor, source);
  parser.token = NULL;
  parser.alive = 1;

  parse_program(&parser);
  *tree = **(TokenTree **) vector_data(&parser.children);
  free(*(TokenNode **) vector_data(&parser.children));

  {
    unsigned long i;
    for (i = 0; i < vector_count(&parser.errors); ++i) {
      Report *report = vector_at(&parser.errors, i);
      report_emit(report, source);
    }
    fflush(stdout);
  }
  result = !vector_count(&parser.errors);

  vector_deinit(&parser.parents);
  vector_deinit(&parser.children);
  vector_deinit(&parser.errors);
  return result;
}
