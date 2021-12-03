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
    int alive, error;
} parser_t;

#define validate(parser, ret, nil, deleter) \
    (parser)->alive ? (ret) : (error_unexpected(parser), deleter(ret), (nil));

#define delete_nothing(x) 0

size_t msg_symbol(char *ptr, terminal_type_t type)
{
    switch (type) {
    case TERMINAL_NAME:
        return sprintf(ptr, "identifier");
    case TERMINAL_NUMBER:
        return sprintf(ptr, "number");
    case TERMINAL_STRING:
        return sprintf(ptr, "string");
    default:
        return sprintf(ptr, "`%s`", terminal_to_str(type));
    }
}

void error_unexpected(parser_t *parser)
{
    msg_t *msg;
    size_t pos, len;
    terminal_type_t i;
    char buf[256], *ptr;
    uint64_t msb, bit;

    assert(parser);

    if (!parser->alive) {
        return;
    }
    parser->alive = 0;
    parser->error = 1;

    assert(parser->expected_terminals);
    pos = parser->current_terminal.pos;
    len = parser->current_terminal.len;

    ptr = buf;
    msb = msb64(parser->expected_terminals);
    for (i = 0; i <= TERMINAL_EOF; i++) {
        if (bit = ((uint64_t) 1 << i) & parser->expected_terminals) {
            if (ptr != buf) {
                ptr += sprintf(ptr, bit != msb ? ", " : " or ");
            }
            ptr += msg_symbol(ptr, i);
        }
    }
    msg = new_msg(parser->src, pos, len,
        MSG_ERROR, "expected %s, got `%.*s`", buf,
        (int) parser->current_terminal.len, parser->current_terminal.ptr);
    msg_emit(msg);
    parser->expected_terminals = 0;
}

void error_expected(parser_t *parser, const char *str)
{
    msg_t *msg;
    assert(parser);

    if (!parser->alive) {
        return;
    }
    parser->alive = 0;
    parser->error = 1;

    msg = new_msg(parser->src, parser->current_terminal.pos, parser->current_terminal.len,
        MSG_ERROR, "expected %s, got `%.*s`", str,
        (int) parser->current_terminal.len, parser->current_terminal.ptr);
    msg_emit(msg);
    parser->expected_terminals = 0;
}

void bump(parser_t *parser)
{
    token_t token;
    assert(parser);

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
    assert(parser);

    if (!parser->alive) {
        return 0;
    }
    parser->expected_terminals |= (uint64_t) 1 << type;
    return parser->current_terminal.type == type;
}

int eat(parser_t *parser, terminal_type_t type)
{
    int ret;
    assert(parser);

    if (ret = check(parser, type)) {
        bump(parser);
    }
    return ret;
}

int expect(parser_t *parser, terminal_type_t type)
{
    int ret;
    assert(parser);

    if (!(ret = eat(parser, type))) {
        error_unexpected(parser);
    }
    return ret;
}

#define validate_number(parser, ret) validate(parser, ret, SIZE_MAX, delete_nothing)

size_t parse_number(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_NUMBER);
    return validate_number(parser, parser->last_terminal.data.number.value);
}

#define validate_ident(parser, ret) validate(parser, ret, NULL, delete_ident)

ident_t *parse_ident(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_NAME);
    return validate_ident(parser,
        new_ident(parser->last_terminal.ptr, parser->last_terminal.len));
}

ident_t *parse_ident_seq(parser_t *parser)
{
    ident_t *ret = NULL, *ident;
    assert(parser);

    ret = ident = parse_ident(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        ident = ident->next = parse_ident(parser);
    }
    return validate_ident(parser, ret);
}

#define validate_type(parser, ret) validate(parser, ret, NULL, delete_type)

type_t *parse_std_type(parser_t *parser)
{
    type_t *ret = NULL;
    assert(parser);

    if (eat(parser, TERMINAL_INTEGER)) {
        ret = new_std_type(TYPE_INTEGER);
    } else if (eat(parser, TERMINAL_BOOLEAN)) {
        ret = new_std_type(TYPE_BOOLEAN);
    } else if (eat(parser, TERMINAL_CHAR)) {
        ret = new_std_type(TYPE_CHAR);
    }
    return validate_type(parser, ret);
}

