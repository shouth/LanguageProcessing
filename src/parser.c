#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "compiler.h"
#include "context.h"
#include "context_fwd.h"
#include "mppl_syntax.h"
#include "report.h"
#include "source.h"
#include "string.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

typedef struct Parser Parser;

struct Parser {
  unsigned long  offset;
  const Source  *source;
  Ctx           *ctx;
  LexStatus      status;
  SyntaxBuilder *builder;
  const String  *token;
  SyntaxKind     token_kind;
  BITSET(expected, SYNTAX_EOF_TOKEN + 1);
  Array        *errors;
  int           alive;
  unsigned long breakable;
};

static const String *token(Parser *self)
{
  if (!self->token) {
    while (1) {
      LexedToken    lexed;
      LexStatus     status = mpplc_lex(self->source, self->offset, &lexed);
      const String *token  = ctx_string(self->ctx, self->source->text + lexed.offset, lexed.length);

      if (syntax_kind_is_token(lexed.kind)) {
        self->status     = status;
        self->token      = token;
        self->token_kind = lexed.kind;
        break;
      } else {
        syntax_builder_trivia(self->builder, lexed.kind, token, 1);
        self->offset += lexed.length;
      }
    }
  }
  return self->token;
}

static void bump(Parser *self)
{
  if (token(self)) {
    bitset_clear(self->expected);
    syntax_builder_token(self->builder, self->token_kind, self->token);
    self->offset += string_length(self->token);
    self->token = NULL;
  }
}

static int check_any(Parser *self, const SyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    return 0;
  } else {
    unsigned long i;
    for (i = 0; i < count; ++i) {
      bitset_set(self->expected, kinds[i]);
    }
    for (i = 0; i < count; ++i) {
      if (token(self) && self->token_kind == kinds[i]) {
        return 1;
      }
    }
    return 0;
  }
}

static int check(Parser *self, SyntaxKind kind)
{
  return check_any(self, &kind, 1);
}

static int eat_any(Parser *self, const SyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    return 0;
  } else if (check_any(self, kinds, count)) {
    bump(self);
    return 1;
  } else {
    return 0;
  }
}

