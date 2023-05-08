#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "context.h"
#include "lexer.h"
#include "message.h"
#include "source.h"
#include "token.h"
#include "utility.h"

typedef struct {
  const source_t *src;
  lexer_t         lexer;
  context_t      *context;
  token_t         current, next;

  unsigned long expected[TOKEN_CATEGORY_COUNT];
  int           within_loop;
  int           alive, error;
} parser_t;

static void error_msg(parser_t *parser, msg_t *msg)
{
  parser->alive = 0;
  parser->error = 1;
  msg_emit(msg);
}

static void error_expected(parser_t *parser, const char *str)
{
  assert(*str != 0);
  if (parser->alive) {
    msg_t *msg = msg_new(parser->src, parser->next.region, MSG_ERROR,
      "expected %s, got `%.*s`", str, (int) parser->next.region.len, parser->next.ptr);
    error_msg(parser, msg);
    memset(parser->expected, 0, sizeof(parser->expected));
  }
}

static void error_unexpected(parser_t *parser)
{
  if (parser->alive) {
    char             buf[1024], *ptr = buf;
    token_category_t i;
    token_category_t last_type;
    long             last_token = -1;

    for (i = TOKEN_CATEGORY_COUNT - 1; i >= 0; --i) {
      if (parser->expected[i]) {
        last_type  = i;
        last_token = bit_left_most(parser->expected[i]);
        break;
      }
    }
    assert(last_token != -1);

    for (i = 0; i < TOKEN_CATEGORY_COUNT; ++i) {
      while (parser->expected[i]) {
        long         current = bit_right_most(parser->expected[i]);
        token_kind_t kind    = (i << 8) | current;
        if (ptr != buf) {
          ptr += sprintf(ptr, i == last_type && current == last_token ? " or " : ", ");
        }
        switch (kind) {
        case TOKEN_IDENT:
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_EOF:
          ptr += sprintf(ptr, "%s", token_to_str(kind));
          break;
        default:
          ptr += sprintf(ptr, "`%s`", token_to_str(kind));
          break;
        }
        parser->expected[i] ^= 1ul << current;
      }
    }
    error_expected(parser, buf);
  }
}

static void bump(parser_t *parser)
{
  if (parser->alive) {
    parser->current = parser->next;
    memset(parser->expected, 0, sizeof(parser->expected));
    while (1) {
      lex_token(&parser->lexer, &parser->next);
      switch (parser->next.type) {
      case TOKEN_WHITESPACE:
      case TOKEN_BRACES_COMMENT:
      case TOKEN_CSTYLE_COMMENT:
        continue;
      case TOKEN_UNKNOWN:
      case TOKEN_ERROR:
        exit(EXIT_FAILURE);
      default:
        return;
      }
    }
  }
}

static int inspect(parser_t *parser, const char *str)
{
  return !strncmp(str, parser->next.ptr, parser->next.region.len) && !str[parser->next.region.len];
}

static int check(parser_t *parser, token_kind_t kind)
{
  if (!parser->alive) {
    return 0;
  } else if (token_category(kind) == TOKEN_TYPE_KEYWORD) {
    parser->expected[TOKEN_CATEGORY_KEYWORD] |= 1ul << (kind ^ TOKEN_TYPE_KEYWORD);
    return parser->next.type == TOKEN_IDENT && inspect(parser, token_to_str(kind));
  } else if (kind == TOKEN_IDENT) {
    if (parser->next.type == TOKEN_IDENT) {
      parser->expected[TOKEN_CATEGORY_TOKEN] |= 1ul << (TOKEN_IDENT ^ TOKEN_TYPE_TOKEN);
      for (kind = TOKEN_TYPE_KEYWORD; kind < TOKEN_TYPE_KEYWORD_END; ++kind) {
        if (inspect(parser, token_to_str(kind))) {
          return 0;
        }
      }
      return 1;
    }
    return 0;
  } else {
    parser->expected[TOKEN_CATEGORY_TOKEN] |= 1ul << (kind ^ TOKEN_TYPE_TOKEN);
    return parser->next.type == kind;
  }
}

static int eat(parser_t *parser, token_kind_t kind)
{
  int ret = check(parser, kind);
  if (ret) {
    bump(parser);
  }
  return ret;
}

