#ifndef IR_H
#define IR_H

#include "mppl.h"

typedef struct impl_ir_factory ir_factory_t;
typedef struct impl_ir_scope   ir_scope_t;
typedef struct impl_ir_item    ir_item_t;

typedef enum {
  IR_TYPE_PROGRAM,
  IR_TYPE_PROCEDURE,
  IR_TYPE_ARRAY,
  IR_TYPE_INTEGER,
  IR_TYPE_BOOLEAN,
  IR_TYPE_CHAR
} ir_type_kind_t;

typedef struct impl_ir_type ir_type_t;
struct impl_ir_type {
  ir_type_kind_t kind;
  ir_type_t     *next;

  union {
    struct {
      ir_type_t *param_types;
    } procedure_type;
    struct {
      ir_type_t *base_type;
      size_t     size;
    } array_type;
    const ir_type_t *ref;
  } u;
};

int         ir_type_is_kind(const ir_type_t *type, ir_type_kind_t kind);
int         ir_type_is_std(const ir_type_t *type);
ir_type_t  *ir_type_ref(const ir_type_t *type);
const char *ir_type_str(const ir_type_t *type);

const ir_type_t *ir_type_program(ir_factory_t *factory);
const ir_type_t *ir_type_procedure(ir_factory_t *factory, ir_type_t *params);
const ir_type_t *ir_type_array(ir_factory_t *factory, ir_type_t *base, size_t size);
const ir_type_t *ir_type_integer(ir_factory_t *factory);
const ir_type_t *ir_type_char(ir_factory_t *factory);
const ir_type_t *ir_type_boolean(ir_factory_t *factory);

typedef enum {
  IR_LOCAL_VAR,
  IR_LOCAL_ARG,
  IR_LOCAL_TEMP
} ir_local_kind_t;

typedef struct impl_ir_local ir_local_t;
struct impl_ir_local {
  ir_local_kind_t kind;
  ir_local_t     *next;

  union {
    struct {
      const ir_item_t *item;
    } var;
    struct {
      const ir_item_t *item;
    } arg;
    struct {
      const ir_type_t *type;
    } temp;
  } u;
};

const ir_local_t *ir_local_for(ir_factory_t *factory, ir_item_t *item, size_t pos);
const ir_local_t *ir_local_temp(ir_factory_t *factory, const ir_type_t *type);
const ir_type_t  *ir_local_type(const ir_local_t *local);

typedef struct impl_ir_operand ir_operand_t;

typedef enum {
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

typedef struct impl_ir_place ir_place_t;
struct impl_ir_place {
  const ir_local_t  *local;
  ir_place_access_t *place_access;
};

ir_place_t      *new_ir_place(const ir_local_t *local);
ir_place_t      *new_ir_index_place(const ir_local_t *local, ir_operand_t *index);
const ir_type_t *ir_place_type(ir_place_t *place);
void             delete_ir_place(ir_place_t *place);

typedef enum {
  IR_CONSTANT_NUMBER,
  IR_CONSTANT_BOOLEAN,
  IR_CONSTANT_CHAR,
  IR_CONSTANT_STRING
} ir_constant_kind_t;

typedef struct impl_ir_constant ir_constant_t;
struct impl_ir_constant {
  ir_constant_kind_t kind;
  const ir_type_t   *type;
  ir_constant_t     *next;

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
};

const ir_constant_t *ir_number_constant(ir_factory_t *factory, unsigned long value);
const ir_constant_t *ir_boolean_constant(ir_factory_t *factory, int value);
const ir_constant_t *ir_char_constant(ir_factory_t *factory, int value);
const ir_constant_t *ir_string_constant(ir_factory_t *factory, const symbol_t *value, size_t len);
const ir_type_t     *ir_constant_type(const ir_constant_t *constant);

typedef enum {
  IR_OPERAND_PLACE,
  IR_OPERAND_CONSTANT
} ir_operand_kind_t;

struct impl_ir_operand {
  ir_operand_kind_t kind;
  ir_operand_t     *next;

  union {
    struct {
      ir_place_t *place;
    } place_operand;
    struct {
      const ir_constant_t *constant;
    } constant_operand;
  } u;
};

ir_operand_t    *new_ir_place_operand(ir_place_t *place);
ir_operand_t    *new_ir_constant_operand(const ir_constant_t *constant);
const ir_type_t *ir_operand_type(ir_operand_t *operand);
void             delete_ir_operand(ir_operand_t *operand);

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
      ir_operand_t        *lhs;
      ir_operand_t        *rhs;
    } binary_op_rvalue;
    struct {
      ast_unary_op_kind_t kind;
      ir_operand_t       *value;
    } unary_op_rvalue;
    struct {
      const ir_type_t *type;
      ir_operand_t    *value;
    } cast_rvalue;
  } u;
} ir_rvalue_t;

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *operand);
ir_rvalue_t *new_ir_binary_op_rvalue(ast_binary_op_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs);
ir_rvalue_t *new_ir_unary_op_rvalue(ast_unary_op_kind_t kind, ir_operand_t *value);
ir_rvalue_t *new_ir_cast_rvalue(const ir_type_t *type, ir_operand_t *value);
void         delete_ir_rvalue(ir_rvalue_t *rvalue);

