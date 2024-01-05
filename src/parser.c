#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "bitset.h"
#include "parser.h"
#include "report.h"
#include "source.h"
#include "syntax_kind.h"
#include "token_cursor.h"
#include "token_tree.h"
#include "utility.h"

typedef struct Parser Parser;

struct Parser {
  TokenCursor   cursor;
  unsigned long offset;
  Token        *token;
  TokenStatus   status;
  Array        *parents;
  Array        *children;
  BitSet       *expected;
  Array        *errors;
  int           alive;
};

static unsigned long node_checkpoint(Parser *parser)
{
  return array_count(parser->children);
}

static void node_start_at(Parser *parser, unsigned long checkpoint)
{
  array_push(parser->parents, &checkpoint);
}

static void node_start(Parser *parser)
{
  node_start_at(parser, node_checkpoint(parser));
}

static void node_finish(Parser *parser, SyntaxKind kind)
{
  unsigned long checkpoint = *(unsigned long *) array_back(parser->parents);
  TokenTree    *tree
    = token_tree_new(kind, array_at(parser->children, checkpoint), array_count(parser->children) - checkpoint);

  array_pop(parser->parents);
  while (array_count(parser->children) > checkpoint) {
    array_pop(parser->children);
  }
  array_push(parser->children, &tree);
}

static void node_null(Parser *parser)
{
  TokenNode *node = NULL;
  array_push(parser->children, &node);
}

static Token *token(Parser *parser)
{
  unsigned long i;

  if (parser->token) {
    return parser->token;
  }

  parser->status = token_cursor_next(&parser->cursor, &parser->token);
  for (i = 0; i < parser->token->trivia_count; ++i) {
    parser->offset += parser->token->trivia[i].text_length;
  }
  return parser->token;
}

static void bump(Parser *parser)
{
  if (token(parser)) {
    bitset_clear(parser->expected);
    array_push(parser->children, &parser->token);
    parser->offset += parser->token->text_length;
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
      bitset_set(parser->expected, kinds[i], 1);
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

static const char *SYNTAX_DISPLAY_STRING[] = {
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
  char          expected[1024];
  unsigned long cursor = 0;
  SyntaxKind    kind;
  Report       *report;

  for (kind = 0; kind <= SYNTAX_EOF_TOKEN; ++kind) {
    if (bitset_set(parser->expected, kind, -1)) {
      if (cursor > 0) {
        if (bitset_count(parser->expected) > 1) {
          cursor += sprintf(expected + cursor, ", ");
        } else {
          cursor += sprintf(expected + cursor, " and ");
        }
      }
      cursor += sprintf(expected + cursor, "%s", SYNTAX_DISPLAY_STRING[kind]);
      bitset_set(parser->expected, kind, 0);
    }
  }

  if (token(parser)->kind == SYNTAX_BAD_TOKEN) {
    switch (parser->status) {
    case TOKEN_ERROR_STRAY_CHAR: {
      if (is_graphic(token(parser)->text[0])) {
        report = report_new(REPORT_KIND_ERROR, parser->offset, "stray `%s` in program", token(parser)->text);
      } else {
        report = report_new(REPORT_KIND_ERROR, parser->offset, "stray `\\x%X` in program", (unsigned char) token(parser)->text[0]);
      }
      break;
    }
    case TOKEN_ERROR_NONGRAPHIC_CHAR: {
      report = report_new(REPORT_KIND_ERROR, parser->offset, "non-graphic character in string");
      break;
    }
    case TOKEN_ERROR_UNTERMINATED_STRING: {
      report = report_new(REPORT_KIND_ERROR, parser->offset, "string is unterminated");
      break;
    }
    case TOKEN_ERROR_UNTERMINATED_COMMENT: {
      report = report_new(REPORT_KIND_ERROR, parser->offset, "comment is unterminated");
      break;
    }
    default:
      unreachable();
    }
  } else {
    report = report_new(REPORT_KIND_ERROR, parser->offset, "expected %s, found `%s`", expected, token(parser)->text);
  }
  report_annotation(report, parser->offset, parser->offset + token(parser)->text_length, "expected %s", expected);
  array_push(parser->errors, &report);

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

static void expect_semi(Parser *parser, const SyntaxKind *next, unsigned long count)
{
  unsigned long offset = parser->offset;
  if (!eat(parser, SYNTAX_SEMI_TOKEN) && parser->alive) {
    if (check_any(parser, next, count)) {
      Report *report = report_new(REPORT_KIND_ERROR, offset, "semicolon is missing");
      report_annotation(report, offset, offset + 1, "insert `;` here");
      array_push(parser->errors, &report);
      parser->alive = 0;
      bump(parser);
    } else {
      error_unexpected(parser);
    }
  }
}

static const SyntaxKind FIRST_STANDARD_TYPE[] = { SYNTAX_INTEGER_KW, SYNTAX_BOOLEAN_KW, SYNTAX_CHAR_KW };

static void parse_std_type(Parser *parser)
{
  expect_any(parser, FIRST_STANDARD_TYPE, sizeof(FIRST_STANDARD_TYPE) / sizeof(SyntaxKind));
}

static void parse_array_type(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_ARRAY_KW);
  expect(parser, SYNTAX_LBRACKET_TOKEN);
  expect(parser, SYNTAX_NUMBER_LIT);
  expect(parser, SYNTAX_RBRACKET_TOKEN);
  expect(parser, SYNTAX_OF_KW);
  parse_std_type(parser);
  node_finish(parser, SYNTAX_ARRAY_TYPE);
}

static void parse_type(Parser *parser)
{
  if (check_any(parser, FIRST_STANDARD_TYPE, sizeof(FIRST_STANDARD_TYPE) / sizeof(SyntaxKind))) {
    parse_std_type(parser);
  } else {
    parse_array_type(parser);
  }
}

static void parse_factor(Parser *parser);
static void parse_expr(Parser *parser);

static void parse_var(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_IDENT_TOKEN);
  if (eat(parser, SYNTAX_LBRACKET_TOKEN)) {
    parse_expr(parser);
    expect(parser, SYNTAX_RBRACKET_TOKEN);
    node_finish(parser, SYNTAX_INDEXED_VAR);
  } else {
    node_finish(parser, SYNTAX_ENTIRE_VAR);
  }
}

static void parse_paren_expr(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  parse_expr(parser);
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYNTAX_PAREN_EXPR);
}