static int expect(parser_t *parser, token_kind_t kind)
{
  int ret = eat(parser, kind);
  if (!ret) {
    error_unexpected(parser);
  }
  return ret;
}

static ast_ident_t *parse_ident(parser_t *parser)
{
  if (expect(parser, TOKEN_IDENT)) {
    ast_ident_t *ident = xmalloc(sizeof(ast_ident_t));
    ident->symbol      = ctx_mk_symbol(parser->context, parser->current.ptr, parser->current.region.len);
    ident->region      = parser->current.region;
    ident->next        = NULL;
    return ident;
  } else {
    return NULL;
  }
}

static ast_lit_t *init_lit(ast_lit_t *lit, ast_lit_kind_t kind, region_t region)
{
  lit->kind   = kind;
  lit->region = region;
  return lit;
}

static ast_lit_t *parse_lit_number(parser_t *parser)
{
  if (expect(parser, TOKEN_NUMBER)) {
    ast_lit_number_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->symbol           = ctx_mk_symbol(parser->context, parser->current.ptr, parser->current.region.len);
    lit->value            = parser->current.data.number.value;
    return init_lit((ast_lit_t *) lit, AST_LIT_KIND_NUMBER, parser->current.region);
  } else {
    return NULL;
  }
}

static ast_lit_t *parse_lit_boolean(parser_t *parser)
{
  if (eat(parser, TOKEN_TRUE) || eat(parser, TOKEN_FALSE)) {
    ast_lit_boolean_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->value             = parser->current.type == TOKEN_TRUE;
    return init_lit((ast_lit_t *) lit, AST_LIT_KIND_BOOLEAN, parser->current.region);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_lit_t *parse_lit_string(parser_t *parser)
{
  if (expect(parser, TOKEN_STRING)) {
    ast_lit_string_t *lit   = xmalloc(sizeof(ast_lit_t));
    token_string_t   *token = (token_string_t *) &parser->current;
    lit->str_len            = token->str_len;
    lit->symbol             = ctx_mk_symbol(parser->context, token->ptr, token->len);
    return init_lit((ast_lit_t *) lit, AST_LIT_KIND_STRING, parser->current.region);
  } else {
    return NULL;
  }
}

static ast_lit_t *parse_lit(parser_t *parser)
{
  if (check(parser, TOKEN_NUMBER)) {
    return parse_lit_number(parser);
  } else if (check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE)) {
    return parse_lit_boolean(parser);
  } else if (check(parser, TOKEN_STRING)) {
    return parse_lit_string(parser);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_type_t *init_type(ast_type_t *type, ast_type_kind_t kind, region_t region)
{
  type->kind   = kind;
  type->region = region;
  return type;
}

static ast_type_t *parse_type_std(parser_t *parser)
{
  if (eat(parser, TOKEN_INTEGER)) {
    return init_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_INTEGER, parser->current.region);
  } else if (eat(parser, TOKEN_BOOLEAN)) {
    return init_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_BOOLEAN, parser->current.region);
  } else if (eat(parser, TOKEN_CHAR)) {
    return init_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_CHAR, parser->current.region);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_type_t *parse_type_array(parser_t *parser)
{
  region_t          left = parser->next.region;
  ast_type_array_t *type = xmalloc(sizeof(ast_type_t));

  expect(parser, TOKEN_ARRAY);
  expect(parser, TOKEN_LSQPAREN);
  type->size = parse_lit_number(parser);
  expect(parser, TOKEN_RSQPAREN);
  expect(parser, TOKEN_OF);
  type->base = parse_type_std(parser);
  return init_type((ast_type_t *) type, AST_TYPE_KIND_ARRAY, region_unite(left, parser->current.region));
}

static ast_type_t *parse_type(parser_t *parser)
{
  if (check(parser, TOKEN_ARRAY)) {
    return parse_type_array(parser);
  } else if (check(parser, TOKEN_INTEGER) || check(parser, TOKEN_BOOLEAN) || check(parser, TOKEN_CHAR)) {
    return parse_type_std(parser);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_expr_t *init_expr(ast_expr_t *expr, ast_expr_kind_t kind, region_t region)
{
  expr->kind   = kind;
  expr->region = region;
  expr->next   = NULL;
  return expr;
}

static ast_expr_t *parse_expr(parser_t *parser);

static ast_expr_t *parse_ref(parser_t *parser)
{
  region_t     left  = parser->next.region;
  ast_ident_t *ident = parse_ident(parser);

  if (eat(parser, TOKEN_LSQPAREN)) {
    ast_expr_array_subscript_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->subscript                  = parse_expr(parser);
    expr->decl                       = ident;
    expect(parser, TOKEN_RSQPAREN);
    return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_ARRAY_SUBSCRIPT, region_unite(left, parser->current.region));
  } else {
    ast_expr_decl_ref_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->decl                = ident;
    return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_DECL_REF, region_unite(left, parser->current.region));
  }
}

static ast_expr_t *parse_factor(parser_t *parser);

static ast_expr_t *parse_expr_constant(parser_t *parser)
{
  region_t             left = parser->next.region;
  ast_expr_constant_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->lit                 = parse_lit(parser);
  return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_CONSTANT, region_unite(left, parser->current.region));
}

static ast_expr_t *parse_expr_paren(parser_t *parser)
{
  region_t          left = parser->next.region;
  ast_expr_paren_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(parser, TOKEN_LPAREN);
  expr->inner = parse_expr(parser);
  expect(parser, TOKEN_RPAREN);
  return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_PAREN, region_unite(left, parser->current.region));
}

