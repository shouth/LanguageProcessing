#ifndef IR_H
#define IR_H

#include "ast.h"
#include "context.h"
#include "source.h"

typedef struct impl_ir_factory ir_factory_t;
typedef struct impl_ir_scope   ir_scope_t;
typedef struct impl_ir_item    ir_item_t;

typedef enum {
  IR_LOCAL_VAR,
  IR_LOCAL_ARG,
  IR_LOCAL_TEMP
} ir_local_kind_t;

typedef struct ir__local_s      ir_local_t;
typedef struct ir__local_var_s  ir_local_var_t;
typedef struct ir__local_arg_s  ir_local_arg_t;
typedef struct ir__local_temp_s ir_local_temp_t;

struct ir__local_var_s {
  const ir_item_t *item;
};

struct ir__local_arg_s {
  const ir_item_t *item;
};

struct ir__local_temp_s {
  const type_t *type;
};

struct ir__local_s {
  union {
    ir_local_var_t  var;
    ir_local_arg_t  arg;
    ir_local_temp_t temp;
  } local;

  ir_local_kind_t kind;
  ir_local_t     *next;
};

const ir_local_t *ir_local_for(ir_factory_t *factory, ir_item_t *item, long pos);
const ir_local_t *ir_local_temp(ir_factory_t *factory, const type_t *type);
const type_t     *ir_local_type(const ir_local_t *local);

typedef struct ir__operand_s ir_operand_t;

typedef enum {
  IR_PLACE_KIND_PLAIN,
  IR_PLACE_KIND_INDEXED
} ir_place_kind_t;

typedef struct ir__place_s         ir_place_t;
typedef struct ir__place_plain_s   ir_place_plain_t;
typedef struct ir__place_indexed_s ir_place_indexed_t;

struct ir__place_plain_s {
  const ir_local_t *local;
};

struct ir__place_indexed_s {
  ir_operand_t     *index;
  const ir_local_t *local;
};

struct ir__place_s {
  union {
    ir_place_plain_t   plain;
    ir_place_indexed_t indexed;
  } place;

  ir_place_kind_t kind;
};

ir_place_t   *new_ir_plain_place(const ir_local_t *local);
ir_place_t   *new_ir_index_place(const ir_local_t *local, ir_operand_t *index);
const type_t *ir_place_type(ir_place_t *place);
void          delete_ir_place(ir_place_t *place);

typedef enum {
  IR_CONSTANT_NUMBER,
  IR_CONSTANT_BOOLEAN,
  IR_CONSTANT_CHAR,
  IR_CONSTANT_STRING
} ir_constant_kind_t;

typedef struct ir__constant_s         ir_constant_t;
typedef struct ir__constant_number_s  ir_constant_number_t;
typedef struct ir__constant_boolean_s ir_constant_boolean_t;
typedef struct ir__constant_char_s    ir_constant_char_t;
typedef struct ir__constant_string_s  ir_constant_string_t;

struct ir__constant_number_s {
  unsigned long value;
};

struct ir__constant_boolean_s {
  int value;
};

struct ir__constant_char_s {
  int value;
};

struct ir__constant_string_s {
  const symbol_t *value;
};

struct ir__constant_s {
  union {
    ir_constant_number_t  number;
    ir_constant_boolean_t boolean;
    ir_constant_char_t    char_;
    ir_constant_string_t  string;
  } constant;

  ir_constant_kind_t kind;
  const type_t      *type;
  ir_constant_t     *next;
};

const ir_constant_t *ir_number_constant(ir_factory_t *factory, unsigned long value);
const ir_constant_t *ir_boolean_constant(ir_factory_t *factory, int value);
const ir_constant_t *ir_char_constant(ir_factory_t *factory, int value);
const ir_constant_t *ir_string_constant(ir_factory_t *factory, const symbol_t *value, long len);
const type_t        *ir_constant_type(const ir_constant_t *constant);

typedef enum {
  IR_OPERAND_PLACE,
  IR_OPERAND_CONSTANT
} ir_operand_kind_t;

typedef struct ir__operand_place_s    ir_operand_place_t;
typedef struct ir__operand_constant_s ir_operand_constant_t;

struct ir__operand_place_s {
  ir_place_t *place;
};

struct ir__operand_constant_s {
  const ir_constant_t *constant;
};

struct ir__operand_s {
  union {
    ir_operand_place_t    place;
    ir_operand_constant_t constant;
  } operand;

  ir_operand_kind_t kind;
  ir_operand_t     *next;
};

ir_operand_t *new_ir_place_operand(ir_place_t *place);
ir_operand_t *new_ir_constant_operand(const ir_constant_t *constant);
const type_t *ir_operand_type(ir_operand_t *operand);
void          delete_ir_operand(ir_operand_t *operand);

typedef enum {
  IR_RVALUE_USE,
  IR_RVALUE_BINARY,
  IR_RVALUE_NOT,
  IR_RVALUE_CAST
} ir_rvalue_kind_t;

typedef struct ir__rvalue_s        ir_rvalue_t;
typedef struct ir__rvalue_use_s    ir_rvalue_use_t;
typedef struct ir__rvalue_binary_s ir_rvalue_binary_t;
typedef struct ir__rvalue_not_s    ir_rvalue_not_t;
typedef struct ir__rvalue_cast_s   ir_rvalue_cast_t;

