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
    size_t ret;
    assert(parser != NULL);

    if (eat(parser, TERMINAL_NUMBER)) {
        ret = parser->last_terminal.data.number.value;
    } else {
        error_unexpected(parser);
        return SIZE_MAX;
    }
    return ret;
}

ident_t *parse_ident(parser_t *parser)
{
    ident_t *ret;
    assert(parser != NULL);

    if (eat(parser, TERMINAL_NAME)) {
        ret = (ident_t *) xmalloc(sizeof(ident_t));
        ret->ptr = parser->last_terminal.ptr;
        ret->len = parser->last_terminal.len;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

ident_t *parse_ident_seq(parser_t *parser)
{
    ident_t *ret, *ident;
    assert(parser != NULL);

    ret = parse_ident(parser);
    for (ident = ret; eat(parser, TERMINAL_COMMA); ident = ident->next) {
        ident->next = parse_ident(parser);
    }
    return ret;
}

type_t *parse_standard_type(parser_t *parser)
{
    type_t *ret;
    assert(parser != NULL);

    ret = (type_t *) xmalloc(sizeof(type_t));
    if (eat(parser, TERMINAL_INTEGER)) {
        ret->kind = TYPE_INTEGER;
    } else if (eat(parser, TERMINAL_BOOLEAN)) {
        ret->kind = TYPE_BOOLEAN;
    } else if (eat(parser, TERMINAL_CHAR)) {
        ret->kind = TYPE_CHAR;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

type_t *parse_array_type(parser_t *parser)
{
    type_t *ret;
    assert(parser != NULL);

    ret = (type_t *) xmalloc(sizeof(type_t));
    ret->kind = TYPE_ARRAY;
    expect(parser, TERMINAL_ARRAY);
    expect(parser, TERMINAL_LSQPAREN);
    ret->array.len = parse_number(parser);
    expect(parser, TERMINAL_RSQPAREN);
    expect(parser, TERMINAL_OF);
    ret->array.base = parse_standard_type(parser);
    return ret;
}

type_t *parse_type(parser_t *parser)
{
    assert(parser != NULL);

    if (check(parser, TERMINAL_ARRAY)) {
        return parse_array_type(parser);
    } else {
        return parse_standard_type(parser);
    }
}

lit_t *parse_number_lit(parser_t *parser)
{
    lit_t *ret;
    number_lit_t *lit;
    assert(parser != NULL);

    ret = (lit_t *) xmalloc(sizeof(lit_t));
    ret->kind = LIT_NUMBER;
    lit = &ret->u.number_lit;

    if (eat(parser, TERMINAL_NUMBER)) {
        lit->value = parser->last_terminal.data.number.value;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

lit_t *parse_boolean_lit(parser_t *parser)
{
    lit_t *ret;
    boolean_lit_t *lit;
    assert(parser != NULL);

    ret = (lit_t *) xmalloc(sizeof(lit_t));
    ret->kind = LIT_BOOLEAN;
    lit = &ret->u.boolean_lit;

    if (eat(parser, TERMINAL_TRUE)) {
        lit->value = 1;
    } else if (eat(parser, TERMINAL_FALSE)) {
        lit->value = 0;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

lit_t *parse_string_lit(parser_t *parser)
{
    lit_t *ret;
    string_lit_t *lit;
    assert(parser != NULL);

    ret = (lit_t *) xmalloc(sizeof(lit_t));
    ret->kind = LIT_STRING;
    lit = &ret->u.string_lit;

    if (eat(parser, TERMINAL_STRING)) {
        lit->ptr = parser->last_terminal.data.string.ptr;
        lit->len = parser->last_terminal.data.string.len;
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

lit_t *parse_lit(parser_t *parser)
{
    lit_t *ret;
    assert(parser != NULL);

    if (check(parser, TERMINAL_NUMBER)) {
        ret = parse_number_lit(parser);
    } else if (check(parser, TERMINAL_TRUE) || check(parser, TERMINAL_FALSE)) {
        ret = parse_boolean_lit(parser);
    } else if (check(parser, TERMINAL_STRING)) {
        ret = parse_string_lit(parser);
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
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
    expr_t *ret;
    ident_t *ident;
    assert(parser != NULL);

    ret = (expr_t *) xmalloc(sizeof(expr_t));
    if (check(parser, TERMINAL_NAME)) {
        ident = parse_ident(parser);

        if (eat(parser, TERMINAL_LSQPAREN)) {
            ret->kind = EXPR_ARRAY_SUBSCRIPT;
            ret->u.array_subscript_expr.name = ident;
            ret->u.array_subscript_expr.index_expr = parse_expr(parser);
            expect(parser, TERMINAL_RPAREN);
        } else {
            ret->kind = EXPR_REF;
            ret->u.ref_expr.name = ident;
        }
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

expr_t *parse_factor(parser_t *parser)
{
    expr_t *ret;
    assert(parser != NULL);

    if (check(parser, TERMINAL_NAME)) {
        ret = parse_lvalue(parser);
    } else if (check(parser, TERMINAL_NUMBER) || check(parser, TERMINAL_TRUE)
        || check(parser, TERMINAL_FALSE) || check(parser, TERMINAL_STRING))
    {
        ret = (expr_t *) xmalloc(sizeof(expr_t));
        ret->kind = EXPR_CONSTANT;
        ret->u.constant_expr.lit = parse_lit(parser);
    } else if (eat(parser, TERMINAL_LPAREN)) {
        ret = (expr_t *) xmalloc(sizeof(expr_t));
        ret->kind = EXPR_PAREN;
        ret->u.paren_expr.expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
    } else if (eat(parser, TERMINAL_NOT)) {
        ret = (expr_t *) xmalloc(sizeof(expr_t));
        ret->kind = EXPR_UNARY_OP;
        ret->u.unary_expr.kind = UNARY_OP_NOT;
        ret->u.unary_expr.expr = parse_factor(parser);
    } else if (check(parser, TERMINAL_INTEGER) || check(parser, TERMINAL_BOOLEAN) || check(parser, TERMINAL_CHAR)) {
        ret = (expr_t *) xmalloc(sizeof(expr_t));
        ret->kind = EXPR_CAST;
        ret->u.cast_expr.type = parse_standard_type(parser);
        expect(parser, TERMINAL_LPAREN);
        ret->u.cast_expr.expr = parse_expr(parser);
        expect(parser, TERMINAL_RPAREN);
    } else {
        error_unexpected(parser);
        return NULL;
    }
    return ret;
}

expr_t *parse_term(parser_t *parser)
{
    expr_t *ret, *tmp;
    binary_op_kind_t kind;
    binary_expr_t *expr;
    assert(parser != NULL);

    ret = parse_factor(parser);
    while (1) {
        if (eat(parser, TERMINAL_STAR)) {
            kind = BINARY_OP_STAR;
        } else if (eat(parser, TERMINAL_DIV)) {
            kind = BINARY_OP_DIV;
        } else if (eat(parser, TERMINAL_AND)) {
            kind = BINARY_OP_AND;
        } else {
            break;
        }

        tmp = (expr_t *) xmalloc(sizeof(expr_t));
        tmp->kind = EXPR_BINARY_OP;
        expr = &tmp->u.binary_expr;
        expr->kind = kind;
        expr->lhs = ret;
        expr->rhs = parse_factor(parser);
        ret = tmp;
    }
    return ret;
}

expr_t *parse_simple_expr(parser_t *parser)
{
    expr_t *ret, *tmp;
    binary_op_kind_t kind;
    binary_expr_t *expr;
    assert(parser != NULL);

    if (check(parser, TERMINAL_PLUS) || check(parser, TERMINAL_MINUS)) {
        ret = (expr_t *) xmalloc(sizeof(expr_t));
        ret->kind = EXPR_EMPTY;
    } else {
        ret = parse_term(parser);
    }
    while (1) {
        if (eat(parser, TERMINAL_PLUS)) {
            kind = BINARY_OP_PLUS;
        } else if (eat(parser, TERMINAL_MINUS)) {
            kind = BINARY_OP_MINUS;
        } else if (eat(parser, TERMINAL_OR)) {
            kind = BINARY_OP_OR;
        } else {
            break;
        }

        tmp = (expr_t *) xmalloc(sizeof(expr_t));
        tmp->kind = EXPR_BINARY_OP;
        expr = &tmp->u.binary_expr;
        expr->kind = kind;
        expr->lhs = ret;
        expr->rhs = parse_term(parser);
        ret = tmp;
    }
    return ret;
}

expr_t *parse_expr(parser_t *parser)
{
    expr_t *ret, *tmp;
    binary_op_kind_t kind;
    binary_expr_t *expr;
    assert(parser != NULL);

    ret = parse_simple_expr(parser);
    while (1) {
        if (eat(parser, TERMINAL_EQUAL)) {
            kind = BINARY_OP_EQUAL;
        } else if (eat(parser, TERMINAL_NOTEQ)) {
            kind = BINARY_OP_NOTEQ;
        } else if (eat(parser, TERMINAL_LE)) {
            kind = BINARY_OP_LE;
        } else if (eat(parser, TERMINAL_LEEQ)) {
            kind = BINARY_OP_LEEQ;
        } else if (eat(parser, TERMINAL_GR)) {
            kind = BINARY_OP_GR;
        } else if (eat(parser, TERMINAL_GREQ)) {
            kind = BINARY_OP_GREQ;
        } else {
            break;
        }

        tmp = (expr_t *) xmalloc(sizeof(expr_t));
        tmp->kind = EXPR_BINARY_OP;
        expr = &tmp->u.binary_expr;
        expr->kind = kind;
        expr->lhs = ret;
        expr->rhs = parse_simple_expr(parser);
        ret = tmp;
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
    ret->kind = STMT_BREAK;

    expect(parser, TERMINAL_BREAK);
    return ret;
}

stmt_t *parse_call_stmt(parser_t *parser)
{
    stmt_t *ret;
    call_stmt_t *stmt;
    assert(parser != NULL);

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
    ret->kind = STMT_RETURN;

    expect(parser, TERMINAL_RETURN);
    return ret;
}

stmt_t *parse_read_stmt(parser_t *parser)
{
    stmt_t *ret;
    read_stmt_t *read;
    assert(parser != NULL);

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (output_format_t *) xmalloc(sizeof(output_format_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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

    ret = (stmt_t *) xmalloc(sizeof(stmt_t));
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
        ret = (stmt_t *) xmalloc(sizeof(stmt_t));
        ret->kind = STMT_EMPTY;
    }
    return ret;
}

decl_part_t *parse_variable_decl(parser_t *parser)
{
    decl_part_t *ret;
    variable_decl_t *decl;
    assert(parser != NULL);

    ret = (decl_part_t *) xmalloc(sizeof(decl_part_t));
    ret->kind = DECL_PART_VARIABLE;
    decl = &ret->u.variable_decl;

    expect(parser, TERMINAL_VAR);
    decl->names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    decl->type = parse_type(parser);
    expect(parser, TERMINAL_SEMI);
    while (check(parser, TERMINAL_NAME)) {
        decl = decl->next = (variable_decl_t *) xmalloc(sizeof(variable_decl_t));
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
    ret = param = (params_t *) xmalloc(sizeof(params_t));
    param->names = parse_ident_seq(parser);
    expect(parser, TERMINAL_COLON);
    param->type = parse_type(parser);
    while (eat(parser, TERMINAL_SEMI)) {
        param = param->next = (params_t *) xmalloc(sizeof(params_t));
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

    ret = (decl_part_t *) xmalloc(sizeof(decl_part_t));
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

    ret = (program_t *) xmalloc(sizeof (program_t));
    expect(parser, TERMINAL_PROGRAM);
    ret->name = parse_ident(parser);
    expect(parser, TERMINAL_SEMI);
    ret->decl_part = parse_decl_part(parser);
    ret->stmt = parse_compound_stmt(parser);
    expect(parser, TERMINAL_DOT);
    expect(parser, TERMINAL_EOF);
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
    ret = (ast_t *) xmalloc(sizeof(ast_t));
    ret->program = parse_program(&parser);
    ret->source = src;
    return ret;
}