static void parse_not_expr(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_NOT_KW);
  parse_factor(parser);
  node_finish(parser, SYNTAX_NOT_EXPR);
}

static void parse_cast_expr(Parser *parser)
{
  node_start(parser);
  parse_std_type(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  parse_expr(parser);
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYNTAX_CAST_EXPR);
}

static const SyntaxKind FIRST_CONSTANT[] = { SYNTAX_NUMBER_LIT, SYNTAX_TRUE_KW, SYNTAX_FALSE_KW, SYNTAX_STRING_LIT };

static void parse_factor(Parser *parser)
{
  if (check(parser, SYNTAX_IDENT_TOKEN)) {
    parse_var(parser);
  } else if (check(parser, SYNTAX_LPAREN_TOKEN)) {
    parse_paren_expr(parser);
  } else if (check(parser, SYNTAX_NOT_KW)) {
    parse_not_expr(parser);
  } else if (check_any(parser, FIRST_STANDARD_TYPE, sizeof(FIRST_STANDARD_TYPE) / sizeof(SyntaxKind))) {
    parse_cast_expr(parser);
  } else {
    expect_any(parser, FIRST_CONSTANT, sizeof(FIRST_CONSTANT) / sizeof(SyntaxKind));
  }
}

static const SyntaxKind FIRST_MULTIPLICATIVE_OPERATOR[] = { SYNTAX_STAR_TOKEN, SYNTAX_DIV_KW, SYNTAX_AND_KW };

static void parse_term(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_factor(parser);
  while (eat_any(parser, FIRST_MULTIPLICATIVE_OPERATOR, sizeof(FIRST_MULTIPLICATIVE_OPERATOR) / sizeof(SyntaxKind))) {
    node_start_at(parser, checkpoint);
    parse_factor(parser);
    node_finish(parser, SYNTAX_BINARY_EXPR);
  }
}

static const SyntaxKind FIRST_ADDITIVE_OPERATOR[] = { SYNTAX_PLUS_TOKEN, SYNTAX_MINUS_TOKEN, SYNTAX_OR_KW };

static void parse_simple_expr(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  if (check_any(parser, FIRST_ADDITIVE_OPERATOR, sizeof(FIRST_ADDITIVE_OPERATOR) / sizeof(SyntaxKind))) {
    node_null(parser);
  } else {
    parse_term(parser);
  }
  while (eat_any(parser, FIRST_ADDITIVE_OPERATOR, sizeof(FIRST_ADDITIVE_OPERATOR) / sizeof(SyntaxKind))) {
    node_start_at(parser, checkpoint);
    parse_term(parser);
    node_finish(parser, SYNTAX_BINARY_EXPR);
  }
}

static const SyntaxKind FIRST_RELATIONAL_OPERATOR[] = {
  SYNTAX_EQUAL_TOKEN,
  SYNTAX_NOTEQ_TOKEN,
  SYNTAX_LESS_TOKEN,
  SYNTAX_LESSEQ_TOKEN,
  SYNTAX_GREATER_TOKEN,
  SYNTAX_GREATEREQ_TOKEN,
};

