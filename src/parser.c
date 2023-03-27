#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "message.h"
#include "source.h"
#include "utility.h"

typedef struct {
  const source_t   *src;
  cursol_t          cursol;
  symbol_storage_t *storage;

  token_t  current_token, next_token;
  uint64_t expected_tokens;
  int      within_loop;
  int      alive, error;
} parser_t;

static size_t msg_token(char *ptr, token_kind_t type)
{
  switch (type) {
  case TOKEN_NAME:
    return sprintf(ptr, "identifier");
  case TOKEN_NUMBER:
    return sprintf(ptr, "number");
  case TOKEN_STRING:
    return sprintf(ptr, "string");
  default:
    return sprintf(ptr, "`%s`", token_to_str(type));
  }
}

static void error_msg(parser_t *parser, msg_t *msg)
{
  parser->alive = 0;
  parser->error = 1;
  msg_emit(msg);
}

static void error_expected(parser_t *parser, const char *str)
{
  msg_t *msg;
  assert(parser);

  if (!parser->alive) {
    return;
  }

  msg = new_msg(parser->src, parser->next_token.region,
    MSG_ERROR, "expected %s, got `%.*s`", str, (int) parser->next_token.region.len, parser->next_token.ptr);
  error_msg(parser, msg);
  parser->expected_tokens = 0;
}

static void error_unexpected(parser_t *parser)
{
  char    buf[256], *ptr;
  uint8_t l, t;

  assert(parser);
  if (!parser->alive) {
    return;
  }
  assert(parser->expected_tokens);

  l   = leading0(parser->expected_tokens);
  ptr = buf;
  while (parser->expected_tokens) {
    t = trailing0(parser->expected_tokens);
    if (ptr != buf) {
      ptr += sprintf(ptr, t != l ? ", " : " or ");
    }
    ptr += msg_token(ptr, t);
    parser->expected_tokens ^= (uint64_t) 1 << t;
  }
  error_expected(parser, buf);
}

static void bump(parser_t *parser)
{
  if (!parser || !parser->alive) {
    return;
  }
  parser->current_token   = parser->next_token;
  parser->expected_tokens = 0;
  while (1) {
    lex_token(&parser->cursol, &parser->next_token);
    switch (parser->next_token.type) {
    case TOKEN_WHITESPACE:
    case TOKEN_BRACES_COMMENT:
    case TOKEN_CSTYLE_COMMENT:
      continue;
    case TOKEN_UNKNOWN:
    case TOKEN_ERROR:
      exit(1);
    default:
      return;
    }
  }
}

static int check(parser_t *parser, token_kind_t type)
{
  assert(parser);
  if (!parser->alive) {
    return 0;
  }
  parser->expected_tokens |= (uint64_t) 1 << type;
  return parser->next_token.type == type;
}

static int eat(parser_t *parser, token_kind_t type)
{
  int ret = check(parser, type);
  if (ret) {
    bump(parser);
  }
  return ret;
}

static int expect(parser_t *parser, token_kind_t type)
{
  int ret = eat(parser, type);
  if (!ret) {
    error_unexpected(parser);
  }
  return ret;
}

static ast_ident_t *parse_ident(parser_t *parser)
{
  if (expect(parser, TOKEN_NAME)) {
    ast_ident_t *ident = xmalloc(sizeof(ast_ident_t));
    ident->symbol      = symbol_intern(parser->storage, parser->current_token.ptr, parser->current_token.region.len);
    ident->region      = parser->current_token.region;
    ident->next        = NULL;
    return ident;
  } else {
    return NULL;
  }
}

static ast_ident_t *parse_ident_seq(parser_t *parser)
{
  ast_ident_t *seq, **tail = &seq;
  do {
    ast_ident_t *ident = parse_ident(parser);
    *tail              = ident;
    tail               = &ident->next;
  } while (eat(parser, TOKEN_COMMA));
  return seq;
}

static ast_lit_t *init_ast_lit(ast_lit_t *lit, ast_lit_kind_t kind, region_t region)
{
  lit->kind   = kind;
  lit->region = region;
  return lit;
}

