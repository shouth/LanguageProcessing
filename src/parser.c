#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "message.h"
#include "source.h"
#include "utility.h"

static struct {
  const source_t   *src;
  cursol_t          cursol;
  symbol_storage_t *storage;

  token_t  current_token, next_token;
  uint64_t expected_tokens;
  int      within_loop;
  int      alive, error;
} ctx;

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

static void error_msg(msg_t *msg)
{
  ctx.alive = 0;
  ctx.error = 1;
  msg_emit(msg);
}

static void error_expected(const char *str)
{
  if (ctx.alive) {
    token_t *token = &ctx.next_token;
    msg_t   *msg   = new_msg(ctx.src, token->region, MSG_ERROR,
          "expected %s, got `%.*s`", str, (int) token->region.len, token->ptr);
    error_msg(msg);
    ctx.expected_tokens = 0;
  }
}

static void error_unexpected(void)
{
  assert(ctx.expected_tokens);
  if (ctx.alive) {
    char buf[1024], *ptr = buf;
    int  last = leading0(ctx.expected_tokens);
    while (ctx.expected_tokens) {
      int current = trailing0(ctx.expected_tokens);
      if (ptr != buf) {
        ptr += sprintf(ptr, current != last ? ", " : " or ");
      }
      ptr += msg_token(ptr, current);
      ctx.expected_tokens ^= (uint64_t) 1 << current;
    }
    error_expected(buf);
  }
}

static void bump(void)
{
  if (ctx.alive) {
    ctx.current_token   = ctx.next_token;
    ctx.expected_tokens = 0;
    while (1) {
      lex_token(&ctx.cursol, &ctx.next_token);
      switch (ctx.next_token.type) {
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
}

static int check(token_kind_t type)
{
  if (!ctx.alive) {
    return 0;
  }
  ctx.expected_tokens |= (uint64_t) 1 << type;
  return ctx.next_token.type == type;
}

static int eat(token_kind_t type)
{
  int ret = check(type);
  if (ret) {
    bump();
  }
  return ret;
}

static int expect(token_kind_t type)
{
  int ret = eat(type);
  if (!ret) {
    error_unexpected();
  }
  return ret;
}

static ast_ident_t *parse_ident(void)
{
  if (expect(TOKEN_NAME)) {
    ast_ident_t *ident = xmalloc(sizeof(ast_ident_t));
    ident->symbol      = symbol_intern(ctx.storage, ctx.current_token.ptr, ctx.current_token.region.len);
    ident->region      = ctx.current_token.region;
    ident->next        = NULL;
    return ident;
  } else {
    return NULL;
  }
}

static ast_ident_t *parse_ident_seq(void)
{
  ast_ident_t *seq, **tail = &seq;
  do {
    ast_ident_t *ident = parse_ident();
    *tail              = ident;
    tail               = &ident->next;
  } while (eat(TOKEN_COMMA));
  return seq;
}

static ast_lit_t *init_ast_lit(ast_lit_t *lit, ast_lit_kind_t kind, region_t region)
{
  lit->kind   = kind;
  lit->region = region;
  return lit;
}

static ast_lit_t *parse_number_lit(void)
{
  if (expect(TOKEN_NUMBER)) {
    ast_lit_number_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->symbol           = symbol_intern(ctx.storage, ctx.current_token.ptr, ctx.current_token.region.len);
    lit->value            = ctx.current_token.data.number.value;
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_NUMBER, ctx.current_token.region);
  } else {
    return NULL;
  }
}

static ast_lit_t *parse_boolean_lit(void)
{
  if (eat(TOKEN_TRUE) || eat(TOKEN_FALSE)) {
    ast_lit_boolean_t *lit = xmalloc(sizeof(ast_lit_t));
    lit->value             = ctx.current_token.type == TOKEN_TRUE;
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_BOOLEAN, ctx.current_token.region);
  } else {
    error_unexpected();
    return NULL;
  }
}

static ast_lit_t *parse_string_lit(void)
{
  if (expect(TOKEN_STRING)) {
    ast_lit_string_t   *lit  = xmalloc(sizeof(ast_lit_t));
    const token_data_t *data = &ctx.current_token.data;
    lit->str_len             = data->string.str_len;
    lit->symbol              = symbol_intern(ctx.storage, data->string.ptr, data->string.len);
    return init_ast_lit((ast_lit_t *) lit, AST_LIT_KIND_STRING, ctx.current_token.region);
  } else {
    return NULL;
  }
}

static int check_lit(void)
{
  return check(TOKEN_NUMBER) || check(TOKEN_TRUE) || check(TOKEN_FALSE) || check(TOKEN_STRING);
}

static ast_lit_t *parse_lit(void)
{
  if (check(TOKEN_NUMBER)) {
    return parse_number_lit();
  } else if (check(TOKEN_TRUE) || check(TOKEN_FALSE)) {
    return parse_boolean_lit();
  } else if (check(TOKEN_STRING)) {
    return parse_string_lit();
  } else {
    error_unexpected();
    return NULL;
  }
}

static int check_std_type(void)
{
  return check(TOKEN_INTEGER) || check(TOKEN_BOOLEAN) || check(TOKEN_CHAR);
}

static ast_type_t *init_ast_type(ast_type_t *type, ast_type_kind_t kind, region_t region)
{
  type->kind   = kind;
  type->region = region;
  return type;
}

static ast_type_t *parse_std_type(void)
{
  if (eat(TOKEN_INTEGER)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_INTEGER, ctx.current_token.region);
  } else if (eat(TOKEN_BOOLEAN)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_BOOLEAN, ctx.current_token.region);
  } else if (eat(TOKEN_CHAR)) {
    return init_ast_type(xmalloc(sizeof(ast_type_t)), AST_TYPE_KIND_CHAR, ctx.current_token.region);
  } else {
    error_unexpected();
    return NULL;
  }
}