static void parse_expr(Parser *parser)
{
  unsigned long checkpoint = node_checkpoint(parser);
  parse_simple_expr(parser);
  while (eat_any(parser, FIRST_RELATIONAL_OPERATOR, sizeof(FIRST_RELATIONAL_OPERATOR) / sizeof(SyntaxKind))) {
    node_start_at(parser, checkpoint);
    parse_simple_expr(parser);
    node_finish(parser, SYNTAX_BINARY_EXPR);
  }
}

static void parse_stmt(Parser *parser);

static void parse_assign_stmt(Parser *parser)
{
  node_start(parser);
  parse_var(parser);
  expect(parser, SYNTAX_ASSIGN_TOKEN);
  parse_expr(parser);
  node_finish(parser, SYNTAX_ASSIGN_STMT);
}

static void parse_if_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_IF_KW);
  parse_expr(parser);
  expect(parser, SYNTAX_THEN_KW);
  parse_stmt(parser);
  if (eat(parser, SYNTAX_ELSE_KW)) {
    parse_stmt(parser);
  } else {
    node_null(parser);
    node_null(parser);
  }
  node_finish(parser, SYNTAX_IF_STMT);
}

static void parse_while_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_WHILE_KW);
  parse_expr(parser);
  expect(parser, SYNTAX_DO_KW);
  parse_stmt(parser);
  node_finish(parser, SYNTAX_WHILE_STMT);
}

static void parse_break_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_BREAK_KW);
  node_finish(parser, SYNTAX_RETURN_STMT);
}

static void parse_actual_param_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  do {
    parse_expr(parser);
  } while (eat(parser, SYNTAX_COMMA_TOKEN));
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYNTAX_ACTUAL_PARAM_LIST);
}

static void parse_call_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_CALL_KW);
  expect(parser, SYNTAX_IDENT_TOKEN);
  if (check(parser, SYNTAX_LPAREN_TOKEN)) {
    parse_actual_param_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_CALL_STMT);
}

static void parse_return_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_RETURN_KW);
  node_finish(parser, SYNTAX_RETURN_STMT);
}

static void parse_input_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  do {
    parse_var(parser);
  } while (eat(parser, SYNTAX_COMMA_TOKEN));
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYTANX_INPUT_LIST);
}

static const SyntaxKind FIRST_INPUT_STMT[] = { SYNTAX_READ_KW, SYNTAX_READLN_KW };

static void parse_input_stmt(Parser *parser)
{
  node_start(parser);
  expect_any(parser, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(SyntaxKind));
  if (check(parser, SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_INPUT_STMT);
}

static void parse_output_value(Parser *parser)
{
  node_start(parser);
  parse_expr(parser);
  if (eat(parser, SYNTAX_COLON_TOKEN)) {
    expect(parser, SYNTAX_NUMBER_LIT);
  } else {
    node_null(parser);
    node_null(parser);
  }
  node_finish(parser, SYNTAX_OUTPUT_VALUE);
}

static void parse_output_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  do {
    parse_output_value(parser);
  } while (eat(parser, SYNTAX_COMMA_TOKEN));
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYNTAX_OUTPUT_LIST);
}

static const SyntaxKind FIRST_OUTPUT_STMT[] = { SYNTAX_WRITE_KW, SYNTAX_WRITELN_KW };

static void parse_output_stmt(Parser *parser)
{
  node_start(parser);
  expect_any(parser, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(SyntaxKind));
  if (check(parser, SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(parser);
  } else {
    node_null(parser);
  }
  node_finish(parser, SYNTAX_OUTPUT_STMT);
}

static void parse_compound_stmt(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_BEGIN_KW);
  do {
    parse_stmt(parser);
  } while (eat(parser, SYNTAX_SEMI_TOKEN));
  expect(parser, SYNTAX_END_KW);
  node_finish(parser, SYNTAX_COMPOUND_STMT);
}

