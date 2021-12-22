#ifndef AST_H
#define AST_H

#include "mppl.h"

typedef enum {
    AST_LIT_NUMBER,
    AST_LIT_BOOLEAN,
    AST_LIT_STRING
} ast_lit_kind_t;

typedef struct {
    symbol_t symbol;
    unsigned long value;
} ast_number_lit_t;

typedef struct {
    int value;
} ast_boolean_lit_t;

typedef struct {
    symbol_t symbol;
    size_t str_len;
} ast_string_lit_t;

typedef struct {
    ast_lit_kind_t kind;
    region_t region;

    union {
        ast_number_lit_t number_lit;
        ast_boolean_lit_t boolean_lit;
        ast_string_lit_t string_lit;
    } u;
} ast_lit_t;

ast_lit_t *new_ast_number_lit(symbol_t symbol, unsigned long value, region_t region);
ast_lit_t *new_ast_boolean_lit(int value, region_t region);
ast_lit_t *new_ast_string_lit(symbol_t symbol, size_t str_len, region_t region);
void delete_ast_lit(ast_lit_t *lit);

typedef struct impl_ast_ident ast_ident_t;
struct impl_ast_ident {
    symbol_t symbol;
    region_t region;
    ast_ident_t *next;
};

ast_ident_t *new_ast_ident(symbol_t symbol, region_t region);
void delete_ast_ident(ast_ident_t *ident);

typedef enum {
    AST_TYPE_CHAR,
    AST_TYPE_INTEGER,
    AST_TYPE_BOOLEAN,
    AST_TYPE_ARRAY
} ast_type_kind_t;

typedef struct impl_ast_type ast_type_t;

typedef struct {
    ast_type_t *base;
    ast_lit_t *size;
} ast_array_type_t;

struct impl_ast_type {
    ast_type_kind_t kind;
    region_t region;

    union {
        ast_array_type_t array_type;
    } u;
};

ast_type_t *new_ast_std_type(ast_type_kind_t kind, region_t region);
ast_type_t *new_ast_array_type(ast_type_t *base, ast_lit_t *size, region_t region);
void delete_ast_type(ast_type_t *type);

typedef struct impl_ast_expr ast_expr_t;

typedef enum {
    AST_BINARY_OP_STAR,
    AST_BINARY_OP_DIV,
    AST_BINARY_OP_AND,
    AST_BINARY_OP_PLUS,
    AST_BINARY_OP_MINUS,
    AST_BINARY_OP_OR,
    AST_BINARY_OP_EQUAL,
    AST_BINARY_OP_NOTEQ,
    AST_BINARY_OP_LE,
    AST_BINARY_OP_LEEQ,
    AST_BINARY_OP_GR,
    AST_BINARY_OP_GREQ
} ast_binary_op_kind_t;

typedef struct {
    ast_binary_op_kind_t kind;
    ast_expr_t *lhs;
    ast_expr_t *rhs;
} ast_binary_expr_t;

typedef enum {
    AST_UNARY_OP_NOT
} ast_unary_op_kind_t;

typedef struct {
    ast_unary_op_kind_t kind;
    ast_expr_t *expr;
} ast_unary_expr_t;

typedef struct {
    ast_expr_t *expr;
} ast_paren_expr_t;

typedef struct {
    ast_expr_t *expr;
    ast_type_t *type;
} ast_cast_expr_t;

typedef struct {
    ast_lit_t *lit;
} ast_constant_expr_t;

typedef struct {
    ast_ident_t *decl;
} ast_decl_ref_expr_t;

typedef struct {
    ast_ident_t *decl;
    ast_expr_t *expr;
} ast_array_subscript_expr_t;

typedef enum {
    AST_EXPR_DECL_REF,
    AST_EXPR_ARRAY_SUBSCRIPT,
    AST_EXPR_BINARY_OP,
    AST_EXPR_UNARY_OP,
    AST_EXPR_PAREN,
    AST_EXPR_CAST,
    AST_EXPR_CONSTANT,
    AST_EXPR_EMPTY
} ast_expr_kind_t;

struct impl_ast_expr {
    ast_expr_kind_t kind;
    ast_expr_t *next;

    union {
        ast_binary_expr_t binary_expr;
        ast_unary_expr_t unary_expr;
        ast_paren_expr_t paren_expr;
        ast_cast_expr_t cast_expr;
        ast_decl_ref_expr_t decl_ref_expr;
        ast_array_subscript_expr_t array_subscript_expr;
        ast_constant_expr_t constant_expr;
    } u;
};

ast_expr_t *new_ast_binary_expr(ast_binary_op_kind_t kind, ast_expr_t *lhs, ast_expr_t *rhs);
ast_expr_t *new_ast_unary_expr(ast_unary_op_kind_t kind, ast_expr_t *expr);
ast_expr_t *new_ast_paren_expr(ast_expr_t *expr);
ast_expr_t *new_ast_cast_expr(ast_type_t *type, ast_expr_t *expr);
ast_expr_t *new_ast_decl_ref_expr(ast_ident_t *decl);
ast_expr_t *new_ast_array_subscript_expr(ast_ident_t *decl, ast_expr_t *expr);
ast_expr_t *new_ast_constant_expr(ast_lit_t *lit);
ast_expr_t *new_ast_empty_expr();
void delete_ast_expr(ast_expr_t *expr);

typedef struct impl_ast_stmt ast_stmt_t;

typedef struct {
    ast_expr_t *lhs;
    ast_expr_t *rhs;
} ast_assign_stmt_t;

