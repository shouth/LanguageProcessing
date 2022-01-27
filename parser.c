#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "mppl.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;
    symbol_storage_t *storage;

    token_t current_token, next_token;
    uint64_t expected_tokens;
    int within_loop;
    int alive, error;
} parser_t;

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

void error_msg(parser_t *parser, msg_t *msg)
{
    parser->alive = 0;
    parser->error = 1;
    msg_emit(msg);
}

void error_expected(parser_t *parser, const char *str)
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

void error_unexpected(parser_t *parser)
{
    char buf[256], *ptr;
    uint8_t m, l;

    assert(parser);
    if (!parser->alive) {
        return;
    }
    assert(parser->expected_tokens);

    m = msb(parser->expected_tokens);
    ptr = buf;
    while (parser->expected_tokens) {
        l = lsb(parser->expected_tokens);
        if (ptr != buf) {
            ptr += sprintf(ptr, l != m ? ", " : " or ");
        }
        ptr += msg_token(ptr, l);
        parser->expected_tokens ^= (uint64_t) 1 << l;
    }
    error_expected(parser, buf);
}

#define maybe_error_unreachable(parser) ((parser)->alive && (unreachable(), 0))

void bump(parser_t *parser)
{
    assert(parser);

    if (!parser->alive) {
        return;
    }
    parser->current_token = parser->next_token;
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

int check(parser_t *parser, token_kind_t type)
{
    assert(parser);

    if (!parser->alive) {
        return 0;
    }
    parser->expected_tokens |= (uint64_t) 1 << type;
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

ast_ident_t *parse_ident(parser_t *parser)
{
    symbol_t symbol;
    assert(parser);

    if (expect(parser, TOKEN_NAME)) {
        symbol = symbol_intern(parser->storage, parser->current_token.ptr, parser->current_token.region.len);
        return new_ast_ident(symbol, parser->current_token.region);
    }
    return NULL;
}

ast_ident_t *parse_ident_seq(parser_t *parser)
{
    ast_ident_t *ret, *ident;
    assert(parser);

    ret = ident = parse_ident(parser);
    while (eat(parser, TOKEN_COMMA)) {
        ident = ident->next = parse_ident(parser);
    }
    return ret;
}

ast_lit_t *parse_number_lit(parser_t *parser)
{
    symbol_t symbol;
    assert(parser);

    if (expect(parser, TOKEN_NUMBER)) {
        symbol = symbol_intern(parser->storage, parser->current_token.ptr, parser->current_token.region.len);
        return new_ast_number_lit(symbol, parser->current_token.data.number.value, parser->current_token.region);
    }
    return NULL;
}

ast_lit_t *parse_boolean_lit(parser_t *parser)
{
    assert(parser);

    if (eat(parser, TOKEN_TRUE)) {
        return new_ast_boolean_lit(1, parser->current_token.region);
    } else if (eat(parser, TOKEN_FALSE)) {
        return new_ast_boolean_lit(0, parser->current_token.region);
    }

    maybe_error_unreachable(parser);
}

ast_lit_t *parse_string_lit(parser_t *parser)
{
    symbol_t symbol;
    const token_data_t *data;
    assert(parser);

    if (expect(parser, TOKEN_STRING)) {
        data = &parser->current_token.data;
        symbol = symbol_intern(parser->storage, data->string.ptr, data->string.len);
        return new_ast_string_lit(symbol, data->string.str_len, parser->current_token.region);
    }
    return NULL;
}

int check_lit(parser_t *parser)
{ return check(parser, TOKEN_NUMBER) || check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE) || check(parser, TOKEN_STRING); }

ast_lit_t *parse_lit(parser_t *parser)
{
    assert(parser);

    if (check(parser, TOKEN_NUMBER)) {
        return parse_number_lit(parser);
    } else if (check(parser, TOKEN_TRUE) || check(parser, TOKEN_FALSE)) {
        return parse_boolean_lit(parser);
    } else if (check(parser, TOKEN_STRING)) {
        return parse_string_lit(parser);
    }

    maybe_error_unreachable(parser);
}

int check_std_type(parser_t *parser)
{ return check(parser, TOKEN_INTEGER) || check(parser, TOKEN_BOOLEAN) || check(parser, TOKEN_CHAR); }

ast_type_t *parse_std_type(parser_t *parser)
{
    assert(parser);

    if (eat(parser, TOKEN_INTEGER)) {
        return new_ast_std_type(AST_TYPE_INTEGER, parser->current_token.region);
    } else if (eat(parser, TOKEN_BOOLEAN)) {
        return new_ast_std_type(AST_TYPE_BOOLEAN, parser->current_token.region);
    } else if (eat(parser, TOKEN_CHAR)) {
        return new_ast_std_type(AST_TYPE_CHAR, parser->current_token.region);
    }

    maybe_error_unreachable(parser);
}

ast_type_t *parse_array_type(parser_t *parser)
{
    ast_lit_t *size = NULL;
    ast_type_t *base = NULL;
    size_t begin, end;
    assert(parser);

    begin = parser->next_token.region.pos;
    expect(parser, TOKEN_ARRAY);
    expect(parser, TOKEN_LSQPAREN);
    if (check_lit(parser)) {
        size = parse_number_lit(parser);
    } else {
        error_unexpected(parser);
    }
    expect(parser, TOKEN_RSQPAREN);
    expect(parser, TOKEN_OF);
    if (check_std_type(parser)) {
        base = parse_std_type(parser);
    } else {
        error_unexpected(parser);
    }
    end = parser->current_token.region.pos + parser->current_token.region.len;
    return new_ast_array_type(base, size, region_from(begin, end - begin));
}

int check_type(parser_t *parser)
{ return check(parser, TOKEN_ARRAY) || check_std_type(parser); }

ast_type_t *parse_type(parser_t *parser)
{
    assert(parser);

    if (check(parser, TOKEN_ARRAY)) {
        return parse_array_type(parser);
    } else if (check_std_type(parser)) {
        return parse_std_type(parser);
    }

    maybe_error_unreachable(parser);
}

ast_expr_t *parse_expr(parser_t *parser);

ast_expr_t *parse_ref(parser_t *parser)
{
    region_t begin;
    ast_ident_t *ident;
    assert(parser);

    begin = parser->next_token.region;
    ident = parse_ident(parser);
    if (eat(parser, TOKEN_LSQPAREN)) {
        ast_expr_t *expr = parse_expr(parser);
        expect(parser, TOKEN_RSQPAREN);
        return new_ast_array_subscript_expr(ident, expr, region_unite(begin, parser->current_token.region));
    }
    return new_ast_decl_ref_expr(ident, region_unite(begin, parser->current_token.region));
}

ast_expr_t *parse_ref_seq(parser_t *parser)
{
    ast_expr_t *ret, *ref;
    assert(parser);

    ret = ref = parse_ref(parser);
    while (eat(parser, TOKEN_COMMA)) {
        ref = ref->next = parse_ref(parser);
    }
    return ret;
}

ast_expr_t *parse_expr_seq(parser_t *parser)
{
    ast_expr_t *ret, *expr;
    assert(parser);

    ret = expr = parse_expr(parser);
    while (eat(parser, TOKEN_COMMA)) {
        expr = expr->next = parse_expr(parser);
    }
    return ret;
}

ast_expr_t *parse_factor(parser_t *parser)
{
    region_t begin;
    assert(parser);

    begin = parser->next_token.region;
    if (check(parser, TOKEN_NAME)) {
        return parse_ref(parser);
    } else if (check_lit(parser)) {
        ast_lit_t *lit = parse_lit(parser);
        return new_ast_constant_expr(lit, region_unite(begin, parser->current_token.region));
    } else if (eat(parser, TOKEN_LPAREN)) {
        ast_expr_t *expr = parse_expr(parser);
        expect(parser, TOKEN_RPAREN);
        return new_ast_paren_expr(expr, region_unite(begin, parser->current_token.region));
    } else if (eat(parser, TOKEN_NOT)) {
        region_t op_region = parser->current_token.region;
        ast_expr_t *expr = parse_factor(parser);
        return new_ast_unary_expr(AST_UNARY_OP_NOT, expr,
            region_unite(begin, parser->current_token.region), op_region);
    } else if (check_std_type(parser)) {
        ast_expr_t *expr;
        ast_type_t *type = parse_std_type(parser);
        expect(parser, TOKEN_LPAREN);
        expr = parse_expr(parser);
        expect(parser, TOKEN_RPAREN);
        return new_ast_cast_expr(type, expr, region_unite(begin, parser->current_token.region));
    }
    error_expected(parser, "expression");
    return NULL;
}

ast_expr_t *parse_term(parser_t *parser)
{
    ast_expr_t *ret;
    assert(parser);

    ret = parse_factor(parser);
    while (1) {
        ast_expr_t *factor;
        ast_expr_kind_t kind;
        region_t op_region;
        if (eat(parser, TOKEN_STAR)) {
            kind = AST_BINARY_OP_STAR;
        } else if (eat(parser, TOKEN_DIV)) {
            kind = AST_BINARY_OP_DIV;
        } else if (eat(parser, TOKEN_AND)) {
            kind = AST_BINARY_OP_AND;
        } else {
            break;
        }
        op_region = parser->current_token.region;
        factor = parse_factor(parser);
        ret = new_ast_binary_expr(kind, ret, factor,
            region_unite(ret->region, factor->region), op_region);
    }
    return ret;
}

ast_expr_t *parse_simple_expr(parser_t *parser)
{
    ast_expr_t *ret;
    assert(parser);

    if (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        size_t pos = parser->next_token.region.pos;
        ret = new_ast_empty_expr(region_from(pos, pos));
    } else {
        ret = parse_term(parser);
    }
    while (1) {
        ast_expr_t *term;
        ast_expr_kind_t kind;
        region_t op_region;
        if (eat(parser, TOKEN_PLUS)) {
            kind = AST_BINARY_OP_PLUS;
        } else if (eat(parser, TOKEN_MINUS)) {
            kind = AST_BINARY_OP_MINUS;
        } else if (eat(parser, TOKEN_OR)) {
            kind = AST_BINARY_OP_OR;
        } else {
            break;
        }
        op_region = parser->current_token.region;
        term = parse_term(parser);
        ret = new_ast_binary_expr(kind, ret, term,
            region_unite(ret->region, term->region), op_region);
    }
    return ret;
}

ast_expr_t *parse_expr(parser_t *parser)
{
    ast_expr_t *ret;
    assert(parser);

    ret = parse_simple_expr(parser);
    while (1) {
        ast_expr_t *simple;
        ast_expr_kind_t kind;
        region_t op_region;
        if (eat(parser, TOKEN_EQUAL)) {
            kind = AST_BINARY_OP_EQUAL;
        } else if (eat(parser, TOKEN_NOTEQ)) {
            kind = AST_BINARY_OP_NOTEQ;
        } else if (eat(parser, TOKEN_LE)) {
            kind = AST_BINARY_OP_LE;
        } else if (eat(parser, TOKEN_LEEQ)) {
            kind = AST_BINARY_OP_LEEQ;
        } else if (eat(parser, TOKEN_GR)) {
            kind = AST_BINARY_OP_GR;
        } else if (eat(parser, TOKEN_GREQ)) {
            kind = AST_BINARY_OP_GREQ;
        } else {
            break;
        }
        op_region = parser->current_token.region;
        simple = parse_simple_expr(parser);
        ret = new_ast_binary_expr(kind, ret, simple,
            region_unite(ret->region, simple->region), op_region);
    }
    return ret;
}

ast_stmt_t *parse_stmt(parser_t *parser);

ast_stmt_t *parse_assign_stmt(parser_t *parser)
{
    ast_expr_t *lhs, *rhs;
    region_t op_region;
    assert(parser);

    lhs = parse_ref(parser);
    expect(parser, TOKEN_ASSIGN);
    op_region = parser->current_token.region;
    rhs = parse_expr(parser);
    return new_ast_assign_stmt(lhs, rhs, op_region);
}

ast_stmt_t *parse_if_stmt(parser_t *parser)
{
    ast_expr_t *cond;
    ast_stmt_t *then_stmt, *else_stmt = NULL;
    assert(parser);

    expect(parser, TOKEN_IF);
    cond = parse_expr(parser);
    expect(parser, TOKEN_THEN);
    then_stmt = parse_stmt(parser);
    if (eat(parser, TOKEN_ELSE)) {
        else_stmt = parse_stmt(parser);
    }
    return new_ast_if_stmt(cond, then_stmt, else_stmt);
}

ast_stmt_t *parse_while_stmt(parser_t *parser)
{
    ast_expr_t *cond;
    ast_stmt_t *do_stmt;
    assert(parser);

    expect(parser, TOKEN_WHILE);
    cond = parse_expr(parser);
    expect(parser, TOKEN_DO);
    parser->within_loop++;
    do_stmt = parse_stmt(parser);
    parser->within_loop--;
    return new_ast_while_stmt(cond, do_stmt);
}

void maybe_error_break_stmt(parser_t *parser)
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

ast_stmt_t *parse_break_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_BREAK);
    maybe_error_break_stmt(parser);
    return new_ast_break_stmt();
}

