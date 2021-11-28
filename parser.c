#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "ast.h"
#include "cursol.h"
#include "lexer.h"
#include "parser.h"
#include "terminal.h"
#include "message.h"
#include "util.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    terminal_t last_terminal, current_terminal;
    uint64_t expected_terminals;
    int alive;
} parser_t;

void error_unexpected(parser_t *parser)
{
    msg_t *msg;
    size_t pos, len;
    terminal_type_t i;
    char buf[256], *ptr;
    uint64_t msb, bit;

    assert(parser != NULL);
    assert(parser->expected_terminals != 0);

    parser->alive = 0;
    if (parser->expected_terminals == (uint64_t) 1 << TERMINAL_SEMI) {
        pos = parser->last_terminal.pos + parser->last_terminal.len;
        len = 1;
    } else {
        pos = parser->current_terminal.pos;
        len = parser->current_terminal.len;
    }

    ptr = buf;
    msb = msb64(parser->expected_terminals);
    for (i = 0; i <= TERMINAL_EOF; i++) {
        if (bit = ((uint64_t) 1 << i) & parser->expected_terminals) {
            if (ptr != buf) {
                ptr += sprintf(ptr, bit != msb ? ", " : " or ");
            }
            ptr += sprintf(ptr, "`%s`", terminal_to_str(i));
        }
    }

    msg = msg_new(parser->src, pos, len,
        MSG_ERROR, "expected %s, got `%.*s`", buf,
        (int) parser->current_terminal.len, parser->current_terminal.ptr);
    if (parser->expected_terminals == (uint64_t) 1 << TERMINAL_SEMI) {
        msg_add_inline_entry(msg, pos, len, "insert `;` here");
        msg_add_inline_entry(msg, parser->current_terminal.pos, parser->current_terminal.len, "unexpected token");
    }
    msg_emit(msg);
    exit(1);
}

void error_expression_expected(parser_t *parser)
{
    msg_t *msg;
    assert(parser != NULL);

    msg = msg_new(parser->src, parser->current_terminal.pos, parser->current_terminal.len,
        MSG_ERROR, "expected expression, got `%.*s`",
        (int) parser->current_terminal.len, parser->current_terminal.ptr);
    msg_emit(msg);
    exit(1);
}

void bump(parser_t *parser)
{
    token_t token;
    assert(parser != NULL);

    if (!parser->alive) {
        return;
    }
    parser->last_terminal = parser->current_terminal;
    parser->expected_terminals = 0;
    while (1) {
        lex(&parser->cursol, &token);
        terminal_from_token(&parser->current_terminal, &token);
        if (parser->current_terminal.type != TERMINAL_NONE) {
            return;
        }
    }
}

int check(parser_t *parser, terminal_type_t type)
{
    assert(parser != NULL);

    if (!parser->alive) {
        return 0;
    }
    parser->expected_terminals |= (uint64_t) 1 << type;
    return parser->current_terminal.type == type;
}

int eat(parser_t *parser, terminal_type_t type)
{
    int ret;
    assert(parser != NULL);
    assert(parser->alive);

    if (!parser->alive) {
        return 0;
    }
    if (ret = check(parser, type)) {
        bump(parser);
    }
    return ret;
}

int expect(parser_t *parser, terminal_type_t type)
{
    int ret;
    assert(parser != NULL);

    if (!parser->alive) {
        return 0;
    }
    if (!(ret = eat(parser, type))) {
        error_unexpected(parser);
    }
    return ret;
}

size_t parse_number(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_NUMBER)) {
        return parser->last_terminal.data.number.value;
    }
    error_unexpected(parser);
    return SIZE_MAX;
}

ident_t *parse_ident(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_NAME)) {
        return new_ident(parser->last_terminal.ptr, parser->last_terminal.len);
    }
    error_unexpected(parser);
    return NULL;
}

