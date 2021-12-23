#ifndef IR_H
#define IR_H

#include "mppl.h"

typedef uintptr_t ir_type_t;

typedef enum {
    IR_TYPE_PROGRAM,
    IR_TYPE_PROCEDURE,
    IR_TYPE_ARRAY,
    IR_TYPE_INTEGER,
    IR_TYPE_BOOLEAN,
    IR_TYPE_CHAR
} ir_type_kind_t;

int ir_type_is_kind(ir_type_t type, ir_type_kind_t kind);
int ir_type_is_std(ir_type_t type);

typedef struct impl_ir_type_instance ir_type_instance_t;
struct impl_ir_type_instance {
    ir_type_kind_t kind;
    ir_type_instance_t *next;

    union {
        struct {
            ir_type_instance_t *param_types;
        } procedure_type;
        struct {
            ir_type_instance_t *base_type;
            size_t size;
        } array_type;
        ir_type_t ref;
    } u;
};

ir_type_instance_t *new_ir_program_type_instance();
ir_type_instance_t *new_ir_procedure_type_instance(ir_type_instance_t *param_types);
ir_type_instance_t *new_ir_array_type_instance(ir_type_instance_t *base_type, size_t size);
ir_type_instance_t *new_ir_integer_type_instance();
ir_type_instance_t *new_ir_boolean_type_instance();
ir_type_instance_t *new_ir_char_type_instance();
ir_type_instance_t *new_ir_type_ref(ir_type_t type);
void delete_ir_type_instance(ir_type_instance_t *type);

typedef struct {
    hash_table_t *table;
    ir_type_t std_integer;
    ir_type_t std_char;
    ir_type_t std_boolean;
} ir_type_storage_t;

ir_type_storage_t *new_ir_type_storage();
void delete_ir_type_storage(ir_type_storage_t *storage);
ir_type_t ir_type_intern(ir_type_storage_t *storage, ir_type_instance_t *instance);
const ir_type_instance_t *ir_type_get_instance(ir_type_t type);

typedef struct impl_ir_item ir_item_t;

typedef enum {
    IR_LOCAL_NORMAL,
    IR_LOCAL_TEMP,
    IR_LOCAL_REF
} ir_local_kind_t;

typedef struct impl_ir_local ir_local_t;
struct impl_ir_local {
    ir_local_kind_t kind;
    ir_local_t *next;

    union {
        struct {
            ir_type_t type;
        } temp;
        struct {
            const ir_item_t *item;
        } normal;
        struct {
            const ir_item_t *item;
        } ref;
    } u;
};

ir_local_t *new_ir_normal_local(const ir_item_t *item);
ir_local_t *new_ir_temp_local(ir_type_t type);
ir_local_t *new_ir_ref_local(const ir_item_t *item);
ir_type_t ir_local_type(const ir_local_t *local);
void delete_ir_local(ir_local_t *local);

typedef struct impl_ir_operand ir_operand_t;

typedef enum {
    IR_PLACE_ACCESS_NORMAL,
    IR_PLACE_ACCESS_INDEX
} ir_place_access_kind_t;

typedef struct {
    ir_place_access_kind_t kind;

    union {
        struct {
            ir_operand_t *index;
        } index_place_access;
    } u;
} ir_place_access_t;

ir_place_access_t *new_ir_normal_place_access();
ir_place_access_t *new_ir_index_place_access(ir_operand_t *index);
void delete_ir_place_access(ir_place_access_t *place_access);

typedef struct impl_ir_place ir_place_t;
struct impl_ir_place {
    ir_local_t *local;
    ir_place_access_t *place_access;
    ir_place_t *next;
};

ir_place_t *new_ir_place(ir_local_t *local, ir_place_access_t *place_access);
ir_type_t ir_place_type(ir_place_t *place);
void delete_ir_place(ir_place_t *place);

typedef enum {
    IR_CONSTANT_NUMBER,
    IR_CONSTANT_BOOLEAN,
    IR_CONSTANT_CHAR,
    IR_CONSTANT_STRING
} ir_constant_kind_t;

typedef struct {
    ir_constant_kind_t kind;
    ir_type_t type;

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
            symbol_t value;
        } string_constant;
    } u;
} ir_constant_t;

ir_constant_t *new_ir_number_constant(ir_type_t type, unsigned long value);
ir_constant_t *new_ir_boolean_constant(ir_type_t type, int value);
ir_constant_t *new_ir_char_constant(ir_type_t type, int value);
ir_constant_t *new_ir_string_constant(ir_type_t type, symbol_t value);
ir_type_t ir_constant_type(const ir_constant_t *constant);
void delete_ir_constant(ir_constant_t *constant);

typedef enum {
    IR_OPERAND_PLACE,
    IR_OPERAND_CONSTANT
} ir_operand_kind_t;

struct impl_ir_operand {
    ir_operand_kind_t kind;

    union {
        struct {
            ir_place_t *place;
        } place_operand;
        struct {
            ir_constant_t *constant;
        } constant_operand;
    } u;
};

