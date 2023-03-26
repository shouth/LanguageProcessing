#ifndef AST_H
#define AST_H

#include "source.h"

/**********     ast literal     **********/

typedef enum {
  AST_LIT_KIND_NUMBER,
  AST_LIT_KIND_BOOLEAN,
  AST_LIT_KIND_STRING
} ast_lit_kind_t;

typedef struct ast__lit_s         ast_lit_t;
typedef struct ast__lit_number_s  ast_lit_number_t;
typedef struct ast__lit_boolean_s ast_lit_boolean_t;
typedef struct ast__lit_string_s  ast_lit_string_t;

struct ast__lit_number_s {
  const symbol_t *symbol;
  unsigned long   value;
};

struct ast__lit_boolean_s {
  int value;
};

struct ast__lit_string_s {
  const symbol_t *symbol;
  size_t          str_len;
};

struct ast__lit_s {
  union {
    ast_lit_number_t  number;
    ast_lit_boolean_t boolean;
    ast_lit_string_t  string;
  } lit;

  ast_lit_kind_t kind;
  region_t       region;
};

void delete_ast_lit(ast_lit_t *lit);

/**********     ast identifier     **********/

typedef struct ast__ident_s ast_ident_t;

struct ast__ident_s {
  const symbol_t *symbol;
  region_t        region;
  ast_ident_t    *next;
};

void delete_ast_ident(ast_ident_t *ident);

/**********     ast type     **********/

typedef enum {
  AST_TYPE_KIND_CHAR,
  AST_TYPE_KIND_INTEGER,
  AST_TYPE_KIND_BOOLEAN,
  AST_TYPE_KIND_ARRAY
} ast_type_kind_t;

typedef struct ast__type_s       ast_type_t;
typedef struct ast__type_array_s ast_type_array_t;

struct ast__type_array_s {
  ast_type_t *base;
  ast_lit_t  *size;
};

struct ast__type_s {
  union {
    ast_type_array_t array;
  } type;

  ast_type_kind_t kind;
  region_t        region;
};

void delete_ast_type(ast_type_t *type);

/**********     ast expression     **********/

typedef enum {
  AST_EXPR_DECL_REF,
  AST_EXPR_ARRAY_SUBSCRIPT,
  AST_EXPR_BINARY,
  AST_EXPR_UNARY,
  AST_EXPR_PAREN,
  AST_EXPR_CAST,
  AST_EXPR_CONSTANT,
  AST_EXPR_EMPTY
} ast_expr_kind_t;

typedef enum {
  AST_EXPR_BINARY_KIND_STAR,
  AST_EXPR_BINARY_KIND_DIV,
  AST_EXPR_BINARY_KIND_AND,
  AST_EXPR_BINARY_KIND_PLUS,
  AST_EXPR_BINARY_KIND_MINUS,
  AST_EXPR_BINARY_KIND_OR,
  AST_EXPR_BINARY_KIND_EQUAL,
  AST_EXPR_BINARY_KIND_NOTEQ,
  AST_EXPR_BINARY_KIND_LE,
  AST_EXPR_BINARY_KIND_LEEQ,
  AST_EXPR_BINARY_KIND_GR,
  AST_EXPR_BINARY_KIND_GREQ
} ast_expr_binary_kind_t;

typedef enum {
  AST_UNARY_OP_NOT
} ast_expr_unary_kind_t;

typedef struct ast__expr_s                 ast_expr_t;
typedef struct ast__expr_decl_ref_s        ast_expr_decl_ref_t;
typedef struct ast__expr_array_subscript_s ast_expr_array_subscript_t;
typedef struct ast__expr_binary_s          ast_expr_binary_t;
typedef struct ast__expr_unary_s           ast_expr_unary_t;
typedef struct ast__expr_paren_s           ast_expr_paren_t;
typedef struct ast__expr_cast_s            ast_expr_cast_t;
typedef struct ast__expr_constant_s        ast_expr_constant_t;
typedef struct ast__expr_empty_s           ast_expr_empty_t;

struct ast__expr_decl_ref_s {
  ast_ident_t *decl;
};

struct ast__expr_array_subscript_s {
  ast_ident_t *decl;
  ast_expr_t  *subscript;
};

struct ast__expr_binary_s {
  ast_expr_binary_kind_t kind;
  region_t               op_region;
  ast_expr_t            *lhs;
  ast_expr_t            *rhs;
};

struct ast__expr_unary_s {
  ast_expr_unary_kind_t kind;
  region_t              op_region;
  ast_expr_t           *expr;
};

struct ast__expr_paren_s {
  ast_expr_t *inner;
};

struct ast__expr_cast_s {
  ast_expr_t *cast;
  ast_type_t *type;
};

struct ast__expr_constant_s {
  ast_lit_t *lit;
};