static ast_type_t *parse_array_type(void)
{
  region_t          left = ctx.next_token.region;
  ast_type_array_t *type = xmalloc(sizeof(ast_type_t));

  expect(TOKEN_ARRAY);
  expect(TOKEN_LSQPAREN);
  type->size = parse_number_lit();
  expect(TOKEN_RSQPAREN);
  expect(TOKEN_OF);
  type->base = parse_std_type();

  return init_ast_type((ast_type_t *) type, AST_TYPE_KIND_ARRAY, region_unite(left, ctx.current_token.region));
}

static ast_type_t *parse_type(void)
{
  if (check(TOKEN_ARRAY)) {
    return parse_array_type();
  } else if (check_std_type()) {
    return parse_std_type();
  } else {
    error_unexpected();
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

static ast_expr_t *parse_expr(void);

static ast_expr_t *parse_ref(void)
{
  region_t     left  = ctx.next_token.region;
  ast_ident_t *ident = parse_ident();

  if (eat(TOKEN_LSQPAREN)) {
    ast_expr_array_subscript_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->subscript                  = parse_expr();
    expr->decl                       = ident;
    expect(TOKEN_RSQPAREN);
    return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_ARRAY_SUBSCRIPT, region_unite(left, ctx.current_token.region));
  } else {
    ast_expr_decl_ref_t *expr = xmalloc(sizeof(ast_expr_t));
    expr->decl                = ident;
    return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_DECL_REF, region_unite(left, ctx.current_token.region));
  }
}

static ast_expr_t *parse_ref_seq(void)
{
  ast_expr_t *seq, **tail = &seq;
  do {
    ast_expr_t *expr = parse_ref();
    *tail            = expr;
    tail             = &expr->next;
  } while (eat(TOKEN_COMMA));
  return seq;
}

static ast_expr_t *parse_expr_seq(void)
{
  ast_expr_t *seq, **tail = &seq;
  do {
    ast_expr_t *expr = parse_expr();
    *tail            = expr;
    tail             = &expr->next;
  } while (eat(TOKEN_COMMA));
  return seq;
}

static ast_expr_t *parse_factor(void);

static ast_expr_t *parse_expr_constant(void)
{
  region_t             left = ctx.next_token.region;
  ast_expr_constant_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->lit                 = parse_lit();
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_CONSTANT, region_unite(left, ctx.current_token.region));
}

static ast_expr_t *parse_expr_paren(void)
{
  region_t          left = ctx.next_token.region;
  ast_expr_paren_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(TOKEN_LPAREN);
  expr->inner = parse_expr();
  expect(TOKEN_RPAREN);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_PAREN, region_unite(left, ctx.current_token.region));
}

static ast_expr_t *parse_expr_unary(void)
{
  region_t          left = ctx.next_token.region;
  ast_expr_unary_t *expr = xmalloc(sizeof(ast_expr_t));
  expect(TOKEN_NOT);
  expr->kind      = AST_EXPR_UNARY_KIND_NOT;
  expr->op_region = ctx.current_token.region;
  expr->expr      = parse_factor();
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_UNARY, region_unite(left, ctx.current_token.region));
}