static ast_lit_t *parse_number_lit(parser_t *parser)
{
  if (expect(parser, TOKEN_NUMBER)) {
    ast_lit_number_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->symbol           = symbol_intern(parser->storage, parser->current_token.ptr, parser->current_token.region.len);
    lit->value            = parser->current_token.data.number.value;
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_NUMBER, parser->current_token.region);
  } else {
    return NULL;
  }
}

static ast_lit_t *parse_boolean_lit(parser_t *parser)
{
  if (eat(parser, TOKEN_TRUE) || eat(parser, TOKEN_FALSE)) {
    ast_lit_boolean_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->value             = parser->current_token.type == TOKEN_TRUE;
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_BOOLEAN, parser->current_token.region);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_lit_t *parse_string_lit(parser_t *parser)
{
  if (expect(parser, TOKEN_STRING)) {
    ast_lit_string_t   *lit  = xmalloc(sizeof(ast_lit_t));
    const token_data_t *data = &parser->current_token.data;
    lit->str_len             = data->string.str_len;
    lit->symbol              = symbol_intern(parser->storage, data->string.ptr, data->string.len);
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_STRING, parser->current_token.region);
  } else {
    return NULL;
  }
}

static int check_lit(parser_t *parser)
{
  return check(parser, TOKEN_NUMBER) || check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE) || check(parser, TOKEN_STRING);
}

static ast_lit_t *parse_lit(parser_t *parser)
{
  if (check(parser, TOKEN_NUMBER)) {
    return parse_number_lit(parser);
  } else if (check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE)) {
    return parse_boolean_lit(parser);
  } else if (check(parser, TOKEN_STRING)) {
    return parse_string_lit(parser);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static int check_std_type(parser_t *parser)
{
  return check(parser, TOKEN_INTEGER) || check(parser, TOKEN_BOOLEAN) || check(parser, TOKEN_CHAR);
}

static ast_type_t *init_ast_type(ast_type_t *type, ast_type_kind_t kind, region_t region)
{
  type->kind   = kind;
  type->region = region;
  return type;
}

static ast_type_t *parse_std_type(parser_t *parser)
{
  if (eat(parser, TOKEN_INTEGER)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_INTEGER, parser->current_token.region);
  } else if (eat(parser, TOKEN_BOOLEAN)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_BOOLEAN, parser->current_token.region);
  } else if (eat(parser, TOKEN_CHAR)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_CHAR, parser->current_token.region);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_type_t *parse_array_type(parser_t *parser)
{
  region_t          left = parser->next_token.region;
  ast_type_array_t *type = xmalloc(sizeof(ast_type_t));

  expect(parser, TOKEN_ARRAY);
  expect(parser, TOKEN_LSQPAREN);
  type->size = parse_number_lit(parser);
  expect(parser, TOKEN_RSQPAREN);
  expect(parser, TOKEN_OF);
  type->base = parse_std_type(parser);

  return init_ast_type((ast_type_t *) type, AST_TYPE_KIND_ARRAY, region_unite(left, parser->current_token.region));
}

static ast_type_t *parse_type(parser_t *parser)
{
  if (check(parser, TOKEN_ARRAY)) {
    return parse_array_type(parser);
  } else if (check_std_type(parser)) {
    return parse_std_type(parser);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_expr_t *init_ast_expr(ast_expr_t *expr, ast_expr_kind_t kind, region_t region)
{
  expr->kind   = kind;
  expr->region = region;
  expr->next   = NULL;
  return expr;
}

static ast_expr_t *parse_expr(parser_t *parser);

static ast_expr_t *parse_ref(parser_t *parser)
{
  region_t     left  = parser->next_token.region;
  ast_ident_t *ident = parse_ident(parser);

  if (eat(parser, TOKEN_LSQPAREN)) {
    ast_expr_array_subscript_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->subscript                  = parse_expr(parser);
    expr->decl                       = ident;
    expect(parser, TOKEN_RSQPAREN);
    return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_ARRAY_SUBSCRIPT, region_unite(left, parser->current_token.region));
  } else {
    ast_expr_decl_ref_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->decl                = ident;
    return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_DECL_REF, region_unite(left, parser->current_token.region));
  }
}