type_t *parse_array_type(parser_t *parser)
{
    size_t len;
    type_t *base;
    assert(parser);

    expect(parser, TERMINAL_ARRAY);
    expect(parser, TERMINAL_LSQPAREN);
    len = parse_number(parser);
    expect(parser, TERMINAL_RSQPAREN);
    expect(parser, TERMINAL_OF);
    base = parse_std_type(parser);
    return validate_type(parser, new_array_type(base, len));
}

type_t *parse_type(parser_t *parser)
{
    type_t *ret = NULL;
    assert(parser);

    if (check(parser, TERMINAL_ARRAY)) {
        ret = parse_array_type(parser);
    } else {
        ret = parse_std_type(parser);
    }
    return validate_type(parser, ret);
}

#define validate_lit(parser, ret) validate(parser, ret, NULL, delete_lit)

lit_t *parse_number_lit(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_NUMBER);
    return validate_lit(parser,
        new_number_lit(parser->last_terminal.data.number.value));
}

lit_t *parse_boolean_lit(parser_t *parser)
{
    lit_t *ret = NULL;
    assert(parser);

    if (eat(parser, TERMINAL_TRUE)) {
        ret = new_boolean_lit(1);
    } else if (eat(parser, TERMINAL_FALSE)) {
        ret = new_boolean_lit(0);
    }
    return validate_lit(parser, ret);
}

lit_t *parse_string_lit(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_STRING);
    return validate_lit(parser, new_string_lit(
        parser->last_terminal.data.string.ptr,
        parser->last_terminal.data.string.len,
        parser->last_terminal.data.string.str_len));
}

lit_t *parse_lit(parser_t *parser)
{
    lit_t *ret = NULL;
    assert(parser);

    if (check(parser, TERMINAL_NUMBER)) {
        ret = parse_number_lit(parser);
    } else if (check(parser, TERMINAL_TRUE) || check(parser, TERMINAL_FALSE)) {
        ret = parse_boolean_lit(parser);
    } else if (check(parser, TERMINAL_STRING)) {
        ret = parse_string_lit(parser);
    }
    return validate_lit(parser, ret);
}

#define validate_ref(parser, ret) validate(parser, ret, NULL, delete_ref)

expr_t *parse_expr(parser_t *parser);

ref_t *parse_ref(parser_t *parser)
{
    ident_t *ident;
    ref_t *ret;
    assert(parser);

    ident = parse_ident(parser);
    if (eat(parser, TERMINAL_LSQPAREN)) {
        expr_t *expr = parse_expr(parser);
        expect(parser, TERMINAL_RSQPAREN);
        ret = new_array_subscript(ident, expr);
    } else {
        ret = new_decl_ref(ident);
    }
    return validate_ref(parser, ret);
}

ref_t *parse_ref_seq(parser_t *parser)
{
    ref_t *ret = NULL, *ref;
    assert(parser);

    ret = ref = parse_ref(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        ref = ref->next = parse_ref(parser);
    }
    return validate_ref(parser, ret);
}

#define validate_expr(parser, ret) validate(parser, ret, NULL, delete_expr)

expr_t *parse_expr_seq(parser_t *parser)
{
    expr_t *ret = NULL, *expr;
    assert(parser);

    ret = expr = parse_expr(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        expr = expr->next = parse_expr(parser);
    }
    return validate_expr(parser, ret);
}

expr_t *parse_factor(parser_t *parser)
{
    expr_t *ret = NULL;
    assert(parser);

    if (check(parser, TERMINAL_NAME)) {
        ref_t *ref = parse_ref(parser);
        ret = new_ref_expr(ref);
    } else if (check(parser, TERMINAL_NUMBER) || check(parser, TERMINAL_TRUE)
        || check(parser, TERMINAL_FALSE) || check(parser, TERMINAL_STRING))
    {
        lit_t *lit = parse_lit(parser);
        ret = new_constant_expr(lit);
    } else if (eat(parser, TERMINAL_LPAREN)) {
        expr_t *expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
        ret = new_paren_expr(expr);
    } else if (eat(parser, TERMINAL_NOT)) {
        expr_t *expr = parse_factor(parser);
        ret = new_unary_expr(UNARY_OP_NOT, expr);
    } else if (check(parser, TERMINAL_INTEGER) || check(parser, TERMINAL_BOOLEAN) || check(parser, TERMINAL_CHAR)) {
        type_t *type;
        expr_t *expr;
        type = parse_std_type(parser);
        expect(parser, TERMINAL_LPAREN);
        expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
        ret = new_cast_expr(type, expr);
    } else {
        error_expected(parser, "expression");
    }
    return validate_expr(parser, ret);
}