ident_t *parse_ident_seq(parser_t *parser)
{
    ident_t *ret, *ident;
    assert(parser != NULL);

    ret = ident = parse_ident(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        ident = ident->next = parse_ident(parser);
    }
    return ret;
}

type_t *parse_std_type(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_INTEGER)) {
        return new_std_type(TYPE_INTEGER);
    } else if (eat(parser, TERMINAL_BOOLEAN)) {
        return new_std_type(TYPE_BOOLEAN);
    } else if (eat(parser, TERMINAL_CHAR)) {
        return new_std_type(TYPE_CHAR);
    }
    error_unexpected(parser);
    return NULL;
}

type_t *parse_array_type(parser_t *parser)
{
    size_t len;
    type_t *base;
    assert(parser != NULL);

    expect(parser, TERMINAL_ARRAY);
    expect(parser, TERMINAL_LSQPAREN);
    len = parse_number(parser);
    expect(parser, TERMINAL_RSQPAREN);
    expect(parser, TERMINAL_OF);
    base = parse_std_type(parser);
    return new_array_type(base, len);
}

type_t *parse_type(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_ARRAY)) {
        return parse_array_type(parser);
    } else {
        return parse_std_type(parser);
    }
}

lit_t *parse_number_lit(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_NUMBER)) {
        return new_number_lit(parser->last_terminal.data.number.value);
    }
    error_unexpected(parser);
    return NULL;
}

lit_t *parse_boolean_lit(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_TRUE)) {
        return new_boolean_lit(1);
    } else if (eat(parser, TERMINAL_FALSE)) {
        return new_boolean_lit(0);
    }
    error_unexpected(parser);
    return NULL;
}

lit_t *parse_string_lit(parser_t *parser)
{
    assert(parser != NULL);

    if (eat(parser, TERMINAL_STRING)) {
        return new_string_lit(
            parser->last_terminal.data.string.ptr,
            parser->last_terminal.data.string.len);
    }
    error_unexpected(parser);
    return NULL;
}

lit_t *parse_lit(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_NUMBER)) {
        return parse_number_lit(parser);
    } else if (check(parser, TERMINAL_TRUE) || check(parser, TERMINAL_FALSE)) {
        return parse_boolean_lit(parser);
    } else if (check(parser, TERMINAL_STRING)) {
        return parse_string_lit(parser);
    }
    error_unexpected(parser);
    return NULL;
}

expr_t *parse_expr(parser_t *parser);

expr_t *parse_expr_seq(parser_t *parser)
{
    expr_t *ret, *expr;
    assert(parser != NULL);

    ret = expr = parse_expr(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        expr = expr->next = parse_expr(parser);
    }
    return ret;
}

expr_t *parse_lvalue(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_NAME)) {
        ident_t *ident = parse_ident(parser);
        if (eat(parser, TERMINAL_LSQPAREN)) {
            expr_t *expr = parse_expr(parser);
            expect(parser, TERMINAL_RSQPAREN);
            return new_array_subscript_expr(ident, expr);
        }
        return new_ref_expr(ident);
    }
    error_unexpected(parser);
    return NULL;
}

expr_t *parse_factor(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_NAME)) {
        return parse_lvalue(parser);
    } else if (check(parser, TERMINAL_NUMBER) || check(parser, TERMINAL_TRUE)
        || check(parser, TERMINAL_FALSE) || check(parser, TERMINAL_STRING))
    {
        lit_t *lit = parse_lit(parser);
        return new_constant_expr(lit);
    } else if (eat(parser, TERMINAL_LPAREN)) {
        expr_t *expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
        return new_paren_expr(expr);
    } else if (eat(parser, TERMINAL_NOT)) {
        expr_t *expr = parse_factor(parser);
        return new_unary_expr(UNARY_OP_NOT, expr);
    } else if (check(parser, TERMINAL_INTEGER) || check(parser, TERMINAL_BOOLEAN) || check(parser, TERMINAL_CHAR)) {
        type_t *type;
        expr_t *expr;
        type = parse_std_type(parser);
        expect(parser, TERMINAL_LPAREN);
        expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
        return new_cast_expr(type, expr);
    }
    return NULL;
}

