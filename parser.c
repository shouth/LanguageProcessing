#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "mppl.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    token_t current_token, next_token;
    uint64_t expected_terminals;
    int alive, error;
} parser_t;

#define validate(parser, ret, nil, deleter) \
    (parser)->alive ? (ret) : (error_unexpected(parser), deleter(ret), (nil));

#define delete_nothing(x) 0

size_t msg_token(char *ptr, token_kind_t type)
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

void error_unexpected(parser_t *parser)
{
    msg_t *msg;
    size_t pos, len;
    token_kind_t i;
    char buf[256], *ptr;
    uint64_t msb, bit;

    assert(parser);

    if (!parser->alive) {
        return;
    }
    parser->alive = 0;
    parser->error = 1;

    assert(parser->expected_terminals);
    pos = parser->next_token.pos;
    len = parser->next_token.len;

    ptr = buf;
    msb = msb64(parser->expected_terminals);
    for (i = 0; i <= TOKEN_EOF; i++) {
        if (bit = ((uint64_t) 1 << i) & parser->expected_terminals) {
            if (ptr != buf) {
                ptr += sprintf(ptr, bit != msb ? ", " : " or ");
            }
            ptr += msg_token(ptr, i);
        }
    }
    msg = new_msg(parser->src, pos, len,
        MSG_ERROR, "expected %s, got `%.*s`", buf,
        (int) parser->next_token.len, parser->next_token.ptr);
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

    msg = new_msg(parser->src, parser->next_token.pos, parser->next_token.len,
        MSG_ERROR, "expected %s, got `%.*s`", str,
        (int) parser->next_token.len, parser->next_token.ptr);
    msg_emit(msg);
    parser->expected_terminals = 0;
}

void bump(parser_t *parser)
{
    assert(parser);

    if (!parser->alive) {
        return;
    }
    parser->current_token = parser->next_token;
    parser->expected_terminals = 0;
    while (1) {
        lex(&parser->cursol, &parser->next_token);
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

int check(parser_t *parser, token_kind_t type)
{
    assert(parser);

    if (!parser->alive) {
        return 0;
    }
    parser->expected_terminals |= (uint64_t) 1 << type;
    return parser->next_token.type == type;
}

int eat(parser_t *parser, token_kind_t type)
{
    int ret;
    assert(parser);

    if (ret = check(parser, type)) {
        bump(parser);
    }
    return ret;
}

int expect(parser_t *parser, token_kind_t type)
{
    int ret;
    assert(parser);

    if (!(ret = eat(parser, type))) {
        error_unexpected(parser);
    }
    return ret;
}

#define validate_ident(parser, ret) validate(parser, ret, NULL, delete_ident)

ident_t *parse_ident(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_NAME);
    return validate_ident(parser,
        new_ident(parser->current_token.ptr, parser->current_token.len));
}

ident_t *parse_ident_seq(parser_t *parser)
{
    ident_t *ret = NULL, *ident;
    assert(parser);

    ret = ident = parse_ident(parser);
    while (eat(parser, TOKEN_COMMA)) {
        ident = ident->next = parse_ident(parser);
    }
    return validate_ident(parser, ret);
}

#define validate_lit(parser, ret) validate(parser, ret, NULL, delete_lit)

lit_t *parse_number_lit(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_NUMBER);
    return validate_lit(parser, new_number_lit(
        parser->current_token.ptr,
        parser->current_token.len,
        parser->current_token.data.number.value));
}

lit_t *parse_boolean_lit(parser_t *parser)
{
    lit_t *ret = NULL;
    assert(parser);

    if (eat(parser, TOKEN_TRUE)) {
        ret = new_boolean_lit(1);
    } else if (eat(parser, TOKEN_FALSE)) {
        ret = new_boolean_lit(0);
    }
    return validate_lit(parser, ret);
}

lit_t *parse_string_lit(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_STRING);
    return validate_lit(parser, new_string_lit(
        parser->current_token.data.string.ptr,
        parser->current_token.data.string.len,
        parser->current_token.data.string.str_len));
}

lit_t *parse_lit(parser_t *parser)
{
    lit_t *ret = NULL;
    assert(parser);

    if (check(parser, TOKEN_NUMBER)) {
        ret = parse_number_lit(parser);
    } else if (check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE)) {
        ret = parse_boolean_lit(parser);
    } else if (check(parser, TOKEN_STRING)) {
        ret = parse_string_lit(parser);
    }
    return validate_lit(parser, ret);
}

#define validate_type(parser, ret) validate(parser, ret, NULL, delete_type)