expr_t *parse_term(parser_t *parser)
{
    expr_t *ret = NULL;
    assert(parser);

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
    return validate_expr(parser, ret);
}

expr_t *parse_simple_expr(parser_t *parser)
{
    expr_t *ret = NULL;
    assert(parser);

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
    return validate_expr(parser, ret);
}

expr_t *parse_expr(parser_t *parser)
{
    expr_t *ret = NULL;
    assert(parser);

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
    return validate_expr(parser, ret);
}

#define validate_stmt(parser, ret) validate(parser, ret, NULL, delete_stmt)

stmt_t *parse_stmt(parser_t *parser);

stmt_t *parse_assign_stmt(parser_t *parser)
{
    ref_t *lhs;
    expr_t *rhs;
    assert(parser);

    lhs = parse_ref(parser);
    expect(parser, TERMINAL_ASSIGN);
    rhs = parse_expr(parser);
    return validate_stmt(parser, new_assign_stmt(lhs, rhs));
}

stmt_t *parse_if_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *then_stmt, *else_stmt = NULL;
    assert(parser);

    expect(parser, TERMINAL_IF);
    cond = parse_expr(parser);
    expect(parser, TERMINAL_THEN);
    then_stmt = parse_stmt(parser);
    if (eat(parser, TERMINAL_ELSE)) {
        else_stmt = parse_stmt(parser);
    }
    return validate_stmt(parser, new_if_stmt(cond, then_stmt, else_stmt));
}

stmt_t *parse_while_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *do_stmt;
    assert(parser);

    expect(parser, TERMINAL_WHILE);
    cond = parse_expr(parser);
    expect(parser, TERMINAL_DO);
    do_stmt = parse_stmt(parser);
    return validate_stmt(parser, new_while_stmt(cond, do_stmt));
}

stmt_t *parse_break_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_BREAK);
    return validate_stmt(parser, new_break_stmt());
}

stmt_t *parse_call_stmt(parser_t *parser)
{
    ident_t *name;
    expr_t *args = NULL;
    assert(parser);

    expect(parser, TERMINAL_CALL);
    name = parse_ident(parser);
    if (eat(parser, TERMINAL_LPAREN)) {
        args = parse_expr_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return validate_stmt(parser, new_call_stmt(name, args));
}

stmt_t *parse_return_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TERMINAL_RETURN);
    return validate_stmt(parser, new_return_stmt());
}