static int eat(Parser *self, SyntaxKind kind)
{
  return eat_any(self, &kind, 1);
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

static void error_unexpected(Parser *self)
{
  char          expected[1024];
  unsigned long cursor = 0;
  SyntaxKind    kind;
  Report       *report;

  for (kind = 0; kind <= SYNTAX_EOF_TOKEN; ++kind) {
    if (bitset_get(self->expected, kind)) {
      if (cursor > 0) {
        if (bitset_count(self->expected) > 1) {
          cursor += sprintf(expected + cursor, ", ");
        } else {
          cursor += sprintf(expected + cursor, " and ");
        }
      }
      cursor += sprintf(expected + cursor, "%s", SYNTAX_DISPLAY_STRING[kind]);
      bitset_reset(self->expected, kind);
    }
  }

  if (token(self) && self->token_kind == SYNTAX_BAD_TOKEN) {
    switch (self->status) {
    case LEX_ERROR_STRAY_CHAR: {
      if (is_graphic(string_data(token(self))[0])) {
        report = report_new(REPORT_KIND_ERROR, self->offset, "stray `%s` in program", string_data(token(self)));
      } else {
        report = report_new(REPORT_KIND_ERROR, self->offset, "stray `\\x%X` in program", (unsigned char) string_data(token(self))[0]);
      }
      report_annotation(report, self->offset, self->offset + string_length(token(self)), "expected %s", expected);
      self->alive = 0;
      break;
    }
    case LEX_ERROR_NONGRAPHIC_CHAR: {
      report = report_new(REPORT_KIND_ERROR, self->offset, "non-graphic character in string");
      report_annotation(report, self->offset, self->offset + string_length(token(self)), NULL);
      break;
    }
    case LEX_ERROR_UNTERMINATED_STRING: {
      report = report_new(REPORT_KIND_ERROR, self->offset, "string is unterminated");
      report_annotation(report, self->offset, self->offset + string_length(token(self)), NULL);
      self->alive = 0;
      break;
    }
    case LEX_ERROR_UNTERMINATED_COMMENT: {
      report = report_new(REPORT_KIND_ERROR, self->offset, "comment is unterminated");
      report_annotation(report, self->offset, self->offset + string_length(token(self)), NULL);
      self->alive = 0;
      break;
    }
    case LEX_ERROR_TOO_BIG_NUMBER: {
      report = report_new(REPORT_KIND_ERROR, self->offset, "number is too big");
      report_annotation(report, self->offset, self->offset + string_length(token(self)), "value should be less than or equal to 32768");
      break;
    }
    default:
      unreachable();
    }
  } else {
    report = report_new(REPORT_KIND_ERROR, self->offset, "expected %s, found `%s`", expected, string_data(token(self)));
    report_annotation(report, self->offset, self->offset + string_length(token(self)), "expected %s", expected);
    self->alive = 0;
  }
  array_push(self->errors, &report);

  bump(self);
}

static int expect_any(Parser *self, const SyntaxKind *kinds, unsigned long count)
{
  if (!self->alive) {
    syntax_builder_null(self->builder);
    return 0;
  } else if (eat_any(self, kinds, count)) {
    return 1;
  } else {
    error_unexpected(self);
    return 0;
  }
}

static int expect(Parser *self, SyntaxKind kind)
{
  return expect_any(self, &kind, 1);
}

static void expect_semi(Parser *self)
{
  unsigned long offset = self->offset;
  if (!eat(self, SYNTAX_SEMI_TOKEN) && self->alive) {
    Report *report = report_new(REPORT_KIND_ERROR, offset, "semicolon is missing");
    report_annotation(report, offset, offset + 1, "insert `;` here");
    array_push(self->errors, &report);
    self->alive = 0;
    bump(self);
  }
}

static const SyntaxKind FIRST_STD_TYPE[] = { SYNTAX_INTEGER_KW, SYNTAX_BOOLEAN_KW, SYNTAX_CHAR_KW };

static void parse_std_type(Parser *self)
{
  expect_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(SyntaxKind));
}

static void parse_array_type(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_ARRAY_KW);
  expect(self, SYNTAX_LBRACKET_TOKEN);
  expect(self, SYNTAX_NUMBER_LIT);
  expect(self, SYNTAX_RBRACKET_TOKEN);
  expect(self, SYNTAX_OF_KW);
  parse_std_type(self);
  syntax_builder_end_tree(self->builder, SYNTAX_ARRAY_TYPE);
}

static void parse_type(Parser *self)
{
  if (check_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(SyntaxKind))) {
    parse_std_type(self);
  } else {
    parse_array_type(self);
  }
}

static void parse_factor(Parser *self);
static void parse_expr(Parser *self);

static void parse_var(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_IDENT_TOKEN);
  if (eat(self, SYNTAX_LBRACKET_TOKEN)) {
    parse_expr(self);
    expect(self, SYNTAX_RBRACKET_TOKEN);
    syntax_builder_end_tree(self->builder, SYNTAX_INDEXED_VAR);
  } else {
    syntax_builder_end_tree(self->builder, SYNTAX_ENTIRE_VAR);
  }
}

static void parse_paren_expr(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_LPAREN_TOKEN);
  parse_expr(self);
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_PAREN_EXPR);
}

static void parse_not_expr(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_NOT_KW);
  parse_factor(self);
  syntax_builder_end_tree(self->builder, SYNTAX_NOT_EXPR);
}

static void parse_cast_expr(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  parse_std_type(self);
  expect(self, SYNTAX_LPAREN_TOKEN);
  parse_expr(self);
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_CAST_EXPR);
}

static const SyntaxKind FIRST_CONST[] = { SYNTAX_NUMBER_LIT, SYNTAX_TRUE_KW, SYNTAX_FALSE_KW, SYNTAX_STRING_LIT };