static ast_expr_t *parse_expr_cast(void)
{
  region_t         left = ctx.next_token.region;
  ast_expr_cast_t *expr = xmalloc(sizeof(ast_expr_t));
  expr->type            = parse_std_type();
  expect(TOKEN_LPAREN);
  expr->cast = parse_expr();
  expect(TOKEN_RPAREN);
  return init_ast_expr((ast_expr_t *) expr, AST_EXPR_KIND_CAST, region_unite(left, ctx.current_token.region));
}

static ast_expr_t *parse_factor(void)
{
  if (check(TOKEN_NAME)) {
    return parse_ref();
  } else if (check_lit()) {
    return parse_expr_constant();
  } else if (check(TOKEN_LPAREN)) {
    return parse_expr_paren();
  } else if (check(TOKEN_NOT)) {
    return parse_expr_unary();
  } else if (check_std_type()) {
    return parse_expr_cast();
  } else {
    error_expected("expression");
    return NULL;
  }
}

static ast_expr_t *parse_term(void)
{
  ast_expr_t *term = parse_factor();
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(TOKEN_STAR)) {
      kind = AST_EXPR_BINARY_KIND_STAR;
    } else if (eat(TOKEN_DIV)) {
      kind = AST_EXPR_BINARY_KIND_DIV;
    } else if (eat(TOKEN_AND)) {
      kind = AST_EXPR_BINARY_KIND_AND;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = ctx.current_token.region;
      binary->lhs               = term;
      binary->rhs               = parse_factor();

      term = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return term;
}

static ast_expr_t *parse_simple_expr(void)
{
  ast_expr_t *simple;
  if (check(TOKEN_PLUS) || check(TOKEN_MINUS)) {
    simple = init_ast_expr(xmalloc(sizeof(ast_expr_t)), AST_EXPR_KIND_EMPTY, region_from(ctx.next_token.region.pos, 0));
  } else {
    simple = parse_term();
  }
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(TOKEN_PLUS)) {
      kind = AST_EXPR_BINARY_KIND_PLUS;
    } else if (eat(TOKEN_MINUS)) {
      kind = AST_EXPR_BINARY_KIND_MINUS;
    } else if (eat(TOKEN_OR)) {
      kind = AST_EXPR_BINARY_KIND_OR;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = ctx.current_token.region;
      binary->lhs               = simple;
      binary->rhs               = parse_term();

      simple = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return simple;
}

static ast_expr_t *parse_expr(void)
{
  ast_expr_t *expr = parse_simple_expr();
  while (1) {
    ast_expr_binary_kind_t kind;
    if (eat(TOKEN_EQUAL)) {
      kind = AST_EXPR_BINARY_KIND_EQUAL;
    } else if (eat(TOKEN_NOTEQ)) {
      kind = AST_EXPR_BINARY_KIND_NOTEQ;
    } else if (eat(TOKEN_LE)) {
      kind = AST_EXPR_BINARY_KIND_LE;
    } else if (eat(TOKEN_LEEQ)) {
      kind = AST_EXPR_BINARY_KIND_LEEQ;
    } else if (eat(TOKEN_GR)) {
      kind = AST_EXPR_BINARY_KIND_GR;
    } else if (eat(TOKEN_GREQ)) {
      kind = AST_EXPR_BINARY_KIND_GREQ;
    } else {
      break;
    }

    {
      ast_expr_binary_t *binary = xmalloc(sizeof(ast_expr_t));
      binary->kind              = kind;
      binary->op_region         = ctx.current_token.region;
      binary->lhs               = expr;
      binary->rhs               = parse_simple_expr();

      expr = init_ast_expr((ast_expr_t *) binary, AST_EXPR_KIND_BINARY, region_unite(binary->lhs->region, binary->rhs->region));
    }
  }
  return expr;
}

static ast_stmt_t *parse_stmt(void);

static ast_stmt_t *init_ast_stmt(ast_stmt_t *stmt, ast_stmt_kind_t kind)
{
  stmt->kind = kind;
  stmt->next = NULL;
  return stmt;
}

static ast_stmt_t *parse_stmt_assign(void)
{
  ast_stmt_assign_t *stmt = xmalloc(sizeof(ast_stmt_t));

  stmt->lhs = parse_ref();
  expect(TOKEN_ASSIGN);
  stmt->op_region = ctx.current_token.region;
  stmt->rhs       = parse_expr();
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_ASSIGN);
}

static ast_stmt_t *parse_stmt_if(void)
{
  ast_stmt_if_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(TOKEN_IF);
  stmt->cond = parse_expr();
  expect(TOKEN_THEN);
  stmt->then_stmt = parse_stmt();
  if (eat(TOKEN_ELSE)) {
    stmt->else_stmt = parse_stmt();
  } else {
    stmt->else_stmt = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_IF);
}