type_t *parse_std_type(parser_t *parser)
{
    type_t *ret = NULL;
    assert(parser);

    if (eat(parser, TOKEN_INTEGER)) {
        ret = new_std_type(TYPE_INTEGER);
    } else if (eat(parser, TOKEN_BOOLEAN)) {
        ret = new_std_type(TYPE_BOOLEAN);
    } else if (eat(parser, TOKEN_CHAR)) {
        ret = new_std_type(TYPE_CHAR);
    }
    return validate_type(parser, ret);
}

type_t *parse_array_type(parser_t *parser)
{
    lit_t *size;
    type_t *base;
    assert(parser);

    expect(parser, TOKEN_ARRAY);
    expect(parser, TOKEN_LSQPAREN);
    size = parse_number_lit(parser);
    expect(parser, TOKEN_RSQPAREN);
    expect(parser, TOKEN_OF);
    base = parse_std_type(parser);
    return validate_type(parser, new_array_type(base, size));
}

type_t *parse_type(parser_t *parser)
{
    type_t *ret = NULL;
    assert(parser);

    if (check(parser, TOKEN_ARRAY)) {
        ret = parse_array_type(parser);
    } else {
        ret = parse_std_type(parser);
    }
    return validate_type(parser, ret);
}

#define validate_expr(parser, ret) validate(parser, ret, NULL, delete_expr)

expr_t *parse_expr(parser_t *parser);

expr_t *parse_ref(parser_t *parser)
{
    ident_t *ident;
    expr_t *ret;
    assert(parser);

    ident = parse_ident(parser);
    if (eat(parser, TOKEN_LSQPAREN)) {
        expr_t *expr = parse_expr(parser);
        expect(parser, TOKEN_RSQPAREN);
        ret = new_array_subscript_expr(ident, expr);
    } else {
        ret = new_decl_ref_expr(ident);
    }
    return validate_expr(parser, ret);
}

expr_t *parse_ref_seq(parser_t *parser)
{
    expr_t *ret = NULL, *ref;
    assert(parser);

    ret = ref = parse_ref(parser);
    while (eat(parser, TOKEN_COMMA)) {
        ref = ref->next = parse_ref(parser);
    }
    return validate_expr(parser, ret);
}

expr_t *parse_expr_seq(parser_t *parser)
{
    expr_t *ret = NULL, *expr;
    assert(parser);

    ret = expr = parse_expr(parser);
    while (eat(parser, TOKEN_COMMA)) {
        expr = expr->next = parse_expr(parser);
    }
    return validate_expr(parser, ret);
}

expr_t *parse_factor(parser_t *parser)
{
    expr_t *ret = NULL;
    assert(parser);

    if (check(parser, TOKEN_NAME)) {
        ret = parse_ref(parser);
    } else if (check(parser, TOKEN_NUMBER) || check(parser, TOKEN_TRUE)
        || check(parser, TOKEN_FALSE) || check(parser, TOKEN_STRING))
    {
        lit_t *lit = parse_lit(parser);
        ret = new_constant_expr(lit);
    } else if (eat(parser, TOKEN_LPAREN)) {
        expr_t *expr = parse_expr(parser);
        expect(parser, TOKEN_RPAREN);
        ret = new_paren_expr(expr);
    } else if (eat(parser, TOKEN_NOT)) {
        expr_t *expr = parse_factor(parser);
        ret = new_unary_expr(UNARY_OP_NOT, expr);
    } else if (check(parser, TOKEN_INTEGER) || check(parser, TOKEN_BOOLEAN) || check(parser, TOKEN_CHAR)) {
        type_t *type;
        expr_t *expr;
        type = parse_std_type(parser);
        expect(parser, TOKEN_LPAREN);
        expr = parse_expr(parser);
        expect(parser, TOKEN_RPAREN);
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
        if (eat(parser, TOKEN_STAR)) {
            expr_t *factor = parse_factor(parser);
            ret = new_binary_expr(BINARY_OP_STAR, ret, factor);
        } else if (eat(parser, TOKEN_DIV)) {
            expr_t *factor = parse_factor(parser);
            ret = new_binary_expr(BINARY_OP_DIV, ret, factor);
        } else if (eat(parser, TOKEN_AND)) {
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

    if (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        ret = new_empty_expr();
    } else {
        ret = parse_term(parser);
    }
    while (ret) {
        if (eat(parser, TOKEN_PLUS)) {
            expr_t *term = parse_term(parser);
            ret = new_binary_expr(BINARY_OP_PLUS, ret, term);
        } else if (eat(parser, TOKEN_MINUS)) {
            expr_t *term = parse_term(parser);
            ret = new_binary_expr(BINARY_OP_MINUS, ret, term);
        } else if (eat(parser, TOKEN_OR)) {
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
        if (eat(parser, TOKEN_EQUAL)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_EQUAL, ret, simple);
        } else if (eat(parser, TOKEN_NOTEQ)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_NOTEQ, ret, simple);
        } else if (eat(parser, TOKEN_LE)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_LE, ret, simple);
        } else if (eat(parser, TOKEN_LEEQ)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_LEEQ, ret, simple);
        } else if (eat(parser, TOKEN_GR)) {
            expr_t *simple = parse_simple_expr(parser);
            ret = new_binary_expr(BINARY_OP_GR, ret, simple);
        } else if (eat(parser, TOKEN_GREQ)) {
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
    expr_t *lhs, *rhs;
    assert(parser);

    lhs = parse_ref(parser);
    expect(parser, TOKEN_ASSIGN);
    rhs = parse_expr(parser);
    return validate_stmt(parser, new_assign_stmt(lhs, rhs));
}

stmt_t *parse_if_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *then_stmt, *else_stmt = NULL;
    assert(parser);

    expect(parser, TOKEN_IF);
    cond = parse_expr(parser);
    expect(parser, TOKEN_THEN);
    then_stmt = parse_stmt(parser);
    if (eat(parser, TOKEN_ELSE)) {
        else_stmt = parse_stmt(parser);
    }
    return validate_stmt(parser, new_if_stmt(cond, then_stmt, else_stmt));
}