static void parse_factor(Parser *self)
{
  if (check(self, SYNTAX_IDENT_TOKEN)) {
    parse_var(self);
  } else if (check(self, SYNTAX_LPAREN_TOKEN)) {
    parse_paren_expr(self);
  } else if (check(self, SYNTAX_NOT_KW)) {
    parse_not_expr(self);
  } else if (check_any(self, FIRST_STD_TYPE, sizeof(FIRST_STD_TYPE) / sizeof(SyntaxKind))) {
    parse_cast_expr(self);
  } else {
    expect_any(self, FIRST_CONST, sizeof(FIRST_CONST) / sizeof(SyntaxKind));
  }
}

static const SyntaxKind FIRST_MULTI_OP[] = { SYNTAX_STAR_TOKEN, SYNTAX_DIV_KW, SYNTAX_AND_KW };

static void parse_term(Parser *self)
{
  unsigned long checkpoint = syntax_builder_checkpoint(self->builder);
  parse_factor(self);
  while (eat_any(self, FIRST_MULTI_OP, sizeof(FIRST_MULTI_OP) / sizeof(SyntaxKind))) {
    syntax_builder_start_tree_at(self->builder, checkpoint);
    parse_factor(self);
    syntax_builder_end_tree(self->builder, SYNTAX_BINARY_EXPR);
  }
}

static const SyntaxKind FIRST_ADD_OP[] = { SYNTAX_PLUS_TOKEN, SYNTAX_MINUS_TOKEN, SYNTAX_OR_KW };

static void parse_simple_expr(Parser *self)
{
  unsigned long checkpoint = syntax_builder_checkpoint(self->builder);
  if (check_any(self, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(SyntaxKind))) {
    syntax_builder_null(self->builder);
  } else {
    parse_term(self);
  }
  while (eat_any(self, FIRST_ADD_OP, sizeof(FIRST_ADD_OP) / sizeof(SyntaxKind))) {
    syntax_builder_start_tree_at(self->builder, checkpoint);
    parse_term(self);
    syntax_builder_end_tree(self->builder, SYNTAX_BINARY_EXPR);
  }
}

static const SyntaxKind FIRST_RELAT_OP[] = {
  SYNTAX_EQUAL_TOKEN,
  SYNTAX_NOTEQ_TOKEN,
  SYNTAX_LESS_TOKEN,
  SYNTAX_LESSEQ_TOKEN,
  SYNTAX_GREATER_TOKEN,
  SYNTAX_GREATEREQ_TOKEN,
};

static void parse_expr(Parser *self)
{
  unsigned long checkpoint = syntax_builder_checkpoint(self->builder);
  parse_simple_expr(self);
  while (eat_any(self, FIRST_RELAT_OP, sizeof(FIRST_RELAT_OP) / sizeof(SyntaxKind))) {
    syntax_builder_start_tree_at(self->builder, checkpoint);
    parse_simple_expr(self);
    syntax_builder_end_tree(self->builder, SYNTAX_BINARY_EXPR);
  }
}

static void parse_stmt(Parser *self);

static void parse_assign_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  parse_var(self);
  expect(self, SYNTAX_ASSIGN_TOKEN);
  parse_expr(self);
  syntax_builder_end_tree(self->builder, SYNTAX_ASSIGN_STMT);
}

static void parse_if_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_IF_KW);
  parse_expr(self);
  expect(self, SYNTAX_THEN_KW);
  parse_stmt(self);
  if (eat(self, SYNTAX_ELSE_KW)) {
    parse_stmt(self);
  } else {
    syntax_builder_null(self->builder);
    syntax_builder_null(self->builder);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_IF_STMT);
}

static void parse_while_stmt(Parser *self)
{
  ++self->breakable;
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_WHILE_KW);
  parse_expr(self);
  expect(self, SYNTAX_DO_KW);
  parse_stmt(self);
  syntax_builder_end_tree(self->builder, SYNTAX_WHILE_STMT);
  --self->breakable;
}

