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
    if (!ret) {
        error_expression_expected(parser);
        return NULL;
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
    if (!ret) {
        error_expression_expected(parser);
        return NULL;
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
    stmt_t *ret;
    assign_stmt_t *stmt;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_ASSIGN;
    stmt = &ret->u.assign_stmt;

    stmt->lhs = parse_lvalue(parser);
    expect(parser, TERMINAL_ASSIGN);
    stmt->rhs = parse_expr(parser);
    return ret;
}

stmt_t *parse_if_stmt(parser_t *parser)
{
    stmt_t *ret;
    if_stmt_t *stmt;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_IF;
    stmt = &ret->u.if_stmt;

    expect(parser, TERMINAL_IF);
    stmt->cond = parse_expr(parser);
    expect(parser, TERMINAL_THEN);
    stmt->then_stmt = parse_stmt(parser);
    if (eat(parser, TERMINAL_ELSE)) {
        stmt->else_stmt = parse_stmt(parser);
    }
    return ret;
}

stmt_t *parse_while_stmt(parser_t *parser)
{
    stmt_t *ret;
    while_stmt_t *stmt;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_WHILE;
    stmt = &ret->u.while_stmt;

    expect(parser, TERMINAL_WHILE);
    stmt->cond = parse_expr(parser);
    expect(parser, TERMINAL_DO);
    stmt->do_stmt = parse_stmt(parser);
    return ret;
}

stmt_t *parse_break_stmt(parser_t *parser)
{
    stmt_t *ret;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_BREAK;

    expect(parser, TERMINAL_BREAK);
    return ret;
}

stmt_t *parse_call_stmt(parser_t *parser)
{
    stmt_t *ret;
    call_stmt_t *stmt;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_CALL;
    stmt = &ret->u.call_stmt;

    expect(parser, TERMINAL_CALL);
    stmt->name = parse_ident(parser);
    if (eat(parser, TERMINAL_LPAREN)) {
        stmt->args = parse_expr_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return ret;
}

stmt_t *parse_return_stmt(parser_t *parser)
{
    stmt_t *ret;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_RETURN;

    expect(parser, TERMINAL_RETURN);
    return ret;
}

stmt_t *parse_read_stmt(parser_t *parser)
{
    stmt_t *ret;
    read_stmt_t *read;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_READ;
    read = &ret->u.read_stmt;

    if (eat(parser, TERMINAL_READ)) {
        read->newline = 0;
    } else if (eat(parser, TERMINAL_READLN)) {
        read->newline = 1;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        read->args = parse_lvalue_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return ret;
}

output_format_t *parse_output_format(parser_t *parser)
{
    output_format_t *ret;
    lit_t *lit;
    size_t init_pos, len;
    assert(parser != NULL);

    ret = new(output_format_t);
    ret->len = SIZE_MAX;

    ret->expr = parse_expr(parser);
    if (eat(parser, TERMINAL_COLON)) {
        init_pos = parser->last_terminal.pos;
        ret->len = parse_number(parser);
    }
    if (ret->expr->kind == EXPR_CONSTANT) {
        lit = ret->expr->u.constant_expr.lit;
        if (lit->kind == LIT_STRING && lit->u.string_lit.len > 1 && ret->len != SIZE_MAX) {
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
    return ret;
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
    stmt_t *ret;
    write_stmt_t *write;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_WRITE;
    write = &ret->u.write_stmt;

    if (eat(parser, TERMINAL_WRITE)) {
        write->newline = 0;
    } else if (eat(parser, TERMINAL_WRITELN)) {
        write->newline = 1;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        write->formats = parse_output_format_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return ret;
}

stmt_t *parse_compound_stmt(parser_t *parser)
{
    stmt_t *ret, *stmts;
    compound_stmt_t *stmt;
    assert(parser != NULL);

    ret = new(stmt_t);
    ret->kind = STMT_COMPOUND;
    stmt = &ret->u.compound_stmt;

    expect(parser, TERMINAL_BEGIN);
    stmts = stmt->stmts = parse_stmt(parser);
    while (eat(parser, TERMINAL_SEMI)) {
        stmts = stmts->next = parse_stmt(parser);
    }
    expect(parser, TERMINAL_END);
    return ret;
}

stmt_t *parse_stmt(parser_t *parser)
{
    stmt_t *ret;
    assert(parser != NULL);

    if (check(parser, TERMINAL_NAME)) {
        ret = parse_assign_stmt(parser);
    } else if (check(parser, TERMINAL_IF)) {
        ret = parse_if_stmt(parser);
    } else if (check(parser, TERMINAL_WHILE)) {
        ret = parse_while_stmt(parser);
    } else if (check(parser, TERMINAL_BREAK)) {
        ret = parse_break_stmt(parser);
    } else if (check(parser, TERMINAL_CALL)) {
        ret = parse_call_stmt(parser);
    } else if (check(parser, TERMINAL_RETURN)) {
        ret = parse_return_stmt(parser);
    } else if (check(parser, TERMINAL_READ) || check(parser, TERMINAL_READLN)) {
        ret = parse_read_stmt(parser);
    } else if (check(parser, TERMINAL_WRITE) || check(parser, TERMINAL_WRITELN)) {
        ret = parse_write_stmt(parser);
    } else if (check(parser, TERMINAL_BEGIN)) {
        ret = parse_compound_stmt(parser);
    } else {
        ret = new(stmt_t);
        ret->kind = STMT_EMPTY;
    }
    return ret;
}

decl_part_t *parse_variable_decl(parser_t *parser)
{
    decl_part_t *ret;
    variable_decl_t *decl;
    assert(parser != NULL);

    ret = new(decl_part_t);
    ret->kind = DECL_PART_VARIABLE;
    decl = &ret->u.variable_decl;

    expect(parser, TERMINAL_VAR);
    decl->names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    decl->type = parse_type(parser);
    expect(parser, TERMINAL_SEMI);
    while (check(parser, TERMINAL_NAME)) {
        decl = decl->next = new(variable_decl_t);
        decl->names = parse_ident_seq(parser);
        expect(parser, TERMINAL_COLON);
        decl->type = parse_type(parser);
        expect(parser, TERMINAL_SEMI);
    }
    return ret;
}

params_t *parse_params(parser_t *parser)
{
    params_t *ret, *param;
    assert(parser != NULL);

    expect(parser, TERMINAL_LPAREN);
    ret = param = new(params_t);
    param->names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    param->type = parse_type(parser);
    while (eat(parser, TERMINAL_SEMI)) {
        param = param->next = new(params_t);
        param->names = parse_ident_seq(parser);
        expect(parser, TERMINAL_COLON);
        param->type = parse_type(parser);
    }
    expect(parser, TERMINAL_RPAREN);
    return ret;
}

decl_part_t *parse_procedure_decl(parser_t *parser)
{
    decl_part_t *ret;
    procedure_decl_t *decl;
    assert(parser != NULL);

    ret = new(decl_part_t);
    ret->kind = DECL_PART_PROCEDURE;
    decl = &ret->u.procedure_decl;

    expect(parser, TERMINAL_PROCEDURE);
    decl->name = parse_ident(parser);
    decl->params = check(parser, TERMINAL_LPAREN)
        ? parse_params(parser) : NULL;
    expect(parser, TERMINAL_SEMI);
    decl->variables = check(parser, TERMINAL_VAR)
        ? parse_variable_decl(parser) : NULL;
    decl->stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_SEMI);
    return ret;
}

decl_part_t *parse_decl_part(parser_t *parser)
{
    decl_part_t *ret, **decl;
    assert(parser != NULL);

    ret = NULL, decl = &ret;
    while (1) {
        if (check(parser, TERMINAL_VAR)) {
            *decl = parse_variable_decl(parser);
        } else if (check(parser, TERMINAL_PROCEDURE)) {
            *decl = parse_procedure_decl(parser);
        } else {
            break;
        }
        decl = &(*decl)->next;
    }
    return ret;
}

program_t *parse_program(parser_t *parser)
{
    program_t *ret;
    assert(parser != NULL);

    ret = new(program_t);
    expect(parser, TERMINAL_PROGRAM);
    ret->name = parse_ident(parser);
    expect(parser, TERMINAL_SEMI);
    ret->decl_part = parse_decl_part(parser);
    ret->stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_DOT);
    expect(parser, TERMINAL_EOF);
    return ret;
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