stmt_t *parse_read_stmt(parser_t *parser)
{
    int newline;
    ref_t *args = NULL;
    assert(parser);

    if (eat(parser, TERMINAL_READ)) {
        newline = 0;
    } else if (eat(parser, TERMINAL_READLN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        args = parse_ref_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return validate_stmt(parser, new_read_stmt(newline, args));
}

#define validate_output_format(parser, ret) validate(parser, ret, NULL, delete_output_format)

output_format_t *parse_output_format(parser_t *parser)
{
    expr_t *expr;
    size_t init_pos, len;
    assert(parser);

    len = SIZE_MAX;
    expr = parse_expr(parser);
    if (eat(parser, TERMINAL_COLON)) {
        init_pos = parser->last_terminal.pos;
        len = parse_number(parser);
    }
    if (expr && expr->kind == EXPR_CONSTANT) {
        lit_t *lit = expr->u.constant_expr.lit;
        if (lit->kind == LIT_STRING && lit->u.string_lit.str_len != 1 && len != SIZE_MAX) {
            if (parser->alive) {
                msg_t *msg;
                parser->alive = 0;
                parser->error = 1;
                len = parser->last_terminal.pos + parser->last_terminal.len - init_pos;
                msg = new_msg(parser->src, init_pos, len,
                    MSG_ERROR, "wrong output format");
                msg_add_inline_entry(msg, init_pos, len,
                    "the field specifier cannot be used for string");
                msg_emit(msg);
            }
        }
    }
    return validate_output_format(parser, new_output_format(expr, len));
}

output_format_t *parse_output_format_seq(parser_t *parser)
{
    output_format_t *ret = NULL, *expr;
    assert(parser);

    ret = expr = parse_output_format(parser);
    while (eat(parser, TERMINAL_COMMA)) {
        expr = expr->next = parse_output_format(parser);
    }
    return validate_output_format(parser, ret);
}

stmt_t *parse_write_stmt(parser_t *parser)
{
    int newline;
    output_format_t *formats = NULL;
    assert(parser);

    if (eat(parser, TERMINAL_WRITE)) {
        newline = 0;
    } else if (eat(parser, TERMINAL_WRITELN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
    }
    if (eat(parser, TERMINAL_LPAREN)) {
        formats = parse_output_format_seq(parser);
        expect(parser, TERMINAL_RPAREN);
    }
    return validate_stmt(parser, new_write_stmt(newline, formats));
}

stmt_t *parse_compound_stmt(parser_t *parser)
{
    stmt_t *stmts, *cur;
    assert(parser);

    expect(parser, TERMINAL_BEGIN);
    stmts = cur = parse_stmt(parser);
    while (eat(parser, TERMINAL_SEMI)) {
        cur = cur->next = parse_stmt(parser);
    }
    if (!eat(parser, TERMINAL_END)) {
        error_expected(parser, "statement");
    }
    return validate_stmt(parser, new_compound_stmt(stmts));
}

stmt_t *parse_stmt(parser_t *parser)
{
    stmt_t *ret = NULL;
    assert(parser);

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
        ret = new_empty_stmt();
    }
    return validate_stmt(parser, ret);
}

#define validate_decl_part(parser, ret) validate(parser, ret, NULL, delete_decl_part)

decl_part_t *parse_variable_decl_part(parser_t *parser)
{
    variable_decl_t *decls, *cur;
    ident_t *names;
    type_t *type;
    assert(parser);

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
    return validate_decl_part(parser, new_variable_decl_part(decls));
}

#define validate_params(parser, ret) validate(parser, ret, NULL, delete_params)

params_t *parse_params(parser_t *parser)
{
    ident_t *names;
    type_t *type;
    params_t *ret = NULL, *param;
    assert(parser);

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
    return validate_params(parser, ret);
}

decl_part_t *parse_procedure_decl_part(parser_t *parser)
{
    ident_t *name;
    params_t *params;
    decl_part_t *variables;
    stmt_t *stmt;
    assert(parser);

    expect(parser, TERMINAL_PROCEDURE);
    name = parse_ident(parser);
    params = check(parser, TERMINAL_LPAREN)
        ? parse_params(parser) : NULL;
    expect(parser, TERMINAL_SEMI);
    variables = check(parser, TERMINAL_VAR)
        ? parse_variable_decl_part(parser) : NULL;
    stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_SEMI);
    return validate_decl_part(parser,
        new_procedure_decl_part(name, params, variables, stmt));
}

decl_part_t *parse_decl_part(parser_t *parser)
{
    decl_part_t *ret = NULL, *cur;
    assert(parser);

    if (check(parser, TERMINAL_VAR)) {
        ret = parse_variable_decl_part(parser);
    } else if (check(parser, TERMINAL_PROCEDURE)) {
        ret = parse_procedure_decl_part(parser);
    }
    cur = ret;
    while (cur) {
        if (check(parser, TERMINAL_VAR)) {
            cur = cur->next = parse_variable_decl_part(parser);
        } else if (check(parser, TERMINAL_PROCEDURE)) {
            cur = cur->next = parse_procedure_decl_part(parser);
        } else {
            break;
        }
    }
    return validate_decl_part(parser, ret);
}

#define validate_program(parser, ret) validate(parser, ret, NULL, delete_program)

program_t *parse_program(parser_t *parser)
{
    ident_t *name;
    decl_part_t *decl_part;
    stmt_t *stmt;
    assert(parser);

    expect(parser, TERMINAL_PROGRAM);
    name = parse_ident(parser);
    expect(parser, TERMINAL_SEMI);
    decl_part = parse_decl_part(parser);
    stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_DOT);
    expect(parser, TERMINAL_EOF);
    return validate_program(parser, new_program(name, decl_part, stmt));
}

ast_t *parse(const source_t *src)
{
    program_t *program;
    parser_t parser;
    assert(src != NULL);

    parser.src = src;
    parser.alive = 1;
    parser.error = 0;
    cursol_init(&parser.cursol, src, src->src_ptr, src->src_size);
    bump(&parser);
    program = parse_program(&parser);
    if (parser.error) {
        delete_program(program);
        return NULL;
    }
    return new_ast(program, src);
}