struct ir__rvalue_use_s {
  ir_operand_t *operand;
};

struct ir__rvalue_binary_s {
  ast_expr_binary_kind_t kind;
  ir_operand_t          *lhs;
  ir_operand_t          *rhs;
};

struct ir__rvalue_not_s {
  ir_operand_t *value;
};

struct ir__rvalue_cast_s {
  const type_t *type;
  ir_operand_t *value;
};

struct ir__rvalue_s {
  union {
    ir_rvalue_use_t    use;
    ir_rvalue_binary_t binary;
    ir_rvalue_not_t    not_;
    ir_rvalue_cast_t   cast;
  } rvalue;

  ir_rvalue_kind_t kind;
};

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *operand);
ir_rvalue_t *new_ir_binary_op_rvalue(ast_expr_binary_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs);
ir_rvalue_t *new_ir_not_rvalue(ir_operand_t *value);
ir_rvalue_t *new_ir_cast_rvalue(const type_t *type, ir_operand_t *value);
void         delete_ir_rvalue(ir_rvalue_t *rvalue);

typedef struct ir__stmt_s        ir_stmt_t;
typedef struct ir__stmt_assign_s ir_stmt_assign_t;
typedef struct ir__stmt_call_s   ir_stmt_call_t;
typedef struct ir__stmt_read_s   ir_stmt_read_t;
typedef struct ir__stmt_write_s  ir_stmt_write_t;

typedef enum {
  IR_STMT_ASSIGN,
  IR_STMT_CALL,
  IR_STMT_READ,
  IR_STMT_READLN,
  IR_STMT_WRITE,
  IR_STMT_WRITELN
} ir_stmt_kind_t;

struct ir__stmt_assign_s {
  ir_place_t  *lhs;
  ir_rvalue_t *rhs;
};

struct ir__stmt_call_s {
  ir_place_t   *func;
  ir_operand_t *args;
};

struct ir__stmt_read_s {
  ir_place_t *ref;
};

struct ir__stmt_write_s {
  ir_operand_t        *value;
  const ir_constant_t *len;
};

struct ir__stmt_s {
  union {
    ir_stmt_assign_t assign;
    ir_stmt_call_t   call;
    ir_stmt_read_t   read;
    ir_stmt_write_t  write;
  } stmt;

  ir_stmt_kind_t kind;
  ir_stmt_t     *next;
};

typedef enum {
  IR_TERMN_GOTO,
  IR_TERMN_IF,
  IR_TERMN_RETURN,
  IR_TERMN_ARG
} ir_termn_kind_t;

typedef struct ir__block_s ir_block_t;

typedef struct ir__termn_s      ir_termn_t;
typedef struct ir__termn_goto_s ir_termn_goto_t;
typedef struct ir__termn_if_s   ir_termn_if_t;
typedef struct ir__termn_arg_s  ir_termn_arg_t;

struct ir__termn_goto_s {
  const ir_block_t *next;
};

struct ir__termn_if_s {
  ir_operand_t     *cond;
  const ir_block_t *then;
  const ir_block_t *els;
};

struct ir__termn_arg_s {
  const ir_operand_t *arg;
  const ir_block_t   *next;
};

struct ir__termn_s {
  union {
    ir_termn_goto_t goto_;
    ir_termn_if_t   if_;
    ir_termn_arg_t  arg;
  } termn;

  ir_termn_kind_t kind;
};

struct ir__block_s {
  ir_block_t *next;
  ir_stmt_t  *stmt;
  ir_stmt_t **stmt_tail;
  ir_termn_t *termn;
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
  long           pos;
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
  ir_item_kind_t  kind;
  const type_t   *type;
  const symbol_t *symbol;
  ir_body_t      *body;
  ir_item_t      *next;
  region_t        name_region;

  struct {
    ir_item_pos_t  *head;
    ir_item_pos_t **tail;
  } refs;
};

ir_item_t *ir_item(ir_factory_t *factory, ir_item_kind_t kind, const symbol_t *symbol, region_t name_region, const type_t *type);
ir_item_t *ir_item_lookup_scope(ir_scope_t *scope, const symbol_t *symbol);
ir_item_t *ir_item_lookup(ir_scope_t *scope, const symbol_t *symbol);

struct impl_ir_scope {
  ir_scope_t *next;
  ir_item_t  *owner;
  struct {
    hash_t     *table;
    ir_item_t  *head;
    ir_item_t **tail;
  } items;
  struct {
    hash_t      *table;
    ir_local_t  *head;
    ir_local_t **tail;
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
    hash_t         *table;
    ir_constant_t  *head;
    ir_constant_t **tail;
  } constants;
  struct {
    hash_t       *table;
    type_t       *head;
    type_t      **tail;
    const type_t *program;
    const type_t *std_integer;
    const type_t *std_char;
    const type_t *std_boolean;
  } types;
  ir_scope_t *scope;
  context_t  *context;
};

ir_factory_t *new_ir_factory(context_t *context);

typedef struct {
  const source_t *source;
  ir_item_t      *items;
  ir_block_t     *blocks;
  ir_constant_t  *constants;
  type_t         *types;
} ir_t;

ir_t *new_ir(const source_t *source, ir_item_t *items, ir_factory_t *factory);
void  delete_ir(ir_t *ir);

#endif