static ast_expr_t *parse_ref_seq(parser_t *parser)
{
  ast_expr_t *seq, **tail = &seq;
  do {
    ast_expr_t *expr = parse_ref(parser);
    *tail            = expr;
    tail             = &expr->next;
  } while (eat(parser, TOKEN_COMMA));
  return seq;
}

static ast_expr_t *parse_expr_seq(parser_t *parser)
{
  ast_expr_t *seq, **tail = &seq;
  do {
    ast_expr_t *expr = parse_expr(parser);
    *tail            = expr;
    tail             = &expr->next;
  } while (eat(parser, TOKEN_COMMA));
  return seq;
}

static ast_expr_t *parse_factor(parser_t *parser);

static ast_expr_t *parse_expr_consant(parser_t *parser)
{
  region_t             left = parser->next_token.region;
  ast_expr_constant_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->lit                 = parse_lit(parser);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_CONSTANT, region_unite(left, parser->current_token.region));
}

static ast_expr_t *parse_expr_paren(parser_t *parser)
{
  region_t          left = parser->next_token.region;
  ast_expr_paren_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(parser, TOKEN_LPAREN);
  expr->inner = parse_expr(parser);
  expect(parser, TOKEN_RPAREN);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_PAREN, region_unite(left, parser->current_token.region));
}

static ast_expr_t *parse_expr_unary(parser_t *parser)
{
  region_t          left = parser->next_token.region;
  ast_expr_unary_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(parser, TOKEN_NOT);
  expr->kind      = AST_EXPR_UNARY_KIND_NOT;
  expr->op_region = parser->current_token.region;
  expr->expr      = parse_factor(parser);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_UNARY, region_unite(left, parser->current_token.region));
}

static ast_expr_t *parse_expr_cast(parser_t *parser)
{
  region_t         left = parser->next_token.region;
  ast_expr_cast_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->type            = parse_std_type(parser);
  expect(parser, TOKEN_LPAREN);
  expr->cast = parse_expr(parser);
  expect(parser, TOKEN_RPAREN);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_CAST, region_unite(left, parser->current_token.region));
}

static ast_expr_t *parse_factor(parser_t *parser)
{
  if (check(parser, TOKEN_NAME)) {
    return parse_ref(parser);
  } else if (check_lit(parser)) {
    return parse_expr_consant(parser);
  } else if (check(parser, TOKEN_LPAREN)) {
    return parse_expr_paren(parser);
  } else if (check(parser, TOKEN_NOT)) {
    return parse_expr_unary(parser);
  } else if (check_std_type(parser)) {
    return parse_expr_cast(parser);
  } else {
    error_expected(parser, "expression");
    return NULL;
  }
}

static ast_expr_t *parse_term(parser_t *parser)
{
  ast_expr_t *term = parse_factor(parser);
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(parser, TOKEN_STAR)) {
      kind = AST_EXPR_BINARY_KIND_STAR;
    } else if (eat(parser, TOKEN_DIV)) {
      kind = AST_EXPR_BINARY_KIND_DIV;
    } else if (eat(parser, TOKEN_AND)) {
      kind = AST_EXPR_BINARY_KIND_AND;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = parser->current_token.region;
      binary->lhs               = term;
      binary->rhs               = parse_factor(parser);

      term = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return term;
}

static ast_expr_t *parse_simple_expr(parser_t *parser)
{
  ast_expr_t *simple;
  if (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
    simple = init_ast_expr(xmalloc(sizeof(ast_expr_t)), AST_EXPR_KIND_EMPTY, region_from(parser->next_token.region.pos, 0));
  } else {
    simple = parse_term(parser);
  }
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(parser, TOKEN_PLUS)) {
      kind = AST_EXPR_BINARY_KIND_PLUS;
    } else if (eat(parser, TOKEN_MINUS)) {
      kind = AST_EXPR_BINARY_KIND_MINUS;
    } else if (eat(parser, TOKEN_OR)) {
      kind = AST_EXPR_BINARY_KIND_OR;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = parser->current_token.region;
      binary->lhs               = simple;
      binary->rhs               = parse_term(parser);

      simple = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return simple;
}