struct ast__expr_s {
  union {
    ast_expr_decl_ref_t        decl_ref;
    ast_expr_array_subscript_t array_subscript;
    ast_expr_binary_t          binary;
    ast_expr_unary_t           unary;
    ast_expr_paren_t           paren;
    ast_expr_cast_t            cast;
    ast_expr_constant_t        constant;
  } expr;

  ast_expr_kind_t kind;
  region_t        region;
  ast_expr_t     *next;
};

const char *ast_binop_str(ast_expr_binary_kind_t kind);
void        delete_ast_expr(ast_expr_t *expr);

/**********     ast output format     **********/

typedef struct ast_output_format_s ast_output_format_t;

struct ast_output_format_s {
  ast_output_format_t *next;
  ast_expr_t          *expr;
  ast_lit_t           *len;
};

void delete_ast_output_format(ast_output_format_t *format);

/**********     ast statement     **********/

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

typedef struct ast__stmt_s          ast_stmt_t;
typedef struct ast__stmt_assign_s   ast_stmt_assign_t;
typedef struct ast__stmt_if_s       ast_stmt_if_t;
typedef struct ast__stmt_while_s    ast_stmt_while_t;
typedef struct ast__stmt_break_s    ast_stmt_break_t;
typedef struct ast__stmt_call_s     ast_stmt_call_t;
typedef struct ast__stmt_return_s   ast_stmt_return_t;
typedef struct ast__stmt_read_s     ast_stmt_read_t;
typedef struct ast__stmt_write_s    ast_stmt_write_t;
typedef struct ast__stmt_compound_s ast_stmt_compound_t;

struct ast__stmt_assign_s {
  ast_expr_t *lhs;
  ast_expr_t *rhs;
  region_t    op_region;
};

struct ast__stmt_if_s {
  ast_expr_t *cond;
  ast_stmt_t *then_stmt;
  ast_stmt_t *else_stmt;
};

struct ast__stmt_while_s {
  ast_expr_t *cond;
  ast_stmt_t *do_stmt;
};

struct ast__stmt_call_s {
  ast_ident_t *name;
  ast_expr_t  *args;
};

struct ast__stmt_read_s {
  int         newline;
  ast_expr_t *args;
};

struct ast__stmt_write_s {
  int                  newline;
  ast_output_format_t *formats;
};

struct ast__stmt_compound_s {
  ast_stmt_t *stmts;
};

struct ast__stmt_s {
  union {
    ast_stmt_assign_t   assign;
    ast_stmt_if_t       if_;
    ast_stmt_while_t    while_;
    ast_stmt_call_t     call;
    ast_stmt_read_t     read;
    ast_stmt_write_t    write;
    ast_stmt_compound_t compound;
  } stmt;

  ast_stmt_kind_t kind;
  ast_stmt_t     *next;
};

void delete_ast_stmt(ast_stmt_t *stmt);

/**********     ast declaration     **********/

typedef struct ast__decl_variable_s ast_decl_variable_t;
typedef struct ast__decl_param_s    ast_decl_param_t;

struct ast__decl_variable_s {
  ast_ident_t         *names;
  ast_type_t          *type;
  ast_decl_variable_t *next;
};

struct ast__decl_param_s {
  ast_decl_param_t *next;
  ast_ident_t      *names;
  ast_type_t       *type;
};

void delete_ast_variable_decl(ast_decl_variable_t *decl);
void delete_ast_param_decl(ast_decl_param_t *params);

/**********     ast declaration part     **********/

typedef enum {
  AST_DECL_PART_VARIABLE,
  AST_DECL_PART_PROCEDURE
} ast_decl_part_kind_t;

typedef struct ast__decl_part_s           ast_decl_part_t;
typedef struct ast__decl_part_variable_s  ast_decl_part_variable_t;
typedef struct ast__decl_part_procedure_s ast_decl_part_procedure_t;

struct ast__decl_part_variable_s {
  ast_decl_variable_t *decls;
};

struct ast__decl_part_procedure_s {
  ast_ident_t      *name;
  ast_decl_param_t *params;
  ast_decl_part_t  *variables;
  ast_stmt_t       *stmt;
};

struct ast__decl_part_s {
  union {
    ast_decl_part_variable_t  variable;
    ast_decl_part_procedure_t procedure;
  } decl_part;

  ast_decl_part_kind_t kind;
  ast_decl_part_t     *next;
};

void delete_decl_part(ast_decl_part_t *decl);

/**********     ast program     **********/

typedef struct ast__program_s ast_program_t;

struct ast__program_s {
  ast_ident_t     *name;
  ast_decl_part_t *decl_part;
  ast_stmt_t      *stmt;
};

ast_program_t *new_program(ast_ident_t *name, ast_decl_part_t *decl_part, ast_stmt_t *stmt);
void           delete_program(ast_program_t *program);

/**********     ast     **********/

typedef struct {
  ast_program_t    *program;
  symbol_storage_t *storage;
  const source_t   *source;
} ast_t;

ast_t *new_ast(ast_program_t *program, symbol_storage_t *storage, const source_t *source);
void   delete_ast(ast_t *ast);

#endif