stmt_t *parse_while_stmt(parser_t *parser)
{
    expr_t *cond;
    stmt_t *do_stmt;
    assert(parser);

    expect(parser, TOKEN_WHILE);
    cond = parse_expr(parser);
    expect(parser, TOKEN_DO);
    do_stmt = parse_stmt(parser);
    return validate_stmt(parser, new_while_stmt(cond, do_stmt));
}

stmt_t *parse_break_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_BREAK);
    return validate_stmt(parser, new_break_stmt());
}

stmt_t *parse_call_stmt(parser_t *parser)
{
    ident_t *name;
    expr_t *args = NULL;
    assert(parser);

    expect(parser, TOKEN_CALL);
    name = parse_ident(parser);
    if (eat(parser, TOKEN_LPAREN)) {
        args = parse_expr_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return validate_stmt(parser, new_call_stmt(name, args));
}

stmt_t *parse_return_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_RETURN);
    return validate_stmt(parser, new_return_stmt());
}

stmt_t *parse_read_stmt(parser_t *parser)
{
    int newline;
    expr_t *args = NULL;
    assert(parser);

    if (eat(parser, TOKEN_READ)) {
        newline = 0;
    } else if (eat(parser, TOKEN_READLN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
    }
    if (eat(parser, TOKEN_LPAREN)) {
        args = parse_ref_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return validate_stmt(parser, new_read_stmt(newline, args));
}

#define validate_output_format(parser, ret) validate(parser, ret, NULL, delete_output_format)

output_format_t *parse_output_format(parser_t *parser)
{
    expr_t *expr;
    lit_t *len = NULL;
    size_t init_pos;
    assert(parser);

    expr = parse_expr(parser);
    if (eat(parser, TOKEN_COLON)) {
        init_pos = parser->current_token.pos;
        len = parse_number_lit(parser);
    }
    if (expr && expr->kind == EXPR_CONSTANT) {
        lit_t *lit = expr->u.constant_expr.lit;
        if (lit->kind == LIT_STRING && lit->u.string_lit.str_len != 1 && len) {
            if (parser->alive) {
                msg_t *msg;
                size_t msg_len;
                parser->alive = 0;
                parser->error = 1;
                msg_len = parser->current_token.pos + parser->current_token.len - init_pos;
                msg = new_msg(parser->src, init_pos, msg_len,
                    MSG_ERROR, "wrong output format");
                msg_add_inline_entry(msg, init_pos, msg_len,
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
    while (eat(parser, TOKEN_COMMA)) {
        expr = expr->next = parse_output_format(parser);
    }
    return validate_output_format(parser, ret);
}

stmt_t *parse_write_stmt(parser_t *parser)
{
    int newline;
    output_format_t *formats = NULL;
    assert(parser);

    if (eat(parser, TOKEN_WRITE)) {
        newline = 0;
    } else if (eat(parser, TOKEN_WRITELN)) {
        newline = 1;
    } else {
        error_unexpected(parser);
    }
    if (eat(parser, TOKEN_LPAREN)) {
        formats = parse_output_format_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return validate_stmt(parser, new_write_stmt(newline, formats));
}

stmt_t *parse_compound_stmt(parser_t *parser)
{
    stmt_t *stmts, *cur;
    assert(parser);

    expect(parser, TOKEN_BEGIN);
    stmts = cur = parse_stmt(parser);
    while (eat(parser, TOKEN_SEMI)) {
        cur = cur->next = parse_stmt(parser);
    }
    expect(parser, TOKEN_END);
    return validate_stmt(parser, new_compound_stmt(stmts));
}

stmt_t *parse_stmt(parser_t *parser)
{
    stmt_t *ret = NULL;
    assert(parser);

    if (check(parser, TOKEN_NAME)) {
        ret = parse_assign_stmt(parser);
    } else if (check(parser, TOKEN_IF)) {
        ret = parse_if_stmt(parser);
    } else if (check(parser, TOKEN_WHILE)) {
        ret = parse_while_stmt(parser);
    } else if (check(parser, TOKEN_BREAK)) {
        ret = parse_break_stmt(parser);
    } else if (check(parser, TOKEN_CALL)) {
        ret = parse_call_stmt(parser);
    } else if (check(parser, TOKEN_RETURN)) {
        ret = parse_return_stmt(parser);
    } else if (check(parser, TOKEN_READ) || check(parser, TOKEN_READLN)) {
        ret = parse_read_stmt(parser);
    } else if (check(parser, TOKEN_WRITE) || check(parser, TOKEN_WRITELN)) {
        ret = parse_write_stmt(parser);
    } else if (check(parser, TOKEN_BEGIN)) {
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

    expect(parser, TOKEN_VAR);

    names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    type = parse_type(parser);
    expect(parser, TOKEN_SEMI);
    decls = cur = new_variable_decl(names, type);
    while (check(parser, TOKEN_NAME)) {
        names = parse_ident_seq(parser);
        expect(parser, TOKEN_COLON);
        type = parse_type(parser);
        expect(parser, TOKEN_SEMI);
        cur = cur->next = new_variable_decl(names, type);
    }
    return validate_decl_part(parser, new_variable_decl_part(decls));
}

#define validate_param_decl(parser, ret) validate(parser, ret, NULL, delete_param_decl)

param_decl_t *parse_param_decl(parser_t *parser)
{
    ident_t *names;
    type_t *type;
    param_decl_t *ret = NULL, *param;
    assert(parser);

    expect(parser, TOKEN_LPAREN);
    names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    type = parse_type(parser);
    ret = param = new_param_decl(names, type);
    while (eat(parser, TOKEN_SEMI)) {
        names = parse_ident_seq(parser);
        expect(parser, TOKEN_COLON);
        type = parse_type(parser);
        param = param->next = new_param_decl(names, type);
    }
    expect(parser, TOKEN_RPAREN);
    return validate_param_decl(parser, ret);
}

decl_part_t *parse_procedure_decl_part(parser_t *parser)
{
    ident_t *name;
    param_decl_t *params;
    decl_part_t *variables;
    stmt_t *stmt;
    assert(parser);

    expect(parser, TOKEN_PROCEDURE);
    name = parse_ident(parser);
    params = check(parser, TOKEN_LPAREN)
        ? parse_param_decl(parser) : NULL;
    expect(parser, TOKEN_SEMI);
    variables = check(parser, TOKEN_VAR)
        ? parse_variable_decl_part(parser) : NULL;
    stmt = parse_compound_stmt(parser);
    expect(parser, TOKEN_SEMI);
    return validate_decl_part(parser,
        new_procedure_decl_part(name, params, variables, stmt));
}

decl_part_t *parse_decl_part(parser_t *parser)
{
    decl_part_t *ret = NULL, *cur;
    assert(parser);

    if (check(parser, TOKEN_VAR)) {
        ret = parse_variable_decl_part(parser);
    } else if (check(parser, TOKEN_PROCEDURE)) {
        ret = parse_procedure_decl_part(parser);
    }
    cur = ret;
    while (cur) {
        if (check(parser, TOKEN_VAR)) {
            cur = cur->next = parse_variable_decl_part(parser);
        } else if (check(parser, TOKEN_PROCEDURE)) {
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

    expect(parser, TOKEN_PROGRAM);
    name = parse_ident(parser);
    expect(parser, TOKEN_SEMI);
    decl_part = parse_decl_part(parser);
    stmt = parse_compound_stmt(parser);
    expect(parser, TOKEN_DOT);
    expect(parser, TOKEN_EOF);
    return validate_program(parser, new_program(name, decl_part, stmt));
}

ast_t *parse(const source_t *src)
{
    program_t *program;
    parser_t parser;
    assert(src);

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