static ast_expr_t *parse_expr_not(parser_t *parser)
{
  region_t        left = parser->next.region;
  ast_expr_not_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(parser, TOKEN_NOT);
  expr->op_region = parser->current.region;
  expr->expr      = parse_factor(parser);
  return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_NOT, region_unite(left, parser->current.region));
}

static ast_expr_t *parse_expr_cast(parser_t *parser)
{
  region_t         left = parser->next.region;
  ast_expr_cast_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->type            = parse_type_std(parser);
  expect(parser, TOKEN_LPAREN);
  expr->cast = parse_expr(parser);
  expect(parser, TOKEN_RPAREN);
  return init_expr((ast_expr_t *) expr, AST_EXPR_KIND_CAST, region_unite(left, parser->current.region));
}

static ast_expr_t *parse_factor(parser_t *parser)
{
  if (check(parser, TOKEN_IDENT)) {
    return parse_ref(parser);
  } else if (check(parser, TOKEN_NUMBER) || check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE) || check(parser, TOKEN_STRING)) {
    return parse_expr_constant(parser);
  } else if (check(parser, TOKEN_LPAREN)) {
    return parse_expr_paren(parser);
  } else if (check(parser, TOKEN_NOT)) {
    return parse_expr_not(parser);
  } else if (check(parser, TOKEN_INTEGER) || check(parser, TOKEN_BOOLEAN) || check(parser, TOKEN_CHAR)) {
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
      binary->op_region         = parser->current.region;
      binary->lhs               = term;
      binary->rhs               = parse_factor(parser);

      term = init_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return term;
}

static ast_expr_t *parse_simple_expr(parser_t *parser)
{
  ast_expr_t *simple;
  if (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
    simple = init_expr(xmalloc(sizeof(ast_expr_t)), AST_EXPR_KIND_EMPTY, region_from(parser->next.region.pos, 0));
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
      binary->op_region         = parser->current.region;
      binary->lhs               = simple;
      binary->rhs               = parse_term(parser);

      simple = init_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
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
      binary->op_region         = parser->current.region;
      binary->lhs               = expr;
      binary->rhs               = parse_simple_expr(parser);

      expr = init_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return expr;
}

static ast_stmt_t *parse_stmt(parser_t *parser);

static ast_stmt_t *init_stmt(ast_stmt_t *stmt, ast_stmt_kind_t kind)
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
  stmt->op_region = parser->current.region;
  stmt->rhs       = parse_expr(parser);
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_ASSIGN);
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
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_IF);
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
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WHILE);
}

static void maybe_error_break_stmt(parser_t *parser)
{
  if (parser->alive) {
    if (!parser->within_loop) {
      msg_t *msg = msg_new(parser->src, parser->current.region,
        MSG_ERROR, "break statement not within loop");
      error_msg(parser, msg);
    }
  }
}

static ast_stmt_t *parse_stmt_break(parser_t *parser)
{
  expect(parser, TOKEN_BREAK);
  maybe_error_break_stmt(parser);
  return init_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_BREAK);
}

