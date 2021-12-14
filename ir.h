#ifndef IR_H
#define IR_H

#include "mppl.h"

typedef enum {
    IR_TYPE_PROGRAM,
    IR_TYPE_PROCEDURE,
    IR_TYPE_ARRAY,
    IR_TYPE_INTEGER,
    IR_TYPE_BOOLEAN,
    IR_TYPE_CHAR
} ir_ast_type_kind_t;

typedef struct impl_ir_type ir_type_t;
struct impl_ir_type {
    ir_ast_type_kind_t kind;
    ir_type_t *next;

    union {
        struct {
            ir_type_t *arg_types;
        } procedure_type;
        struct {
            size_t size;
        } array_type;
    } u;
};

typedef enum {
    IR_LOCAL_NORMAL,
    IR_LOCAL_TEMP,
    IR_LOCAL_REF
} ir_local_kind_t;

typedef struct {
    ir_local_kind_t kind;
    const symbol_t *key;
} ir_local_t;

typedef enum {
    IR_PLACE_ACCESS_NORMAL,
    IR_PLACE_ACCESS_INDEX
} ir_place_access_kind_t;

typedef struct {
    ir_place_access_kind_t kind;

    union {
        struct {
            size_t index;
        } index_place_access;
    } u;
} ir_place_access_t;

typedef struct {
    ir_local_t *local;
    ir_place_access_t *place_access;
} ir_place_t;

typedef enum {
    IR_CONSTANT_NUMBER,
    IR_CONSTANT_BOOLEAN,
    IR_CONSTANT_CHAR,
    IR_CONSTANT_STRING
} ir_constant_kind_t;

typedef struct {
    ir_constant_kind_t kind;

    union {
        struct {
            unsigned long value;
        } number_constant;
        struct {
            int value;
        } boolean_constant;
        struct {
            int value;
        } char_constant;
        struct {
            const symbol_t *value;
        } string_constant;
    } u;
} ir_constant_t;

typedef enum {
    IR_OPERAND_PLACE,
    IR_OPERAND_CONSTANT
} ir_operand_kind_t;

typedef struct {
    ir_operand_kind_t kind;

    union {
        struct {
            ir_place_t *place;
        } place_operand;
        struct {
            ir_constant_t *constant;
        } constant_operand;
    } u;
} ir_operand_t;

typedef enum {
    IR_RVALUE_USE,
    IR_RVALUE_BINARY_OP,
    IR_RVALUE_UNARY_OP,
    IR_RVALUE_CAST
} ir_rvalue_kind_t;

typedef struct impl_ir_rvalue ir_rvalue_t;
struct impl_ir_rvalue {
    ir_rvalue_kind_t kind;
    ir_rvalue_t *next;

    union {
        struct {
            ir_operand_t *place;
        } use_rvalue;
        struct {
            ast_binary_op_kind_t kind;
            ir_operand_t *lhs;
            ir_operand_t *rhs;
        } binary_op_rvalue;
        struct {
            ast_unary_op_kind_t kind;
            ir_operand_t *value;
        } unary_op_rvalue;
        struct {
            const ir_type_t *type;
            ir_operand_t *value;
        } cast_rvalue;
    } u;
};

typedef struct impl_ir_stmt ir_stmt_t;
typedef struct impl_ir_block ir_block_t;

struct impl_ir_stmt {
    ir_place_t *lhs;
    ir_rvalue_t *rhs;
    ir_stmt_t *next;
};

typedef enum {
    IR_TERMN_GOTO,
    IR_TERMN_IF,
    IR_TERMN_CALL,
    IR_TERMN_RETURN
} ir_termn_kind_t;

typedef struct {
    ir_termn_kind_t kind;

    union {
        struct {
            ir_block_t *next;
        } goto_termn;
        struct {
            ir_operand_t *cond;
            ir_block_t *then;
            ir_block_t *els;
        } if_termn;
        struct {
            ir_place_t *func;
            ir_rvalue_t *args;
            ir_block_t *dest;
        } call_termn;
    } u;
} ir_termn_t;

struct impl_ir_block {
    ir_stmt_t *stmt;
    ir_termn_t *termn;
};

typedef struct {
    hash_table_t *items;
    hash_table_t *refs;
    ir_block_t *inner;
} ir_body_t;

typedef enum {
    IR_ITEM_PROGRAM,
    IR_ITEM_PROCEDURE,
    IR_ITEM_VAR,
    IR_ITEM_ARG_VAR,
    IR_ITEM_LOCAL_VAR
} ir_item_kind_t;

typedef struct {
    ir_item_kind_t kind;
    const ir_type_t *type;
    const symbol_t *symbol;
    ir_body_t *body;
    const symbol_t *next_key;
} ir_item_t;

typedef struct {
    ir_item_t *program;
} ir_t;

#endif