static ast_expr_t *parse_expr(parser_t *parser)
{
  ast_expr_t *expr = parse_simple_expr(parser);
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(parser, TOKEN_EQUAL)) {
      kind = AST_EXPR_BINARY_KIND_EQUAL;
    } else if (eat(parser, TOKEN_NOTEQ)) {
      kind = AST_EXPR_BINARY_KIND_NOTEQ;
    } else if (eat(parser, TOKEN_LE)) {
      kind = AST_EXPR_BINARY_KIND_LE;
    } else if (eat(parser, TOKEN_LEEQ)) {
      kind = AST_EXPR_BINARY_KIND_LEEQ;
    } else if (eat(parser, TOKEN_GR)) {
      kind = AST_EXPR_BINARY_KIND_GR;
    } else if (eat(parser, TOKEN_GREQ)) {
      kind = AST_EXPR_BINARY_KIND_GREQ;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = parser->current_token.region;
      binary->lhs               = expr;
      binary->rhs               = parse_simple_expr(parser);

      expr = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return expr;
}

static ast_stmt_t *parse_stmt(parser_t *parser);

static ast_stmt_t *init_ast_stmt(ast_stmt_t *stmt, ast_stmt_kind_t kind)
{
  stmt->kind = kind;
  stmt->next = NULL;
  return stmt;
}

static ast_stmt_t *parse_stmt_assign(parser_t *parser)
{
  ast_stmt_assign_t *stmt = xmalloc(sizeof(ast_stmt_t));

  stmt->lhs = parse_ref(parser);
  expect(parser, TOKEN_ASSIGN);
  stmt->op_region = parser->current_token.region;
  stmt->rhs       = parse_expr(parser);
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_ASSIGN);
}

static ast_stmt_t *parse_stmt_if(parser_t *parser)
{
  ast_stmt_if_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(parser, TOKEN_IF);
  stmt->cond = parse_expr(parser);
  expect(parser, TOKEN_THEN);
  stmt->then_stmt = parse_stmt(parser);
  if (eat(parser, TOKEN_ELSE)) {
    stmt->else_stmt = parse_stmt(parser);
  } else {
    stmt->else_stmt = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_IF);
}

static ast_stmt_t *parse_stmt_while(parser_t *parser)
{
  ast_stmt_while_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(parser, TOKEN_WHILE);
  stmt->cond = parse_expr(parser);
  expect(parser, TOKEN_DO);
  ++parser->within_loop;
  stmt->do_stmt = parse_stmt(parser);
  --parser->within_loop;
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WHILE);
}

static void maybe_error_break_stmt(parser_t *parser)
{
  assert(parser);
  if (!parser->alive) {
    return;
  }

  if (!parser->within_loop) {
    msg_t *msg = new_msg(parser->src, parser->current_token.region,
      MSG_ERROR, "break statement not within loop");
    error_msg(parser, msg);
  }
}

static ast_stmt_t *parse_break_stmt(parser_t *parser)
{
  expect(parser, TOKEN_BREAK);
  maybe_error_break_stmt(parser);
  return init_ast_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_BREAK);
}

static ast_stmt_t *parse_stmt_call(parser_t *parser)
{
  ast_stmt_call_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(parser, TOKEN_CALL);
  stmt->name = parse_ident(parser);
  if (eat(parser, TOKEN_LPAREN)) {
    stmt->args = parse_expr_seq(parser);
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_CALL);
}

static ast_stmt_t *parse_return_stmt(parser_t *parser)
{
  expect(parser, TOKEN_RETURN);
  return init_ast_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_RETURN);
}