static ast_stmt_t *parse_stmt_call(parser_t *parser)
{
  ast_stmt_call_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(parser, TOKEN_CALL);
  stmt->name = parse_ident(parser);
  if (eat(parser, TOKEN_LPAREN)) {
    ast_expr_t **tail = &stmt->args;
    while (((*tail) = parse_expr(parser)) && eat(parser, TOKEN_COMMA)) {
      tail = &(*tail)->next;
    }
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_CALL);
}

static ast_stmt_t *parse_stmt_return(parser_t *parser)
{
  expect(parser, TOKEN_RETURN);
  return init_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_RETURN);
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
    ast_expr_t **tail = &stmt->args;
    while ((*tail = parse_ref(parser)) && eat(parser, TOKEN_COMMA)) {
      tail = &(*tail)->next;
    }
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_READ);
}

static void maybe_error_out_fmt(parser_t *parser, ast_out_fmt_t *format, region_t colon)
{
  if (parser->alive) {
    if (format->expr->kind == AST_EXPR_KIND_CONSTANT) {
      ast_expr_constant_t *expr = (ast_expr_constant_t *) format->expr;
      if (expr->lit->kind == AST_LIT_KIND_STRING) {
        ast_lit_string_t *lit = (ast_lit_string_t *) expr->lit;
        if (lit->str_len != 1) {
          region_t region = region_unite(colon, format->len->region);
          msg_t   *msg    = msg_new(parser->src, region, MSG_ERROR, "wrong output format");
          msg_add_inline(msg, region, "field width cannot be used for string");
          error_msg(parser, msg);
        }
      }
    }
  }
}

static ast_out_fmt_t *parse_out_fmt(parser_t *parser)
{
  ast_out_fmt_t *format = xmalloc(sizeof(ast_out_fmt_t));
  format->next          = NULL;

  format->expr = parse_expr(parser);
  if (eat(parser, TOKEN_COLON)) {
    region_t colon = parser->current.region;
    format->len    = parse_lit_number(parser);
    maybe_error_out_fmt(parser, format, colon);
  } else {
    format->len = NULL;
  }
  return format;
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
    ast_out_fmt_t **tail = &stmt->formats;
    while ((*tail = parse_out_fmt(parser)) && eat(parser, TOKEN_COMMA)) {
      tail = &(*tail)->next;
    }
    expect(parser, TOKEN_RPAREN);
  } else {
    stmt->formats = NULL;
  }
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WRITE);
}

static ast_stmt_t *parse_stmt_compound(parser_t *parser)
{
  ast_stmt_compound_t *stmt = xmalloc(sizeof(ast_stmt_t));
  ast_stmt_t         **tail = &stmt->stmts;

  expect(parser, TOKEN_BEGIN);
  while ((*tail = parse_stmt(parser)) && eat(parser, TOKEN_SEMI)) {
    tail = &(*tail)->next;
  }
  expect(parser, TOKEN_END);
  return init_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_COMPOUND);
}

static ast_stmt_t *parse_stmt(parser_t *parser)
{
  if (check(parser, TOKEN_IDENT)) {
    return parse_stmt_assign(parser);
  } else if (check(parser, TOKEN_IF)) {
    return parse_stmt_if(parser);
  } else if (check(parser, TOKEN_WHILE)) {
    return parse_stmt_while(parser);
  } else if (check(parser, TOKEN_BREAK)) {
    return parse_stmt_break(parser);
  } else if (check(parser, TOKEN_CALL)) {
    return parse_stmt_call(parser);
  } else if (check(parser, TOKEN_RETURN)) {
    return parse_stmt_return(parser);
  } else if (check(parser, TOKEN_READ) || check(parser, TOKEN_READLN)) {
    return parse_stmt_read(parser);
  } else if (check(parser, TOKEN_WRITE) || check(parser, TOKEN_WRITELN)) {
    return parse_stmt_write(parser);
  } else if (check(parser, TOKEN_BEGIN)) {
    return parse_stmt_compound(parser);
  } else {
    return init_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_EMPTY);
  }
}

static ast_decl_part_t *init_decl_part(ast_decl_part_t *decl_part, ast_decl_part_kind_t kind)
{
  decl_part->kind = kind;
  decl_part->next = NULL;
  return decl_part;
}