static void parse_break_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  if (check(self, SYNTAX_BREAK_KW)) {
    if (!self->breakable && self->alive) {
      Report *report = report_new(REPORT_KIND_ERROR, self->offset, "`break` is outside of a loop");
      report_annotation(report, self->offset, self->offset + string_length(token(self)),
        "`break` should be enclosed by a loop");
      array_push(self->errors, &report);
    }
    bump(self);
  } else {
    error_unexpected(self);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_BREAK_STMT);
}

static void parse_act_param_list(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_LPAREN_TOKEN);
  do {
    parse_expr(self);
  } while (eat(self, SYNTAX_COMMA_TOKEN));
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_ACT_PARAM_LIST);
}

static void parse_call_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_CALL_KW);
  expect(self, SYNTAX_IDENT_TOKEN);
  if (check(self, SYNTAX_LPAREN_TOKEN)) {
    parse_act_param_list(self);
  } else {
    syntax_builder_null(self->builder);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_CALL_STMT);
}

static void parse_return_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_RETURN_KW);
  syntax_builder_end_tree(self->builder, SYNTAX_RETURN_STMT);
}

static void parse_input_list(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_LPAREN_TOKEN);
  do {
    parse_var(self);
  } while (eat(self, SYNTAX_COMMA_TOKEN));
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYTANX_INPUT_LIST);
}

static const SyntaxKind FIRST_INPUT_STMT[] = { SYNTAX_READ_KW, SYNTAX_READLN_KW };

static void parse_input_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect_any(self, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(SyntaxKind));
  if (check(self, SYNTAX_LPAREN_TOKEN)) {
    parse_input_list(self);
  } else {
    syntax_builder_null(self->builder);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_INPUT_STMT);
}

static void parse_output_value(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  parse_expr(self);
  if (eat(self, SYNTAX_COLON_TOKEN)) {
    expect(self, SYNTAX_NUMBER_LIT);
  } else {
    syntax_builder_null(self->builder);
    syntax_builder_null(self->builder);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_OUTPUT_VALUE);
}

static void parse_output_list(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_LPAREN_TOKEN);
  do {
    parse_output_value(self);
  } while (eat(self, SYNTAX_COMMA_TOKEN));
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_OUTPUT_LIST);
}

static const SyntaxKind FIRST_OUTPUT_STMT[] = { SYNTAX_WRITE_KW, SYNTAX_WRITELN_KW };

static void parse_output_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect_any(self, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(SyntaxKind));
  if (check(self, SYNTAX_LPAREN_TOKEN)) {
    parse_output_list(self);
  } else {
    syntax_builder_null(self->builder);
  }
  syntax_builder_end_tree(self->builder, SYNTAX_OUTPUT_STMT);
}

static void parse_comp_stmt(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_BEGIN_KW);
  do {
    parse_stmt(self);
  } while (eat(self, SYNTAX_SEMI_TOKEN));
  expect(self, SYNTAX_END_KW);
  syntax_builder_end_tree(self->builder, SYNTAX_COMP_STMT);
}

static void parse_stmt(Parser *self)
{
  if (check(self, SYNTAX_IDENT_TOKEN)) {
    parse_assign_stmt(self);
  } else if (check(self, SYNTAX_IF_KW)) {
    parse_if_stmt(self);
  } else if (check(self, SYNTAX_WHILE_KW)) {
    parse_while_stmt(self);
  } else if (check(self, SYNTAX_BREAK_KW)) {
    parse_break_stmt(self);
  } else if (check(self, SYNTAX_CALL_KW)) {
    parse_call_stmt(self);
  } else if (check(self, SYNTAX_RETURN_KW)) {
    parse_return_stmt(self);
  } else if (check_any(self, FIRST_INPUT_STMT, sizeof(FIRST_INPUT_STMT) / sizeof(SyntaxKind))) {
    parse_input_stmt(self);
  } else if (check_any(self, FIRST_OUTPUT_STMT, sizeof(FIRST_OUTPUT_STMT) / sizeof(SyntaxKind))) {
    parse_output_stmt(self);
  } else if (check(self, SYNTAX_BEGIN_KW)) {
    parse_comp_stmt(self);
  } else {
    syntax_builder_null(self->builder);
  }
}