expr_t *parse_term(parser_t *parser)
{
    expr_t *ret;
    assert(parser != NULL);

    ret = parse_factor(parser);
    while (ret) {
        if (eat(parser, TERMINAL_STAR)) {
            expr_t *factor = parse_factor(parser);
            ret = new_binary_expr(BINARY_OP_STAR, ret, factor);
        } else if (eat(parser, TERMINAL_DIV)) {
            expr_t *factor = parse_factor(parser);
            ret = new_binary_expr(BINARY_OP_DIV, ret, factor);
        } else if (eat(parser, TERMINAL_AND)) {
            expr_t *factor = parse_factor(parser);
            ret = new_binary_expr(BINARY_OP_AND, ret, factor);
        } else {
            break;
        }
    }
    return ret;
}

expr_t *parse_simple_expr(parser_t *parser)
{
    expr_t *ret;
    assert(parser != NULL);

    if (check(parser, TERMINAL_PLUS) || check(parser, TERMINAL_MINUS)) {
        ret = new_empty_expr();
    } else {
        ret = parse_term(parser);
    }
    while (ret) {
        if (eat(parser, TERMINAL_PLUS)) {
            expr_t *term = parse_term(parser);
            ret = new_binary_expr(BINARY_OP_PLUS, ret, term);
        } else if (eat(parser, TERMINAL_MINUS)) {
            expr_t *term = parse_term(parser);
            ret = new_binary_expr(BINARY_OP_MINUS, ret, term);
        } else if (eat(parser, TERMINAL_OR)) {
            expr_t *term = parse_term(parser);
            ret = new_binary_expr(BINARY_OP_OR, ret, term);
        } else {
            break;
        }
    }
    return ret;
}

expr_t *parse_expr(parser_t *parser)
{
    expr_t *ret;
    assert(parser != NULL);

    ret = parse_simple_expr(parser);
    while (ret) {
        if (eat(parser, TERMINAL_EQUAL)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_EQUAL, ret, simple);
        } else if (eat(parser, TERMINAL_NOTEQ)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_NOTEQ, ret, simple);
        } else if (eat(parser, TERMINAL_LE)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_LE, ret, simple);
        } else if (eat(parser, TERMINAL_LEEQ)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_LEEQ, ret, simple);
        } else if (eat(parser, TERMINAL_GR)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_GR, ret, simple);
        } else if (eat(parser, TERMINAL_GREQ)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_GREQ, ret, simple);
        } else {
            break;
        }
    }
    if (!ret) {
        error_expression_expected(parser);
        return NULL;
    }
    return ret;
}

expr_t *parse_lvalue_seq(parser_t *parser)
{
    expr_t *ret, *expr;
    assert(parser != NULL);

    ret = expr = parse_lvalue(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        expr = expr->next = parse_lvalue(parser);
    }
    return ret;
}

stmt_t *parse_stmt(parser_t *parser);

stmt_t *parse_assign_stmt(parser_t *parser)
{
    expr_t *lhs, *rhs;
    assert(parser != NULL);

    lhs = parse_lvalue(parser);
    expect(parser, TERMINAL_ASSIGN);
    rhs = parse_expr(parser);
    return new_assign_stmt(lhs, rhs);
}

stmt_t *parse_if_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *then_stmt, *else_stmt = NULL;
    assert(parser != NULL);

    expect(parser, TERMINAL_IF);
    cond = parse_expr(parser);
    expect(parser, TERMINAL_THEN);
    then_stmt = parse_stmt(parser);
    if (eat(parser, TERMINAL_ELSE)) {
        else_stmt = parse_stmt(parser);
    }
    return new_if_stmt(cond, then_stmt, else_stmt);
}