static ast_stmt_t *parse_stmt_while(void)
{
  ast_stmt_while_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(TOKEN_WHILE);
  stmt->cond = parse_expr();
  expect(TOKEN_DO);
  ++ctx.within_loop;
  stmt->do_stmt = parse_stmt();
  --ctx.within_loop;
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WHILE);
}

static void maybe_error_break_stmt(void)
{
  if (ctx.alive) {
    if (!ctx.within_loop) {
      msg_t *msg = new_msg(ctx.src, ctx.current_token.region,
        MSG_ERROR, "break statement not within loop");
      error_msg(msg);
    }
  }
}

static ast_stmt_t *parse_break_stmt(void)
{
  expect(TOKEN_BREAK);
  maybe_error_break_stmt();
  return init_ast_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_BREAK);
}

static ast_stmt_t *parse_stmt_call(void)
{
  ast_stmt_call_t *stmt = xmalloc(sizeof(ast_stmt_t));

  expect(TOKEN_CALL);
  stmt->name = parse_ident();
  if (eat(TOKEN_LPAREN)) {
    stmt->args = parse_expr_seq();
    expect(TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_CALL);
}

static ast_stmt_t *parse_return_stmt(void)
{
  expect(TOKEN_RETURN);
  return init_ast_stmt(xmalloc(sizeof(ast_stmt_t)), AST_STMT_KIND_RETURN);
}

static ast_stmt_t *parse_stmt_read(void)
{
  ast_stmt_read_t *stmt = xmalloc(sizeof(ast_stmt_t));

  if (eat(TOKEN_READ)) {
    stmt->newline = 0;
  } else if (eat(TOKEN_READLN)) {
    stmt->newline = 1;
  } else {
    error_unexpected();
  }
  if (eat(TOKEN_LPAREN)) {
    stmt->args = parse_ref_seq();
    expect(TOKEN_RPAREN);
  } else {
    stmt->args = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_READ);
}

static void maybe_error_output_format(ast_output_format_t *format, region_t colon)
{
  if (ctx.alive) {
    if (format->expr->kind == AST_EXPR_KIND_CONSTANT) {
      ast_expr_constant_t *expr = (ast_expr_constant_t *) format->expr;
      if (expr->lit->kind == AST_LIT_KIND_STRING) {
        ast_lit_string_t *lit = (ast_lit_string_t *) expr->lit;
        if (lit->str_len != 1) {
          region_t region = region_unite(colon, format->len->region);
          msg_t   *msg    = new_msg(ctx.src, region, MSG_ERROR, "wrong output format");
          msg_add_inline_entry(msg, region, "field width cannot be used for string");
          error_msg(msg);
        }
      }
    }
  }
}

static ast_output_format_t *parse_output_format(void)
{
  ast_output_format_t *format = xmalloc(sizeof(ast_output_format_t));
  format->next                = NULL;

  format->expr = parse_expr();
  if (eat(TOKEN_COLON)) {
    region_t colon = ctx.current_token.region;
    format->len    = parse_number_lit();
    maybe_error_output_format(format, colon);
  } else {
    format->len = NULL;
  }
  return format;
}

static ast_output_format_t *parse_output_format_seq(void)
{
  ast_output_format_t *seq, **tail = &seq;
  do {
    ast_output_format_t *format = parse_output_format();
    *tail                       = format;
    tail                        = &format->next;
  } while (eat(TOKEN_COMMA));
  return seq;
}

static ast_stmt_t *parse_stmt_write(void)
{
  ast_stmt_write_t *stmt = xmalloc(sizeof(ast_stmt_t));

  if (eat(TOKEN_WRITE)) {
    stmt->newline = 0;
  } else if (eat(TOKEN_WRITELN)) {
    stmt->newline = 1;
  } else {
    error_unexpected();
  }
  if (eat(TOKEN_LPAREN)) {
    stmt->formats = parse_output_format_seq();
    expect(TOKEN_RPAREN);
  } else {
    stmt->formats = NULL;
  }
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_WRITE);
}

static ast_stmt_t *parse_stmt_compound(void)
{
  ast_stmt_compound_t *stmt = xmalloc(sizeof(ast_stmt_t));
  ast_stmt_t         **tail = &stmt->stmts;

  expect(TOKEN_BEGIN);
  do {
    ast_stmt_t *inner = parse_stmt();
    *tail             = inner;
    tail              = &inner->next;
  } while (eat(TOKEN_SEMI));
  expect(TOKEN_END);
  return init_ast_stmt((ast_stmt_t *) stmt, AST_STMT_KIND_COMPOUND);
}