static void parse_var_decl(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  do {
    expect(self, SYNTAX_IDENT_TOKEN);
  } while (eat(self, SYNTAX_COMMA_TOKEN));
  expect(self, SYNTAX_COLON_TOKEN);
  parse_type(self);
  syntax_builder_end_tree(self->builder, SYNTAX_VAR_DECL);
}

static void parse_var_decl_part(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_VAR_KW);
  do {
    parse_var_decl(self);
    expect_semi(self);
  } while (check(self, SYNTAX_IDENT_TOKEN));
  syntax_builder_end_tree(self->builder, SYNTAX_VAR_DECL_PART);
}

static void parse_fml_param_sec(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  do {
    expect(self, SYNTAX_IDENT_TOKEN);
  } while (eat(self, SYNTAX_COMMA_TOKEN));
  expect(self, SYNTAX_COLON_TOKEN);
  parse_type(self);
  syntax_builder_end_tree(self->builder, SYNTAX_FML_PARAM_SEC);
}

static void parse_fml_param_list(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_LPAREN_TOKEN);
  do {
    parse_fml_param_sec(self);
  } while (eat(self, SYNTAX_SEMI_TOKEN));
  expect(self, SYNTAX_RPAREN_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_FML_PARAM_LIST);
}

static void parse_proc_decl(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_PROCEDURE_KW);
  expect(self, SYNTAX_IDENT_TOKEN);
  if (check(self, SYNTAX_LPAREN_TOKEN)) {
    parse_fml_param_list(self);
  } else {
    syntax_builder_null(self->builder);
  }
  expect_semi(self);
  if (check(self, SYNTAX_VAR_KW)) {
    parse_var_decl_part(self);
  } else {
    syntax_builder_null(self->builder);
  }
  parse_comp_stmt(self);
  expect_semi(self);
  syntax_builder_end_tree(self->builder, SYNTAX_PROC_DECL);
}

static void parse_program(Parser *self)
{
  syntax_builder_start_tree(self->builder);
  expect(self, SYNTAX_PROGRAM_KW);
  expect(self, SYNTAX_IDENT_TOKEN);
  expect_semi(self);

  while (1) {
    if (check(self, SYNTAX_VAR_KW)) {
      parse_var_decl_part(self);
    } else if (check(self, SYNTAX_PROCEDURE_KW)) {
      parse_proc_decl(self);
    } else {
      break;
    }
  }
  parse_comp_stmt(self);
  expect(self, SYNTAX_DOT_TOKEN);
  expect(self, SYNTAX_EOF_TOKEN);
  syntax_builder_end_tree(self->builder, SYNTAX_PROGRAM);
}

int mpplc_parse(const Source *source, Ctx *ctx, MpplProgram **syntax)
{
  Parser self;
  int    result;
  self.offset     = 0;
  self.source     = source;
  self.ctx        = ctx;
  self.status     = LEX_OK;
  self.builder    = syntax_builder_new();
  self.token      = NULL;
  self.token_kind = SYNTAX_BAD_TOKEN;
  bitset_clear(self.expected);
  self.errors    = array_new(sizeof(Report *));
  self.alive     = 1;
  self.breakable = 0;

  parse_program(&self);
  *syntax = (MpplProgram *) syntax_builder_build(self.builder);
  {
    unsigned long i;
    for (i = 0; i < array_count(self.errors); ++i) {
      report_emit(*(Report **) array_at(self.errors, i), source);
    }
    fflush(stdout);
  }
  result = !array_count(self.errors);

  if (!result) {
    mppl_unref(*syntax);
    *syntax = NULL;
  }

  array_free(self.errors);
  return result;
}