static ast_decl_variable_t *parse_decl_variable(parser_t *parser)
{
  ast_decl_variable_t *decl = xmalloc(sizeof(ast_decl_variable_t));
  ast_ident_t        **tail = &decl->names;
  decl->next                = NULL;

  while ((*tail = parse_ident(parser)) && eat(parser, TOKEN_COMMA)) {
    tail = &(*tail)->next;
  }
  expect(parser, TOKEN_COLON);
  decl->type = parse_type(parser);
  expect(parser, TOKEN_SEMI);
  return decl;
}

static ast_decl_part_t *parse_decl_part_variable(parser_t *parser)
{
  ast_decl_part_variable_t *decl_part = xmalloc(sizeof(ast_decl_part_t));
  ast_decl_variable_t     **tail      = &decl_part->decls;

  expect(parser, TOKEN_VAR);
  while ((*tail = parse_decl_variable(parser)) && check(parser, TOKEN_IDENT)) {
    tail = &(*tail)->next;
  }
  return init_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_VARIABLE);
}

static ast_decl_param_t *parse_decl_param(parser_t *parser)
{
  ast_decl_param_t *decl = xmalloc(sizeof(ast_decl_param_t));
  ast_ident_t     **tail = &decl->names;
  decl->next             = NULL;

  while ((*tail = parse_ident(parser)) && eat(parser, TOKEN_COMMA)) {
    tail = &(*tail)->next;
  }
  expect(parser, TOKEN_COLON);
  decl->type = parse_type(parser);
  return decl;
}

static ast_decl_part_t *parse_decl_part_procedure(parser_t *parser)
{
  ast_decl_part_procedure_t *decl_part = xmalloc(sizeof(ast_decl_part_t));

  expect(parser, TOKEN_PROCEDURE);
  decl_part->name = parse_ident(parser);
  if (eat(parser, TOKEN_LPAREN)) {
    ast_decl_param_t **tail = &decl_part->params;
    while ((*tail = parse_decl_param(parser)) && eat(parser, TOKEN_SEMI)) {
      tail = &(*tail)->next;
    }
    expect(parser, TOKEN_RPAREN);
  } else {
    decl_part->params = NULL;
  }
  expect(parser, TOKEN_SEMI);
  if (check(parser, TOKEN_VAR)) {
    decl_part->variables = parse_decl_part_variable(parser);
  } else {
    decl_part->variables = NULL;
  }
  decl_part->stmt = parse_stmt_compound(parser);
  expect(parser, TOKEN_SEMI);
  return init_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_PROCEDURE);
}

static ast_decl_part_t *parse_decl_part(parser_t *parser)
{
  if (check(parser, TOKEN_VAR)) {
    return parse_decl_part_variable(parser);
  } else if (check(parser, TOKEN_PROCEDURE)) {
    return parse_decl_part_procedure(parser);
  } else {
    error_unexpected(parser);
    return NULL;
  }
}

static ast_program_t *parse_program(parser_t *parser)
{
  ast_program_t *program = xmalloc(sizeof(ast_program_t));

  expect(parser, TOKEN_PROGRAM);
  program->name = parse_ident(parser);
  expect(parser, TOKEN_SEMI);
  {
    ast_decl_part_t **tail = &program->decl_part;
    while ((check(parser, TOKEN_VAR) || check(parser, TOKEN_PROCEDURE)) && (*tail = parse_decl_part(parser))) {
      tail = &(*tail)->next;
    }
    *tail = NULL;
  }
  program->stmt = parse_stmt_compound(parser);
  expect(parser, TOKEN_DOT);
  expect(parser, TOKEN_EOF);
  return program;
}

int mpplc_parse(context_t *ctx)
{
  parser_t parser;
  parser.src     = ctx->src;
  parser.context = ctx;
  memset(parser.expected, 0, sizeof(parser.expected));
  parser.alive       = 1;
  parser.error       = 0;
  parser.within_loop = 0;
  lexer_init(&parser.lexer, parser.src);
  bump(&parser);

  ctx->ast          = xmalloc(sizeof(ast_t));
  ctx->ast->program = parse_program(&parser);
  if (parser.error) {
    ast_delete(ctx->ast);
    ctx->ast = NULL;
  }
  return !parser.error;
}