stmt_t *parse_while_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *do_stmt;
    assert(parser != NULL);

    expect(parser, TERMINAL_WHILE);
    cond = parse_expr(parser);
    expect(parser, TERMINAL_DO);
    do_stmt = parse_stmt(parser);
    return new_while_stmt(cond, do_stmt);
}

stmt_t *parse_break_stmt(parser_t *parser)
{
    stmt_t *ret;
    assert(parser != NULL);

    return new_break_stmt();
}

stmt_t *parse_call_stmt(parser_t *parser)
{
    ident_t *name;
    expr_t *args = NULL;
    assert(parser != NULL);

    expect(parser, TERMINAL_CALL);
    name = parse_ident(parser);
    if (eat(parser, TERMINAL_LPAREN)) {
        args = parse_expr_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return new_call_stmt(name, args);
}

stmt_t *parse_return_stmt(parser_t *parser)
{
    stmt_t *ret;
    assert(parser != NULL);

    expect(parser, TERMINAL_RETURN);
    return new_return_stmt();
}

stmt_t *parse_read_stmt(parser_t *parser)
{
    int newline;
    expr_t *args = NULL;
    assert(parser != NULL);

    if (eat(parser, TERMINAL_READ)) {
        newline = 0;
    } else if (eat(parser, TERMINAL_READLN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        args = parse_lvalue_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return new_read_stmt(newline, args);
}

output_format_t *parse_output_format(parser_t *parser)
{
    expr_t *expr;
    size_t init_pos, len;
    assert(parser != NULL);

    len = SIZE_MAX;
    expr = parse_expr(parser);
    if (eat(parser, TERMINAL_COLON)) {
        init_pos = parser->last_terminal.pos;
        len = parse_number(parser);
    }
    if (expr->kind == EXPR_CONSTANT) {
        lit_t *lit = expr->u.constant_expr.lit;
        if (lit->kind == LIT_STRING && lit->u.string_lit.len > 1 && len != SIZE_MAX) {
            msg_t *msg;
            len = parser->last_terminal.pos + parser->last_terminal.len - init_pos;
            msg = msg_new(parser->src, init_pos, len,
                MSG_ERROR, "wrong output format");
            msg_add_inline_entry(msg, init_pos, len,
                "the field specifier cannot be used here");
            msg_emit(msg);
            exit(1);
            return NULL;
        }
    }
    return new_output_format(expr, len);
}

output_format_t *parse_output_format_seq(parser_t *parser)
{
    output_format_t *ret, *expr;
    assert(parser != NULL);

    ret = expr = parse_output_format(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        expr = expr->next = parse_output_format(parser);
    }
    return ret;
}

stmt_t *parse_write_stmt(parser_t *parser)
{
    int newline;
    output_format_t *formats;
    assert(parser != NULL);

    if (eat(parser, TERMINAL_WRITE)) {
        newline = 0;
    } else if (eat(parser, TERMINAL_WRITELN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        formats = parse_output_format_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return new_write_stmt(newline, formats);
}

stmt_t *parse_compound_stmt(parser_t *parser)
{
    stmt_t *stmts, *cur;
    assert(parser != NULL);

    expect(parser, TERMINAL_BEGIN);
    stmts = cur = parse_stmt(parser);
    while (eat(parser, TERMINAL_SEMI)) {
        cur = cur->next = parse_stmt(parser);
    }
    expect(parser, TERMINAL_END);
    return new_compound_stmt(stmts);
}

stmt_t *parse_stmt(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_NAME)) {
        return parse_assign_stmt(parser);
    } else if (check(parser, TERMINAL_IF)) {
        return parse_if_stmt(parser);
    } else if (check(parser, TERMINAL_WHILE)) {
        return parse_while_stmt(parser);
    } else if (check(parser, TERMINAL_BREAK)) {
        return parse_break_stmt(parser);
    } else if (check(parser, TERMINAL_CALL)) {
        return parse_call_stmt(parser);
    } else if (check(parser, TERMINAL_RETURN)) {
        return parse_return_stmt(parser);
    } else if (check(parser, TERMINAL_READ) || check(parser, TERMINAL_READLN)) {
        return parse_read_stmt(parser);
    } else if (check(parser, TERMINAL_WRITE) || check(parser, TERMINAL_WRITELN)) {
        return parse_write_stmt(parser);
    } else if (check(parser, TERMINAL_BEGIN)) {
        return parse_compound_stmt(parser);
    }
    return new_empty_stmt();
}

decl_part_t *parse_variable_decl_part(parser_t *parser)
{
    variable_decl_t *decls, *cur;
    ident_t *names;
    type_t *type;
    assert(parser != NULL);

    expect(parser, TERMINAL_VAR);

    names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    type = parse_type(parser);
    expect(parser, TERMINAL_SEMI);
    decls = cur = new_variable_decl(names, type);
    while (check(parser, TERMINAL_NAME)) {
        names = parse_ident_seq(parser);
        expect(parser, TERMINAL_COLON);
        type = parse_type(parser);
        expect(parser, TERMINAL_SEMI);
        cur = cur->next = new_variable_decl(names, type);
    }
    return new_variable_decl_part(decls);
}

params_t *parse_params(parser_t *parser)
{
    ident_t *names;
    type_t *type;
    params_t *ret, *param;
    assert(parser != NULL);

    expect(parser, TERMINAL_LPAREN);
    names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    type = parse_type(parser);
    ret = param = new_params(names, type);
    while (eat(parser, TERMINAL_SEMI)) {
        names = parse_ident_seq(parser);
        expect(parser, TERMINAL_COLON);
        type = parse_type(parser);
        param = param->next = new_params(names, type);
    }
    expect(parser, TERMINAL_RPAREN);
    return ret;
}

decl_part_t *parse_procedure_decl_part(parser_t *parser)
{
    ident_t *name;
    params_t *params;
    decl_part_t *variables;
    stmt_t *stmt;
    assert(parser != NULL);

    expect(parser, TERMINAL_PROCEDURE);
    name = parse_ident(parser);
    params = check(parser, TERMINAL_LPAREN)
        ? parse_params(parser) : NULL;
    expect(parser, TERMINAL_SEMI);
    variables = check(parser, TERMINAL_VAR)
        ? parse_variable_decl_part(parser) : NULL;
    stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_SEMI);
    return new_procedure_decl_part(name, params, variables, stmt);
}

decl_part_t *parse_decl_part(parser_t *parser)
{
    decl_part_t *ret, **decl;
    assert(parser != NULL);

    ret = NULL, decl = &ret;
    while (1) {
        if (check(parser, TERMINAL_VAR)) {
            *decl = parse_variable_decl_part(parser);
        } else if (check(parser, TERMINAL_PROCEDURE)) {
            *decl = parse_procedure_decl_part(parser);
        } else {
            break;
        }
        decl = &(*decl)->next;
    }
    return ret;
}

program_t *parse_program(parser_t *parser)
{
    ident_t *name;
    decl_part_t *decl_part;
    stmt_t *stmt;
    assert(parser != NULL);

    expect(parser, TERMINAL_PROGRAM);
    name = parse_ident(parser);
    expect(parser, TERMINAL_SEMI);
    decl_part = parse_decl_part(parser);
    stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_DOT);
    expect(parser, TERMINAL_EOF);
    return new_program(name, decl_part, stmt);
}

ast_t *parse(const source_t *src)
{
    ast_t *ret;
    parser_t parser;
    assert(src != NULL);

    parser.src = src;
    parser.alive = 1;
    cursol_init(&parser.cursol, src, src->src_ptr, src->src_size);
    bump(&parser);
    ret = new(ast_t);
    ret->program = parse_program(&parser);
    ret->source = src;
    return ret;
}
