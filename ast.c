#include <assert.h>
#include <stdlib.h>

#include "ast.h"
#include "util.h"

static lit_t *new_lit(lit_kind_t kind)
{
    lit_t *ret = new(lit_t);
    ret->kind = kind;
    return ret;
}

lit_t *new_number_lit(unsigned long value)
{
    lit_t *ret = new_lit(LIT_NUMBER);
    ret->u.number_lit.value = value;
    if (value > 32767) {
        delete_lit(ret);
        return NULL;
    }
    return ret;
}

lit_t *new_boolean_lit(int value)
{
    lit_t *ret = new_lit(LIT_BOOLEAN);
    ret->u.boolean_lit.value = value;
    return ret;
}

lit_t *new_string_lit(const char *ptr, size_t len)
{
    lit_t *ret = new_lit(LIT_STRING);
    ret->u.string_lit.ptr = ptr;
    ret->u.string_lit.len = len;
    if (ptr == NULL) {
        delete_lit(ret);
        return NULL;
    }
    return ret;
}

void delete_lit(lit_t *lit)
{
    free(lit);
}

type_t *new_std_type(type_kind_t kind)
{
    type_t *ret = new(type_t);
    ret->kind = kind;
    if (kind == TYPE_ARRAY) {
        delete_type(ret);
        return NULL;
    }
    return ret;
}

type_t *new_array_type(type_t *base, size_t len)
{
    type_t *ret = new(type_t);
    ret->kind = TYPE_ARRAY;
    ret->array.base = base;
    ret->array.len = len;
    if (base == NULL || base->kind == TYPE_ARRAY) {
        delete_type(ret);
        return NULL;
    }
    return ret;
}

void delete_type(type_t *type)
{
    if (type == NULL) {
        return;
    }
    if (type->kind == TYPE_ARRAY) {
        delete_type(type->array.base);
    }
    free(type);
}

ident_t *new_ident(const char *ptr, size_t len)
{
    ident_t *ret = new(ident_t);
    ret->next = NULL;
    ret->ptr = ptr;
    ret->len = len;
    if (ptr == NULL) {
        delete_ident(ret);
        return NULL;
    }
    return ret;
}

void delete_ident(ident_t *ident)
{
    free(ident);
}

static expr_t *new_expr(expr_kind_t kind)
{
    expr_t *ret = new(expr_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

expr_t *new_binary_expr(binary_op_kind_t kind, expr_t *lhs, expr_t *rhs)
{
    expr_t *ret = new_expr(EXPR_BINARY_OP);
    ret->u.binary_expr.kind = kind;
    ret->u.binary_expr.lhs = lhs;
    ret->u.binary_expr.rhs = rhs;
    if (lhs == NULL || rhs == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_unary_expr(unary_op_kind_t kind, expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_UNARY_OP);
    ret->u.unary_expr.kind = kind;
    ret->u.unary_expr.expr = expr;
    if (expr == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_paren_expr(expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_PAREN);
    ret->u.paren_expr.expr = expr;
    if (expr == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_cast_expr(type_t *type, expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_CAST);
    ret->u.cast_expr.type = type;
    ret->u.cast_expr.expr = expr;
    if (type == NULL || expr == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_ref_expr(ident_t *ident)
{
    expr_t *ret = new_expr(EXPR_REF);
    ret->u.ref_expr.name = ident;
    if (ident == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_array_subscript_expr(ident_t *name, expr_t *index_expr)
{
    expr_t *ret = new_expr(EXPR_ARRAY_SUBSCRIPT);
    ret->u.array_subscript_expr.name = name;
    ret->u.array_subscript_expr.index_expr = index_expr;
    if (name == NULL || index_expr == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_constant_expr(lit_t *lit)
{
    expr_t *ret = new_expr(EXPR_CONSTANT);
    ret->u.constant_expr.lit = lit;
    if (lit == NULL) {
        delete_expr(ret);
        return NULL;
    }
    return ret;
}

expr_t *new_empty_expr()
{
    return new_expr(EXPR_EMPTY);
}

void delete_expr(expr_t *expr)
{
    if (expr == NULL) {
        return;
    }
    switch (expr->kind) {
    case EXPR_BINARY_OP:
        delete_expr(expr->u.binary_expr.lhs);
        delete_expr(expr->u.binary_expr.rhs);
        break;
    case EXPR_UNARY_OP:
        delete_expr(expr->u.unary_expr.expr);
        break;
    case EXPR_PAREN:
        delete_expr(expr->u.paren_expr.expr);
        break;
    case EXPR_CAST:
        delete_type(expr->u.cast_expr.type);
        delete_expr(expr->u.cast_expr.expr);
        break;
    case EXPR_REF:
        delete_ident(expr->u.ref_expr.name);
        break;
    case EXPR_ARRAY_SUBSCRIPT:
        delete_ident(expr->u.array_subscript_expr.name);
        delete_expr(expr->u.array_subscript_expr.index_expr);
        break;
    case EXPR_CONSTANT:
        delete_lit(expr->u.constant_expr.lit);
        break;
    }
    free(expr);
}