typedef struct {
    ast_expr_t *cond;
    ast_stmt_t *then_stmt;
    ast_stmt_t *else_stmt;
} ast_if_stmt_t;

typedef struct {
    ast_expr_t *cond;
    ast_stmt_t *do_stmt;
} ast_while_stmt_t;

typedef struct {
    ast_ident_t *name;
    ast_expr_t *args;
} ast_call_stmt_t;

typedef struct {
    int newline;
    ast_expr_t *args;
} ast_read_stmt_t;

typedef struct impl_ast_output_format ast_output_format_t;
struct impl_ast_output_format {
    ast_output_format_t *next;
    ast_expr_t *expr;
    ast_lit_t *len;
};

ast_output_format_t *new_ast_output_format(ast_expr_t *expr, ast_lit_t *len);
void delete_ast_output_format(ast_output_format_t *format);

typedef struct {
    int newline;
    ast_output_format_t *formats;
} ast_write_stmt_t;

typedef struct impl_ast_compound_stmt ast_compound_stmt_t;
struct impl_ast_compound_stmt {
    ast_stmt_t *stmts;
};

typedef enum {
    AST_STMT_ASSIGN,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_BREAK,
    AST_STMT_CALL,
    AST_STMT_RETURN,
    AST_STMT_READ,
    AST_STMT_WRITE,
    AST_STMT_COMPOUND,
    AST_STMT_EMPTY
} ast_stmt_kind_t;

struct impl_ast_stmt {
    ast_stmt_kind_t kind;
    ast_stmt_t *next;

    union {
        ast_assign_stmt_t assign_stmt;
        ast_if_stmt_t if_stmt;
        ast_while_stmt_t while_stmt;
        ast_call_stmt_t call_stmt;
        ast_read_stmt_t read_stmt;
        ast_write_stmt_t write_stmt;
        ast_compound_stmt_t compound_stmt;
    } u;
};

ast_stmt_t *new_ast_assign_stmt(ast_expr_t *lhs, ast_expr_t *rhs);
ast_stmt_t *new_ast_if_stmt(ast_expr_t *cond, ast_stmt_t *then_stmt, ast_stmt_t *else_stmt);
ast_stmt_t *new_ast_while_stmt(ast_expr_t *cond, ast_stmt_t *do_stmt);
ast_stmt_t *new_ast_break_stmt();
ast_stmt_t *new_ast_call_stmt(ast_ident_t *name, ast_expr_t *args);
ast_stmt_t *new_ast_return_stmt();
ast_stmt_t *new_ast_read_stmt(int newline, ast_expr_t *args);
ast_stmt_t *new_ast_write_stmt(int newline, ast_output_format_t *formats);
ast_stmt_t *new_ast_compound_stmt(ast_stmt_t *stmts);
ast_stmt_t *new_ast_empty_stmt();
void delete_ast_stmt(ast_stmt_t *stmt);

typedef struct impl_ast_decl_part ast_decl_part_t;

typedef struct impl_ast_variable_decl ast_variable_decl_t;
struct impl_ast_variable_decl {
    ast_variable_decl_t *next;
    ast_ident_t *names;
    ast_type_t *type;
};

typedef struct impl_ast_variable_decl_part ast_variable_decl_part_t;
struct impl_ast_variable_decl_part {
    ast_variable_decl_t *decls;
};

ast_variable_decl_t *new_ast_variable_decl(ast_ident_t *names, ast_type_t *type);
void delete_ast_variable_decl(ast_variable_decl_t *decl);

typedef struct impl_ast_param_decl ast_param_decl_t;
struct impl_ast_param_decl {
    ast_param_decl_t *next;
    ast_ident_t *names;
    ast_type_t *type;
};

ast_param_decl_t *new_ast_param_decl(ast_ident_t *names, ast_type_t *type);
void delete_ast_param_decl(ast_param_decl_t *params);

typedef struct impl_ast_procedure_decl_part ast_procedure_decl_part_t;
struct impl_ast_procedure_decl_part {
    ast_ident_t *name;
    ast_param_decl_t *params;
    ast_decl_part_t *variables;
    ast_stmt_t *stmt;
};

typedef enum {
    AST_DECL_PART_VARIABLE,
    AST_DECL_PART_PROCEDURE
} ast_decl_part_kind_t;

struct impl_ast_decl_part {
    ast_decl_part_kind_t kind;
    ast_decl_part_t *next;

    union {
        ast_variable_decl_part_t variable_decl_part;
        ast_procedure_decl_part_t procedure_decl_part;
    } u;
};

ast_decl_part_t *new_variable_decl_part(ast_variable_decl_t *decls);
ast_decl_part_t *new_procedure_decl_part(ast_ident_t *name, ast_param_decl_t *params, ast_decl_part_t *variables, ast_stmt_t *stmt);
void delete_decl_part(ast_decl_part_t *decl);

typedef struct {
    ast_ident_t *name;
    ast_decl_part_t *decl_part;
    ast_stmt_t *stmt;
} ast_program_t;

ast_program_t *new_program(ast_ident_t *name, ast_decl_part_t *decl_part, ast_stmt_t *stmt);
void delete_program(ast_program_t *program);

typedef struct {
    ast_program_t *program;
    symbol_storage_t *storage;
    const source_t *source;
} ast_t;

ast_t *new_ast(ast_program_t *program, symbol_storage_t *storage, const source_t *source);
void delete_ast(ast_t *ast);

#endif
