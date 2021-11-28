#ifndef AST_H
#define AST_H

#include <stddef.h>

#include "source.h"

typedef enum {
    LIT_NUMBER,
    LIT_BOOLEAN,
    LIT_STRING
} lit_kind_t;

typedef struct {
    unsigned long value;
} number_lit_t;

typedef struct {
    int value;
} boolean_lit_t;

typedef struct {
    const char *ptr;
    size_t len;
} string_lit_t;

typedef struct {
    lit_kind_t kind;

    union {
        number_lit_t number_lit;
        boolean_lit_t boolean_lit;
        string_lit_t string_lit;
    } u;
} lit_t;

lit_t *new_number_lit(unsigned long value);
lit_t *new_boolean_lit(int value);
lit_t *new_string_lit(const char *ptr, size_t len);

void delete_lit(lit_t *lit);

typedef enum {
    TYPE_CHAR,
    TYPE_INTEGER,
    TYPE_BOOLEAN,
    TYPE_ARRAY
} type_kind_t;

typedef struct impl_type type_t;
struct impl_type {
    type_kind_t kind;

    struct {
        type_t *base;
        size_t len;
    } array;
};

type_t *new_std_type(type_kind_t kind);
type_t *new_array_type(type_t *base, size_t len);

void delete_type(type_t *type);

typedef struct impl_ident ident_t;
struct impl_ident {
    ident_t *next;
    const char *ptr;
    size_t len;
};

ident_t *new_ident(const char *ptr, size_t len);

void delete_ident(ident_t *ident);

typedef struct impl_expr expr_t;

typedef enum {
    BINARY_OP_STAR,
    BINARY_OP_DIV,
    BINARY_OP_AND,
    BINARY_OP_PLUS,
    BINARY_OP_MINUS,
    BINARY_OP_OR,
    BINARY_OP_EQUAL,
    BINARY_OP_NOTEQ,
    BINARY_OP_LE,
    BINARY_OP_LEEQ,
    BINARY_OP_GR,
    BINARY_OP_GREQ
} binary_op_kind_t;

typedef struct {
    binary_op_kind_t kind;
    expr_t *lhs;
    expr_t *rhs;
} binary_expr_t;

typedef enum {
    UNARY_OP_NOT
} unary_op_kind_t;

typedef struct {
    unary_op_kind_t kind;
    expr_t *expr;
} unary_expr_t;

typedef struct {
    expr_t *expr;
} paren_expr_t;

typedef struct {
    expr_t *expr;
    type_t *type;
} cast_expr_t;

typedef struct {
    ident_t *name;
} ref_expr_t;

typedef struct {
    ident_t *name;
    expr_t *index_expr;
} array_subscript_expr_t;

typedef struct {
    lit_t *lit;
} constant_expr_t;

typedef enum {
    EXPR_BINARY_OP,
    EXPR_UNARY_OP,
    EXPR_PAREN,
    EXPR_CAST,
    EXPR_REF,
    EXPR_ARRAY_SUBSCRIPT,
    EXPR_CONSTANT,
    EXPR_EMPTY
} expr_kind_t;

void delete_expr(expr_t *expr);

struct impl_expr {
    expr_kind_t kind;
    expr_t *next;

    union {
        binary_expr_t binary_expr;
        unary_expr_t unary_expr;
        paren_expr_t paren_expr;
        cast_expr_t cast_expr;
        ref_expr_t ref_expr;
        array_subscript_expr_t array_subscript_expr;
        constant_expr_t constant_expr;
    } u;
};

expr_t *new_binary_expr(binary_op_kind_t kind, expr_t *lhs, expr_t *rhs);
expr_t *new_unary_expr(unary_op_kind_t kind, expr_t *expr);
expr_t *new_paren_expr(expr_t *expr);
expr_t *new_cast_expr(type_t *type, expr_t *expr);
expr_t *new_ref_expr(ident_t *ident);
expr_t *new_array_subscript_expr(ident_t *name, expr_t *index_expr);
expr_t *new_constant_expr(lit_t *lit);
expr_t *new_empty_expr();

void delete_expr(expr_t *expr);

typedef struct impl_stmt stmt_t;

typedef struct {
    expr_t *lhs;
    expr_t *rhs;
} assign_stmt_t;