static ast_stmt_t *parse_stmt(void)
{
  if (check(TOKEN_NAME)) {
    return parse_stmt_assign();
  } else if (check(TOKEN_IF)) {
    return parse_stmt_if();
  } else if (check(TOKEN_WHILE)) {
    return parse_stmt_while();
  } else if (check(TOKEN_BREAK)) {
    return parse_break_stmt();
  } else if (check(TOKEN_CALL)) {
    return parse_stmt_call();
  } else if (check(TOKEN_RETURN)) {
    return parse_return_stmt();
  } else if (check(TOKEN_READ) || check(TOKEN_READLN)) {
    return parse_stmt_read();
  } else if (check(TOKEN_WRITE) || check(TOKEN_WRITELN)) {
    return parse_stmt_write();
  } else if (check(TOKEN_BEGIN)) {
    return parse_stmt_compound();
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

static ast_decl_part_t *parse_variable_decl_part(void)
{
  ast_decl_part_variable_t *decl_part = xmalloc(sizeof(ast_decl_part_t));
  ast_decl_variable_t     **tail      = &decl_part->decls;

  expect(TOKEN_VAR);
  do {
    ast_decl_variable_t *decl = xmalloc(sizeof(ast_decl_variable_t));
    decl->next                = NULL;

    decl->names = parse_ident_seq();
    expect(TOKEN_COLON);
    decl->type = parse_type();
    expect(TOKEN_SEMI);
    *tail = decl;
    tail  = &decl->next;
  } while (check(TOKEN_NAME));
  return init_ast_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_VARIABLE);
}

static ast_decl_param_t *parse_param_decl(void)
{
  ast_decl_param_t *param = NULL, **tail = &param;

  expect(TOKEN_LPAREN);
  do {
    ast_decl_param_t *decl = xmalloc(sizeof(ast_decl_param_t));
    decl->next             = NULL;

    decl->names = parse_ident_seq();
    expect(TOKEN_COLON);
    decl->type = parse_type();
    *tail      = decl;
    tail       = &decl->next;
  } while (eat(TOKEN_SEMI));
  expect(TOKEN_RPAREN);
  return param;
}

static ast_decl_part_t *parse_procedure_decl_part(void)
{
  ast_decl_part_procedure_t *decl_part = xmalloc(sizeof(ast_decl_part_t));

  expect(TOKEN_PROCEDURE);
  decl_part->name = parse_ident();
  if (check(TOKEN_LPAREN)) {
    decl_part->params = parse_param_decl();
  } else {
    decl_part->params = NULL;
  }
  expect(TOKEN_SEMI);
  if (check(TOKEN_VAR)) {
    decl_part->variables = parse_variable_decl_part();
  } else {
    decl_part->variables = NULL;
  }
  decl_part->stmt = parse_stmt_compound();
  expect(TOKEN_SEMI);
  return init_ast_decl_part((ast_decl_part_t *) decl_part, AST_DECL_PART_PROCEDURE);
}

static ast_decl_part_t *parse_decl_part(void)
{
  ast_decl_part_t *seq = NULL, **tail = &seq;
  while (1) {
    ast_decl_part_t *decl_part;
    if (check(TOKEN_VAR)) {
      decl_part = parse_variable_decl_part();
    } else if (check(TOKEN_PROCEDURE)) {
      decl_part = parse_procedure_decl_part();
    } else {
      break;
    }
    *tail = decl_part;
    tail  = &decl_part->next;
  }
  return seq;
}

static ast_program_t *parse_program(void)
{
  ast_program_t *program = xmalloc(sizeof(ast_program_t));

  expect(TOKEN_PROGRAM);
  program->name = parse_ident();
  expect(TOKEN_SEMI);
  program->decl_part = parse_decl_part();
  program->stmt      = parse_stmt_compound();
  expect(TOKEN_DOT);
  expect(TOKEN_EOF);
  return program;
}

ast_t *parse_source(const source_t *src)
{
  ast_t   *ast = xmalloc(sizeof(ast_t));
  assert(src);

  ctx.src         = src;
  ctx.storage     = new_symbol_storage();
  ctx.alive       = 1;
  ctx.error       = 0;
  ctx.within_loop = 0;
  cursol_init(&ctx.cursol, src, src->src_ptr, src->src_size);
  bump();

  ast->program = parse_program();
  ast->storage = ctx.storage;
  ast->source  = src;
  if (ctx.error) {
    delete_ast(ast);
    return NULL;
  }
  return ast;
}