static ast_stmt_t *parse_stmt_read(parser_t *parser)
{
  ast_stmt_read_t *stmt = xmalloc(sizeof(ast_stmt_t));

  if (eat(parser, TOKEN_READ)) {
    stmt->newline = 0;
  } else if (eat(parser, TOKEN_READLN)) {
    stmt->newline = 1;
  } else {
    error_unexpected(parser);
  }
  if (eat(parser, TOKEN_LPAREN)) {
    stmt->args = parse_ref_seq(parser);
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_READ);
}

static void maybe_error_output_format(parser_t *parser, ast_output_format_t *format, size_t spec_pos)
{
  assert(parser && format);
  if (!parser->alive) {
    return;
  }

  if (format->expr && format->expr->kind == AST_EXPR_KIND_CONSTANT) {
    ast_lit_t *lit = format->expr->expr.constant.lit;
    if (lit->kind == AST_LIT_KIND_STRING && lit->lit.string.str_len != 1 && format->len) {
      size_t msg_len = parser->current_token.region.pos + parser->current_token.region.len - spec_pos;
      msg_t *msg     = new_msg(parser->src, region_from(spec_pos, msg_len), MSG_ERROR, "wrong output format");
      msg_add_inline_entry(msg, region_from(spec_pos, msg_len), "field width cannot be used for string");
      error_msg(parser, msg);
    }
  }
}

static ast_output_format_t *parse_output_format(parser_t *parser)
{
  size_t               spec_pos = 0;
  ast_output_format_t *format   = xmalloc(sizeof(ast_output_format_t));
  format->next                  = NULL;

  format->expr = parse_expr(parser);
  if (eat(parser, TOKEN_COLON)) {
    spec_pos    = parser->current_token.region.pos;
    format->len = parse_number_lit(parser);
  } else {
    format->len = NULL;
  }

  maybe_error_output_format(parser, format, spec_pos);
  return format;
}

static ast_output_format_t *parse_output_format_seq(parser_t *parser)
{
  ast_output_format_t *seq, **tail = &seq;
  do {
    ast_output_format_t *format = parse_output_format(parser);
    *tail                       = format;
    tail                        = &format->next;
  } while (eat(parser, TOKEN_COMMA));
  return seq;
}

static ast_stmt_t *parse_stmt_write(parser_t *parser)
{
  ast_stmt_write_t *stmt = xmalloc(sizeof(ast_stmt_t));

  if (eat(parser, TOKEN_WRITE)) {
    stmt->newline = 0;
  } else if (eat(parser, TOKEN_WRITELN)) {
    stmt->newline = 1;
  } else {
    error_unexpected(parser);
  }
  if (eat(parser, TOKEN_LPAREN)) {
    stmt->formats = parse_output_format_seq(parser);
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->formats = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WRITE);
}

static ast_stmt_t *parse_stmt_compound(parser_t *parser)
{
  ast_stmt_compound_t *stmt = xmalloc(sizeof(ast_stmt_t));
  ast_stmt_t         **tail = &stmt->stmts;

  expect(parser, TOKEN_BEGIN);
  do {
    ast_stmt_t *inner = parse_stmt(parser);
    *tail             = inner;
    tail              = &inner->next;
  } while (eat(parser, TOKEN_SEMI));
  expect(parser, TOKEN_END);
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_COMPOUND);
}

static ast_stmt_t *parse_stmt(parser_t *parser)
{
  if (check(parser, TOKEN_NAME)) {
    return parse_stmt_assign(parser);
  } else if (check(parser, TOKEN_IF)) {
    return parse_stmt_if(parser);
  } else if (check(parser, TOKEN_WHILE)) {
    return parse_stmt_while(parser);
  } else if (check(parser, TOKEN_BREAK)) {
    return parse_break_stmt(parser);
  } else if (check(parser, TOKEN_CALL)) {
    return parse_stmt_call(parser);
  } else if (check(parser, TOKEN_RETURN)) {
    return parse_return_stmt(parser);
  } else if (check(parser, TOKEN_READ) || check(parser, TOKEN_READLN)) {
    return parse_stmt_read(parser);
  } else if (check(parser, TOKEN_WRITE) || check(parser, TOKEN_WRITELN)) {
    return parse_stmt_write(parser);
  } else if (check(parser, TOKEN_BEGIN)) {
    return parse_stmt_compound(parser);
  } else {
    return init_ast_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_EMPTY);
  }
}