static void parse_stmt(Parser *parser)
{
  if (check(parser, SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(parser);
  } else if (check(parser, SYNTAX_IF_KW)) {
    parse_if_stmt(parser);
  } else if (check(parser, SYNTAX_WHILE_KW)) {
    parse_while_stmt(parser);
  } else if (check(parser, SYNTAX_BREAK_KW)) {
    parse_break_stmt(parser);
  } else if (check(parser, SYNTAX_CALL_KW)) {
    parse_call_stmt(parser);
  } else if (check(parser, SYNTAX_RETURN_KW)) {
    parse_return_stmt(parser);
  } else if (check_any(parser, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(SyntaxKind))) {
    parse_input_stmt(parser);
  } else if (check_any(parser, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(SyntaxKind))) {
    parse_output_stmt(parser);
  } else if (check(parser, SYNTAX_BEGIN_KW)) {
    parse_compound_stmt(parser);
  } else {
    node_null(parser);
  }
}

static void parse_var_decl(Parser *parser, const SyntaxKind *next, unsigned long count)
{
  node_start(parser);
  do {
    expect(parser, SYNTAX_IDENT_TOKEN);
  } while (eat(parser, SYNTAX_COMMA_TOKEN));
  expect(parser, SYNTAX_COLON_TOKEN);
  parse_type(parser);
  node_finish(parser, SYNTAX_VAR_DECL);
}

static void parse_var_decl_part(Parser *parser, const SyntaxKind *next, unsigned long count)
{
  node_start(parser);
  expect(parser, SYNTAX_VAR_KW);
  do {
    parse_var_decl(parser, next, count);
    expect_semi(parser, next, count);
  } while (check(parser, SYNTAX_IDENT_TOKEN));
  node_finish(parser, SYNTAX_VAR_DECL_PART);
}

static void parse_formal_param_section(Parser *parser)
{
  node_start(parser);
  do {
    expect(parser, SYNTAX_IDENT_TOKEN);
  } while (eat(parser, SYNTAX_COMMA_TOKEN));
  expect(parser, SYNTAX_COLON_TOKEN);
  parse_type(parser);
  node_finish(parser, SYNTAX_FORMAL_PARAM_SECTION);
}

static void parse_formal_param_list(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_LPAREN_TOKEN);
  do {
    parse_formal_param_section(parser);
  } while (eat(parser, SYNTAX_SEMI_TOKEN));
  expect(parser, SYNTAX_RPAREN_TOKEN);
  node_finish(parser, SYNTAX_FORMAL_PARAM_LIST);
}

static void parse_proc_decl(Parser *parser, const SyntaxKind *next, unsigned long count)
{
  node_start(parser);
  expect(parser, SYNTAX_PROCEDURE_KW);
  expect(parser, SYNTAX_IDENT_TOKEN);
  if (check(parser, SYNTAX_LPAREN_TOKEN)) {
    parse_formal_param_list(parser);
  } else {
    node_null(parser);
  }
  {
    SyntaxKind next[] = { SYNTAX_VAR_KW, SYNTAX_BEGIN_KW };
    expect_semi(parser, next, sizeof(next) / sizeof(SyntaxKind));
  }
  if (check(parser, SYNTAX_VAR_KW)) {
    SyntaxKind next[] = { SYNTAX_BEGIN_KW };
    parse_var_decl_part(parser, next, sizeof(next) / sizeof(SyntaxKind));
  } else {
    node_null(parser);
  }
  parse_compound_stmt(parser);
  expect_semi(parser, next, count);
  node_finish(parser, SYNTAX_PROC_DECL);
}

static void parse_program(Parser *parser)
{
  node_start(parser);
  expect(parser, SYNTAX_PROGRAM_KW);
  expect(parser, SYNTAX_IDENT_TOKEN);
  {
    SyntaxKind next[] = { SYNTAX_VAR_KW, SYNTAX_PROCEDURE_KW, SYNTAX_BEGIN_KW };
    expect_semi(parser, next, sizeof(next) / sizeof(SyntaxKind));
  }
  while (1) {
    SyntaxKind next[] = { SYNTAX_VAR_KW, SYNTAX_PROCEDURE_KW, SYNTAX_BEGIN_KW };
    if (check(parser, SYNTAX_VAR_KW)) {
      parse_var_decl_part(parser, next, sizeof(next) / sizeof(SyntaxKind));
    } else if (check(parser, SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl(parser, next, sizeof(next) / sizeof(SyntaxKind));
    } else {
      break;
    }
  }
  parse_compound_stmt(parser);
  expect(parser, SYNTAX_DOT_TOKEN);
  expect(parser, SYNTAX_EOF_TOKEN);
  node_finish(parser, SYNTAX_PROGRAM);
}

int mppl_parse(const Source *source, TokenTree **tree)
{
  Parser parser;
  int    result;
  parser.parents  = array_new(sizeof(unsigned long));
  parser.children = array_new(sizeof(TokenNode *));
  parser.errors   = array_new(sizeof(Report *));
  parser.expected = bitset_new(SYNTAX_EOF_TOKEN + 1);
  token_cursor_init(&parser.cursor, source);
  parser.token  = NULL;
  parser.alive  = 1;
  parser.offset = 0;

  parse_program(&parser);
  *tree = *(TokenTree **) array_data(parser.children);

  {
    unsigned long i;
    for (i = 0; i < array_count(parser.errors); ++i) {
      Report **report = array_at(parser.errors, i);
      report_emit(*report, source);
    }
    fflush(stdout);
  }
  result = !array_count(parser.errors);

  bitset_free(parser.expected);
  array_free(parser.errors);
  array_free(parser.children);
  array_free(parser.parents);
  return result;
}