typedef struct impl_ir_stmt ir_stmt_t;

typedef enum {
  IR_STMT_ASSIGN,
  IR_STMT_CALL,
  IR_STMT_READ,
  IR_STMT_READLN,
  IR_STMT_WRITE,
  IR_STMT_WRITELN
} ir_stmt_kind_t;

typedef struct {
  ir_place_t  *lhs;
  ir_rvalue_t *rhs;
} ir_assign_stmt_t;

typedef struct {
  ir_place_t   *func;
  ir_operand_t *args;
} ir_call_stmt_t;

typedef struct {
  ir_place_t *ref;
} ir_read_stmt_t;

typedef struct {
  ir_operand_t        *value;
  const ir_constant_t *len;
} ir_write_stmt_t;

struct impl_ir_stmt {
  ir_stmt_kind_t kind;
  ir_stmt_t     *next;

  union {
    ir_assign_stmt_t assign_stmt;
    ir_call_stmt_t   call_stmt;
    ir_read_stmt_t   read_stmt;
    ir_write_stmt_t  write_stmt;
  } u;
};

typedef enum {
  IR_TERMN_GOTO,
  IR_TERMN_IF,
  IR_TERMN_RETURN,
  IR_TERMN_ARG
} ir_termn_kind_t;

typedef struct impl_ir_block ir_block_t;
struct impl_ir_block {
  ir_block_t *next;
  ir_stmt_t  *stmt;
  ir_stmt_t **stmt_tail;

  struct {
    ir_termn_kind_t kind;
    union {
      struct {
        const ir_block_t *next;
      } goto_termn;
      struct {
        ir_operand_t     *cond;
        const ir_block_t *then;
        const ir_block_t *els;
      } if_termn;
      struct {
        const ir_operand_t *arg;
        const ir_block_t   *next;
      } arg_termn;
    } u;
  } termn;
};

ir_block_t *ir_block(ir_factory_t *factory);
void        ir_block_push_assign(ir_block_t *block, ir_place_t *lhs, ir_rvalue_t *rhs);
void        ir_block_push_call(ir_block_t *block, ir_place_t *func, ir_operand_t *args);
void        ir_block_push_read(ir_block_t *block, ir_place_t *ref);
void        ir_block_push_readln(ir_block_t *block);
void        ir_block_push_write(ir_block_t *block, ir_operand_t *value, const ir_constant_t *len);
void        ir_block_push_writeln(ir_block_t *block);
void        ir_block_terminate_goto(ir_block_t *block, const ir_block_t *next);
void        ir_block_terminate_if(ir_block_t *block, ir_operand_t *cond, const ir_block_t *then, const ir_block_t *els);
void        ir_block_terminate_return(ir_block_t *block);
void        ir_block_terminate_arg(ir_block_t *block, const ir_operand_t *arg, const ir_block_t *next);

typedef struct {
  const ir_block_t *inner;
  ir_item_t        *items;
  ir_local_t       *locals;
} ir_body_t;

typedef struct impl_ir_item_pos ir_item_pos_t;
struct impl_ir_item_pos {
  size_t         pos;
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
  ir_item_kind_t   kind;
  const ir_type_t *type;
  const symbol_t  *symbol;
  ir_body_t       *body;
  ir_item_t       *next;
  region_t         name_region;

  struct {
    ir_item_pos_t  *head;
    ir_item_pos_t **tail;
  } refs;
};

ir_item_t *ir_item(ir_factory_t *factory, ir_item_kind_t kind, const symbol_t *symbol, region_t name_region, const ir_type_t *type);
ir_item_t *ir_item_lookup_scope(ir_scope_t *scope, const symbol_t *symbol);
ir_item_t *ir_item_lookup(ir_scope_t *scope, const symbol_t *symbol);

struct impl_ir_scope {
  ir_scope_t *next;
  ir_item_t  *owner;
  struct {
    hash_table_t *table;
    ir_item_t    *head;
    ir_item_t   **tail;
  } items;
  struct {
    hash_table_t *table;
    ir_local_t   *head;
    ir_local_t  **tail;
  } locals;
};

void ir_scope_start(ir_factory_t *factory, ir_item_t *owner);
void ir_scope_end(ir_factory_t *factory, const ir_block_t *block);

struct impl_ir_factory {
  struct {
    ir_block_t  *head;
    ir_block_t **tail;
  } blocks;
  struct {
    hash_table_t   *table;
    ir_constant_t  *head;
    ir_constant_t **tail;
  } constants;
  struct {
    hash_table_t    *table;
    ir_type_t       *head;
    ir_type_t      **tail;
    const ir_type_t *program;
    const ir_type_t *std_integer;
    const ir_type_t *std_char;
    const ir_type_t *std_boolean;
  } types;
  ir_scope_t *scope;
};

ir_factory_t *new_ir_factory();

typedef struct {
  const source_t *source;
  ir_item_t      *items;
  ir_block_t     *blocks;
  ir_constant_t  *constants;
  ir_type_t      *types;
} ir_t;

ir_t *new_ir(const source_t *source, ir_item_t *items, ir_factory_t *factory);
void  delete_ir(ir_t *ir);

#endif