static ast_decl_part_t *init_ast_decl_part(ast_decl_part_t *decl_part, ast_decl_part_kind_t kind)
{
  decl_part->kind = kind;
  decl_part->next = NULL;
  return decl_part;
}

static ast_decl_part_t *parse_variable_decl_part(parser_t *parser)
{
  ast_decl_part_variable_t *decl_part = xmalloc(sizeof(ast_decl_part_t));
  ast_decl_variable_t     **tail      = &decl_part->decls;

  expect(parser, TOKEN_VAR);
  do {
    ast_decl_variable_t *decl = xmalloc(sizeof(ast_decl_variable_t));
    decl->next                = NULL;

    decl->names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    decl->type = parse_type(parser);
    expect(parser, TOKEN_SEMI);
    *tail = decl;
    tail  = &decl->next;
  } while (check(parser, TOKEN_NAME));
  return init_ast_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_VARIABLE);
}

static ast_decl_param_t *parse_param_decl(parser_t *parser)
{
  ast_decl_param_t *param = NULL, **tail = &param;

  expect(parser, TOKEN_LPAREN);
  do {
    ast_decl_param_t *decl = xmalloc(sizeof(ast_decl_param_t));
    decl->next             = NULL;

    decl->names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    decl->type = parse_type(parser);
    *tail      = decl;
    tail       = &decl->next;
  } while (eat(parser, TOKEN_SEMI));
  expect(parser, TOKEN_RPAREN);
  return param;
}

static ast_decl_part_t *parse_procedure_decl_part(parser_t *parser)
{
  ast_decl_part_procedure_t *decl_part = xmalloc(sizeof(ast_decl_part_t));

  expect(parser, TOKEN_PROCEDURE);
  decl_part->name = parse_ident(parser);
  if (check(parser, TOKEN_LPAREN)) {
    decl_part->params = parse_param_decl(parser);
  } else {
    decl_part->params = NULL;
  }
  expect(parser, TOKEN_SEMI);
  if (check(parser, TOKEN_VAR)) {
    decl_part->variables = parse_variable_decl_part(parser);
  } else {
    decl_part->variables = NULL;
  }
  decl_part->stmt = parse_stmt_compound(parser);
  expect(parser, TOKEN_SEMI);
  return init_ast_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_PROCEDURE);
}

static ast_decl_part_t *parse_decl_part(parser_t *parser)
{
  ast_decl_part_t *seq = NULL, **tail = &seq;
  while (1) {
    ast_decl_part_t *decl_part;
    if (check(parser, TOKEN_VAR)) {
      decl_part = parse_variable_decl_part(parser);
    } else if (check(parser, TOKEN_PROCEDURE)) {
      decl_part = parse_procedure_decl_part(parser);
    } else {
      break;
    }
    *tail = decl_part;
    tail  = &decl_part->next;
  }
  return seq;
}

static ast_program_t *parse_program(parser_t *parser)
{
  ast_program_t *program = xmalloc(sizeof(ast_program_t));

  expect(parser, TOKEN_PROGRAM);
  program->name = parse_ident(parser);
  expect(parser, TOKEN_SEMI);
  program->decl_part = parse_decl_part(parser);
  program->stmt      = parse_stmt_compound(parser);
  expect(parser, TOKEN_DOT);
  expect(parser, TOKEN_EOF);
  return program;
}

ast_t *parse_source(const source_t *src)
{
  ast_t   *ast = xmalloc(sizeof(ast_t));
  parser_t parser;
  assert(src);

  parser.src         = src;
  parser.storage     = new_symbol_storage();
  parser.alive       = 1;
  parser.error       = 0;
  parser.within_loop = 0;
  cursol_init(&parser.cursol, src, src->src_ptr, src->src_size);
  bump(&parser);

  ast->program = parse_program(&parser);
  ast->storage = parser.storage;
  ast->source  = src;
  if (parser.error) {
    delete_ast(ast);
    return NULL;
  }
  return ast;
}