ir_operand_t *new_ir_place_operand(ir_place_t *place);
ir_operand_t *new_ir_constant_operand(ir_constant_t *constant);
ir_type_t ir_operand_type(ir_operand_t *operand);
void delete_ir_operand(ir_operand_t *operand);

typedef enum {
    IR_RVALUE_USE,
    IR_RVALUE_BINARY_OP,
    IR_RVALUE_UNARY_OP,
    IR_RVALUE_CAST
} ir_rvalue_kind_t;

typedef struct {
    ir_rvalue_kind_t kind;

    union {
        struct {
            ir_operand_t *operand;
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
            ir_type_t type;
            ir_operand_t *value;
        } cast_rvalue;
    } u;
} ir_rvalue_t;

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *operand);
ir_rvalue_t *new_ir_binary_op_rvalue(ast_binary_op_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs);
ir_rvalue_t *new_ir_unary_op_rvalue(ast_unary_op_kind_t kind, ir_operand_t *value);
ir_rvalue_t *new_ir_cast_rvalue(ir_type_t type, ir_operand_t *value);
void delete_ir_rvalue(ir_rvalue_t *rvalue);

typedef struct impl_ir_stmt ir_stmt_t;
typedef struct impl_ir_block ir_block_t;

typedef enum {
    IR_STMT_ASSIGN,
    IR_STMT_CALL,
    IR_STMT_READ,
    IR_STMT_WRITE
} ir_stmt_kind_t;

struct impl_ir_stmt {
    ir_stmt_kind_t kind;
    ir_stmt_t *next;

    union {
        struct {
            ir_place_t *lhs;
            ir_rvalue_t *rhs;
        } assign_stmt;
        struct {
            ir_place_t *func;
            ir_place_t *args;
        } call_stmt;
    } u;
};

ir_stmt_t *new_ir_assign_stmt(ir_place_t *lhs, ir_rvalue_t *rhs);
ir_stmt_t *new_ir_call_stmt(ir_place_t *func, ir_place_t *args);
void delete_ir_stmt(ir_stmt_t *stmt);

typedef enum {
    IR_TERMN_GOTO,
    IR_TERMN_IF,
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
    } u;
} ir_termn_t;

ir_termn_t *new_ir_goto_termn(ir_block_t *next);
ir_termn_t *new_ir_if_termn(ir_operand_t *cond, ir_block_t *then, ir_block_t *els);
ir_termn_t *new_ir_return_termn();
void delete_ir_termn(ir_termn_t *termn);

struct impl_ir_block {
    ir_stmt_t *stmt;
    ir_termn_t *termn;
};

ir_block_t *new_ir_block(ir_stmt_t *stmt, ir_termn_t *termn);
void delete_ir_block(ir_block_t *block);

typedef struct impl_ir_item_table ir_item_table_t;

typedef struct {
    ir_block_t *inner;
    ir_item_t *items;
} ir_body_t;

ir_body_t *new_ir_body(ir_block_t *inner, ir_item_t *items);
void delete_ir_body(ir_body_t *body);

typedef struct impl_ir_item_pos ir_item_pos_t;
struct impl_ir_item_pos {
    size_t pos;
    ir_item_pos_t *next;
};

typedef enum {
    IR_ITEM_PROGRAM,
    IR_ITEM_PROCEDURE,
    IR_ITEM_VAR,
    IR_ITEM_ARG_VAR,
    IR_ITEM_LOCAL_VAR
} ir_item_kind_t;

struct impl_ir_item {
    ir_item_kind_t kind;
    ir_type_t type;
    symbol_t symbol;
    ir_body_t *body;
    ir_item_t *next;
    region_t name_region;

    struct {
        ir_item_pos_t *head;
        ir_item_pos_t **tail;
    } refs;
};

ir_item_t *new_ir_program_item(ir_type_t type, symbol_t symbol, region_t name_region);
ir_item_t *new_ir_procedure_item(ir_type_t type, symbol_t symbol, region_t name_region);
ir_item_t *new_ir_var_item(ir_type_t type, symbol_t symbol, region_t name_region);
ir_item_t *new_ir_param_var_item(ir_type_t type, symbol_t symbol, region_t name_region);
ir_item_t *new_ir_local_var_item(ir_type_t type, symbol_t symbol, region_t name_region);
void delete_ir_item(ir_item_t *item);
void ir_item_add_ref(ir_item_t *item, size_t pos);

struct impl_ir_item_table {
    hash_table_t *table;
};

ir_item_table_t *new_ir_item_table();
void delete_ir_item_table(ir_item_table_t *table);
ir_item_t *ir_item_table_try_register(ir_item_table_t *table, ir_item_t *item);
ir_item_t *ir_item_table_lookup(ir_item_table_t *table, symbol_t symbol);

typedef struct {
    const source_t *source;
    ir_item_t *program;
} ir_t;

ir_t *new_ir(const source_t *source, ir_item_t *program);
void delete_ir(ir_t *ir);

#endif