ast_stmt_t *parse_call_stmt(parser_t *parser)
{
    ast_ident_t *name;
    ast_expr_t *args = NULL;
    assert(parser);

    expect(parser, TOKEN_CALL);
    name = parse_ident(parser);
    if (eat(parser, TOKEN_LPAREN)) {
        args = parse_expr_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return new_ast_call_stmt(name, args);
}

ast_stmt_t *parse_return_stmt(parser_t *parser)
{
    assert(parser);

    expect(parser, TOKEN_RETURN);
    return new_ast_return_stmt();
}

ast_stmt_t *parse_read_stmt(parser_t *parser)
{
    int newline;
    ast_expr_t *args = NULL;
    assert(parser);

    if (eat(parser, TOKEN_READ)) {
        newline = 0;
    } else if (eat(parser, TOKEN_READLN)) {
        newline = 1;
    } else {
        maybe_error_unreachable(parser);
    }
    if (eat(parser, TOKEN_LPAREN)) {
        args = parse_ref_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return new_ast_read_stmt(newline, args);
}

void maybe_error_output_format(parser_t *parser, ast_output_format_t *format, size_t spec_pos)
{
    assert(parser && format);
    if (!parser->alive) {
        return;
    }

    if (format->expr && format->expr->kind == AST_EXPR_CONSTANT) {
        ast_lit_t *lit = format->expr->u.constant_expr.lit;
        if (lit->kind == AST_LIT_STRING && lit->u.string_lit.str_len != 1 && format->len) {
            size_t msg_len = parser->current_token.region.pos + parser->current_token.region.len - spec_pos;
            msg_t *msg = new_msg(parser->src, region_from(spec_pos, msg_len), MSG_ERROR, "wrong output format");
            msg_add_inline_entry(msg, region_from(spec_pos, msg_len), "field width cannot be used for string");
            error_msg(parser, msg);
        }
    }
}

ast_output_format_t *parse_output_format(parser_t *parser)
{
    ast_expr_t *expr;
    ast_lit_t *len = NULL;
    size_t spec_pos;
    ast_output_format_t *ret;
    assert(parser);

    expr = parse_expr(parser);
    if (eat(parser, TOKEN_COLON)) {
        spec_pos = parser->current_token.region.pos;
        len = parse_number_lit(parser);
    }
    ret = new_ast_output_format(expr, len);
    maybe_error_output_format(parser, ret, spec_pos);
    return ret;
}

ast_output_format_t *parse_output_format_seq(parser_t *parser)
{
    ast_output_format_t *ret, *expr;
    assert(parser);

    ret = expr = parse_output_format(parser);
    while (eat(parser, TOKEN_COMMA)) {
        expr = expr->next = parse_output_format(parser);
    }
    return ret;
}

ast_stmt_t *parse_write_stmt(parser_t *parser)
{
    int newline;
    ast_output_format_t *formats = NULL;
    assert(parser);

    if (eat(parser, TOKEN_WRITE)) {
        newline = 0;
    } else if (eat(parser, TOKEN_WRITELN)) {
        newline = 1;
    } else {
        maybe_error_unreachable(parser);
    }
    if (eat(parser, TOKEN_LPAREN)) {
        formats = parse_output_format_seq(parser);
        expect(parser, TOKEN_RPAREN);
    }
    return new_ast_write_stmt(newline, formats);
}

ast_stmt_t *parse_compound_stmt(parser_t *parser)
{
    ast_stmt_t *stmts, *cur;
    assert(parser);

    expect(parser, TOKEN_BEGIN);
    stmts = cur = parse_stmt(parser);
    while (eat(parser, TOKEN_SEMI)) {
        cur = cur->next = parse_stmt(parser);
    }
    expect(parser, TOKEN_END);
    return new_ast_compound_stmt(stmts);
}

ast_stmt_t *parse_stmt(parser_t *parser)
{
    assert(parser);

    if (check(parser, TOKEN_NAME)) {
        return parse_assign_stmt(parser);
    } else if (check(parser, TOKEN_IF)) {
        return parse_if_stmt(parser);
    } else if (check(parser, TOKEN_WHILE)) {
        return parse_while_stmt(parser);
    } else if (check(parser, TOKEN_BREAK)) {
        return parse_break_stmt(parser);
    } else if (check(parser, TOKEN_CALL)) {
        return parse_call_stmt(parser);
    } else if (check(parser, TOKEN_RETURN)) {
        return parse_return_stmt(parser);
    } else if (check(parser, TOKEN_READ) || check(parser, TOKEN_READLN)) {
        return parse_read_stmt(parser);
    } else if (check(parser, TOKEN_WRITE) || check(parser, TOKEN_WRITELN)) {
        return parse_write_stmt(parser);
    } else if (check(parser, TOKEN_BEGIN)) {
        return parse_compound_stmt(parser);
    }
    return new_ast_empty_stmt();
}

ast_decl_part_t *parse_variable_decl_part(parser_t *parser)
{
    ast_variable_decl_t *decls, *cur;
    ast_ident_t *names;
    ast_type_t *type = NULL;
    assert(parser);

    expect(parser, TOKEN_VAR);

    names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    if (check_type(parser)) {
        type = parse_type(parser);
    } else {
        error_unexpected(parser);
    }
    expect(parser, TOKEN_SEMI);
    decls = cur = new_ast_variable_decl(names, type);
    while (check(parser, TOKEN_NAME)) {
        names = parse_ident_seq(parser);
        expect(parser, TOKEN_COLON);
        if (check_type(parser)) {
            type = parse_type(parser);
        } else {
            error_unexpected(parser);
        }
        expect(parser, TOKEN_SEMI);
        cur = cur->next = new_ast_variable_decl(names, type);
    }
    return new_variable_decl_part(decls);
}

ast_param_decl_t *parse_param_decl(parser_t *parser)
{
    ast_ident_t *names;
    ast_type_t *type = NULL;
    ast_param_decl_t *ret, *param;
    assert(parser);

    expect(parser, TOKEN_LPAREN);
    names = parse_ident_seq(parser);
    expect(parser, TOKEN_COLON);
    if (check_type(parser)) {
        type = parse_type(parser);
    } else {
        error_unexpected(parser);
    }
    ret = param = new_ast_param_decl(names, type);
    while (eat(parser, TOKEN_SEMI)) {
        names = parse_ident_seq(parser);
        expect(parser, TOKEN_COLON);
        if (check_type(parser)) {
            type = parse_type(parser);
        } else {
            error_unexpected(parser);
        }
        param = param->next = new_ast_param_decl(names, type);
    }
    expect(parser, TOKEN_RPAREN);
    return ret;
}

ast_decl_part_t *parse_procedure_decl_part(parser_t *parser)
{
    ast_ident_t *name;
    ast_param_decl_t *params = NULL;
    ast_decl_part_t *variables = NULL;
    ast_stmt_t *stmt;
    assert(parser);

    expect(parser, TOKEN_PROCEDURE);
    name = parse_ident(parser);
    if (check(parser, TOKEN_LPAREN)) {
        params = parse_param_decl(parser);
    }
    expect(parser, TOKEN_SEMI);
    if (check(parser, TOKEN_VAR)) {
        variables = parse_variable_decl_part(parser);
    }
    stmt = parse_compound_stmt(parser);
    expect(parser, TOKEN_SEMI);
    return new_procedure_decl_part(name, params, variables, stmt);
}

ast_decl_part_t *parse_decl_part(parser_t *parser)
{
    ast_decl_part_t *ret, *cur;
    assert(parser);

    if (check(parser, TOKEN_VAR)) {
        ret = cur = parse_variable_decl_part(parser);
    } else if (check(parser, TOKEN_PROCEDURE)) {
        ret = cur = parse_procedure_decl_part(parser);
    } else {
        return NULL;
    }
    while (1) {
        if (check(parser, TOKEN_VAR)) {
            cur = cur->next = parse_variable_decl_part(parser);
        } else if (check(parser, TOKEN_PROCEDURE)) {
            cur = cur->next = parse_procedure_decl_part(parser);
        } else {
            break;
        }
    }
    return ret;
}

ast_program_t *parse_program(parser_t *parser)
{
    ast_ident_t *name;
    ast_decl_part_t *decl_part;
    ast_stmt_t *stmt;
    assert(parser);

    expect(parser, TOKEN_PROGRAM);
    name = parse_ident(parser);
    expect(parser, TOKEN_SEMI);
    decl_part = parse_decl_part(parser);
    stmt = parse_compound_stmt(parser);
    expect(parser, TOKEN_DOT);
    expect(parser, TOKEN_EOF);
    return new_program(name, decl_part, stmt);
}

ast_t *parse_source(const source_t *src)
{
    ast_program_t *program;
    parser_t parser;
    assert(src);

    parser.src = src;
    parser.storage = new_symbol_storage();
    parser.alive = 1;
    parser.error = 0;
    parser.within_loop = 0;
    cursol_init(&parser.cursol, src, src->src_ptr, src->src_size);
    bump(&parser);
    program = parse_program(&parser);
    if (parser.error) {
        delete_program(program);
        delete_symbol_storage(parser.storage);
        return NULL;
    }
    return new_ast(program, parser.storage, src);
}