typedef struct {
    expr_t *cond;
    stmt_t *then_stmt;
    stmt_t *else_stmt;
} if_stmt_t;

typedef struct {
    expr_t *cond;
    stmt_t *do_stmt;
} while_stmt_t;

typedef struct {
    ident_t *name;
    expr_t *args;
} call_stmt_t;

typedef struct {
    int newline;
    expr_t *args;
} read_stmt_t;

typedef struct impl_output_format output_format_t;
struct impl_output_format {
    output_format_t *next;
    expr_t *expr;
    size_t len;
};

output_format_t *new_output_format(expr_t *expr, size_t len);

void delete_output_format(output_format_t *format);

typedef struct {
    int newline;
    output_format_t *formats;
} write_stmt_t;

typedef struct impl_compound_stmt compound_stmt_t;
struct impl_compound_stmt {
    stmt_t *stmts;
};

typedef enum {
    STMT_ASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_BREAK,
    STMT_CALL,
    STMT_RETURN,
    STMT_READ,
    STMT_WRITE,
    STMT_COMPOUND,
    STMT_EMPTY
} stmt_kind_t;

struct impl_stmt {
    stmt_kind_t kind;
    stmt_t *next;

    union {
        assign_stmt_t assign_stmt;
        if_stmt_t if_stmt;
        while_stmt_t while_stmt;
        call_stmt_t call_stmt;
        read_stmt_t read_stmt;
        write_stmt_t write_stmt;
        compound_stmt_t compound_stmt;
    } u;
};

stmt_t *new_assign_stmt(expr_t *lhs, expr_t *rhs);
stmt_t *new_if_stmt(expr_t *cond, stmt_t *then_stmt, stmt_t *else_stmt);
stmt_t *new_while_stmt(expr_t *cond, stmt_t *do_stmt);
stmt_t *new_break_stmt();
stmt_t *new_call_stmt(ident_t *name, expr_t *args);
stmt_t *new_return_stmt();
stmt_t *new_read_stmt(int newline, expr_t *args);
stmt_t *new_write_stmt(int newline, output_format_t *formats);
stmt_t *new_compound_stmt(stmt_t *stmts);
stmt_t *new_empty_stmt();

void delete_stmt(stmt_t *stmt);

typedef struct impl_decl_part decl_part_t;

typedef struct impl_variable_decl variable_decl_t;
struct impl_variable_decl {
    variable_decl_t *next;
    ident_t *names;
    type_t *type;
};

typedef struct impl_variable_decl_part variable_decl_part_t;
struct impl_variable_decl_part {
    variable_decl_t *decls;
};

variable_decl_t *new_variable_decl(ident_t *names, type_t *type);

void delete_variable_decl(variable_decl_t *decl);

typedef struct impl_params params_t;
struct impl_params {
    params_t *next;
    ident_t *names;
    type_t *type;
};

params_t *new_params(ident_t *names, type_t *type);

void delete_params(params_t *params);

typedef struct impl_procedure_decl_part procedure_decl_part_t;
struct impl_procedure_decl_part {
    ident_t *name;
    params_t *params;
    decl_part_t *variables;
    stmt_t *stmt;
};

typedef enum {
    DECL_PART_VARIABLE,
    DECL_PART_PROCEDURE
} decl_part_kind_t;

struct impl_decl_part {
    decl_part_kind_t kind;
    decl_part_t *next;

    union {
        variable_decl_part_t variable_decl_part;
        procedure_decl_part_t procedure_decl_part;
    } u;
};

decl_part_t *new_variable_decl_part(variable_decl_t *decls);
decl_part_t *new_procedure_decl_part(ident_t *name, params_t *params, decl_part_t *variables, stmt_t *stmt);

void delete_decl_part(decl_part_t *decl);

typedef struct {
    ident_t *name;
    decl_part_t *decl_part;
    stmt_t *stmt;
} program_t;

program_t *new_program(ident_t *name, decl_part_t *decl_part, stmt_t *stmt);

void delete_program(program_t *program);

typedef struct {
    program_t *program;
    const source_t *source;
} ast_t;

ast_t *new_ast(program_t *program, const source_t *source);

void delete_ast(ast_t *ast);

#endif
