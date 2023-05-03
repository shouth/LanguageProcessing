#include <assert.h>

#include "context.h"
#include "ir.h"
#include "mppl.h"
#include "source.h"
#include "utility.h"

void ir_scope_start(ir_factory_t *factory, ir_item_t *owner)
{
  ir_scope_t *scope;
  assert(factory && owner);

  scope               = xmalloc(sizeof(ir_scope_t));
  scope->next         = factory->scope;
  factory->scope      = scope;
  scope->owner        = owner;
  scope->items.table  = hash_new(hash_default_comp, hash_default_hasher);
  scope->items.head   = NULL;
  scope->items.tail   = &scope->items.head;
  scope->locals.table = hash_new(hash_default_comp, hash_default_hasher);
  scope->locals.head  = NULL;
  scope->locals.tail  = &scope->locals.head;
}

void ir_scope_end(ir_factory_t *factory, const ir_block_t *block)
{
  ir_scope_t *scope;
  assert(factory);

  scope          = factory->scope;
  factory->scope = scope->next;

  scope->owner->body         = xmalloc(sizeof(ir_body_t));
  scope->owner->body->inner  = block;
  scope->owner->body->items  = scope->items.head;
  scope->owner->body->locals = scope->locals.head;

  hash_delete(scope->items.table, NULL, NULL);
  hash_delete(scope->locals.table, NULL, NULL);
  free(scope);
}

static ir_local_t *ir_scope_append_local(ir_scope_t *scope, ir_local_t *local)
{
  *scope->locals.tail = local;
  scope->locals.tail  = &local->next;
  return local;
}

static ir_local_t *new_ir_local(ir_local_kind_t kind)
{
  ir_local_t *ret = xmalloc(sizeof(ir_local_t));
  ret->kind       = kind;
  ret->next       = NULL;
  return ret;
}

const ir_local_t *ir_local_for(ir_factory_t *factory, ir_item_t *item, long pos)
{
  ir_item_pos_t *item_pos = xmalloc(sizeof(ir_item_pos_t));
  item_pos->pos           = pos;
  item_pos->next          = NULL;
  *item->refs.tail        = item_pos;
  item->refs.tail         = &item_pos->next;

  {
    const hash_entry_t *entry;
    if ((entry = hash_find(factory->scope->locals.table, item))) {
      return entry->value;
    }
  }

  {
    ir_local_t *local;
    switch (item->kind) {
    case IR_ITEM_VAR:
    case IR_ITEM_LOCAL_VAR:
    case IR_ITEM_PROCEDURE:
      local                 = new_ir_local(IR_LOCAL_VAR);
      local->local.var.item = item;
      break;
    case IR_ITEM_ARG_VAR:
      local                 = new_ir_local(IR_LOCAL_ARG);
      local->local.arg.item = item;
      break;
    default:
      unreachable();
    }
    hash_insert_unchecked(factory->scope->locals.table, item, local);
    return ir_scope_append_local(factory->scope, local);
  }
}

const ir_local_t *ir_local_temp(ir_factory_t *factory, const type_t *type)
{
  ir_local_t *local;
  assert(factory && type);

  local                  = new_ir_local(IR_LOCAL_TEMP);
  local->local.temp.type = type;
  return ir_scope_append_local(factory->scope, local);
}

const type_t *ir_local_type(const ir_local_t *local)
{
  switch (local->kind) {
  case IR_LOCAL_VAR:
    return local->local.var.item->type;
  case IR_LOCAL_ARG:
    return local->local.arg.item->type;
  case IR_LOCAL_TEMP:
    return local->local.temp.type;
  }

  unreachable();
}

void delete_ir_local(ir_local_t *local)
{
  if (!local) {
    return;
  }
  delete_ir_local(local->next);
  free(local);
}

static ir_place_t *init_ir_place(ir_place_t *place, ir_place_kind_t kind)
{
  place->kind = kind;
  return place;
}

ir_place_t *new_ir_plain_place(const ir_local_t *local)
{
  ir_place_plain_t *place = xmalloc(sizeof(ir_place_t));
  place->local            = local;
  return init_ir_place((ir_place_t *) place, IR_PLACE_KIND_PLAIN);
}

ir_place_t *new_ir_index_place(const ir_local_t *local, ir_operand_t *index)
{
  ir_place_indexed_t *place = xmalloc(sizeof(ir_place_t));
  place->local              = local;
  place->index              = index;
  return init_ir_place((ir_place_t *) place, IR_PLACE_KIND_INDEXED);
}

const type_t *ir_place_type(ir_place_t *place)
{
  switch (place->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *plain = (ir_place_plain_t *) place;
    return ir_local_type(plain->local);
  }
  case IR_PLACE_KIND_INDEXED: {
    ir_place_indexed_t *indexed = (ir_place_indexed_t *) place;
    const type_t       *type    = ir_local_type(indexed->local);
    switch (type->kind) {
    case TYPE_ARRAY: {
      const type_array_t *array = (type_array_t *) type;
      return array->base->types[0];
    }
    default:
      unreachable();
    }
  }
  default:
    unreachable();
  }
}

void delete_ir_place(ir_place_t *place)
{
  if (!place) {
    return;
  }
  switch (place->kind) {
  case IR_PLACE_KIND_INDEXED:
    delete_ir_operand(place->place.indexed.index);
    break;
  case IR_PLACE_KIND_PLAIN:
    /* do nothing */
    break;
  }
  free(place);
}

static ir_constant_t *new_ir_constant(ir_constant_kind_t kind, const type_t *type)
{
  ir_constant_t *ret = xmalloc(sizeof(ir_constant_t));
  ret->kind          = kind;
  ret->type          = type;
  ret->next          = NULL;
  return ret;
}

static void delete_ir_constant(ir_constant_t *constant)
{
  if (!constant) {
    return;
  }
  delete_ir_constant(constant->next);
  free(constant);
}

static const ir_constant_t *ir_constant_intern(ir_factory_t *factory, ir_constant_t *constant)
{
  const hash_entry_t *entry;
  assert(factory && constant);

  if ((entry = hash_find(factory->constants.table, constant))) {
    if (entry->value != constant) {
      delete_ir_constant(constant);
    }
    return entry->value;
  }
  hash_insert_unchecked(factory->constants.table, constant, constant);
  *factory->constants.tail = constant;
  factory->constants.tail  = &constant->next;
  return constant;
}

const ir_constant_t *ir_number_constant(ir_factory_t *factory, unsigned long value)
{
  ir_constant_t *ret         = new_ir_constant(IR_CONSTANT_NUMBER, type_integer(factory->context));
  ret->constant.number.value = value;
  return ir_constant_intern(factory, ret);
}

const ir_constant_t *ir_boolean_constant(ir_factory_t *factory, int value)
{
  ir_constant_t *ret          = new_ir_constant(IR_CONSTANT_BOOLEAN, type_boolean(factory->context));
  ret->constant.boolean.value = value;
  return ir_constant_intern(factory, ret);
}

const ir_constant_t *ir_char_constant(ir_factory_t *factory, int value)
{
  ir_constant_t *ret        = new_ir_constant(IR_CONSTANT_CHAR, type_char(factory->context));
  ret->constant.char_.value = value;
  return ir_constant_intern(factory, ret);
}

const ir_constant_t *ir_string_constant(ir_factory_t *factory, const symbol_t *value, long len)
{
  const type_t   *types[1];
  const substs_t *base;
  ir_constant_t  *ret;

  types[0]                   = type_char(factory->context);
  base                       = substs(factory->context, types, 1);
  ret                        = new_ir_constant(IR_CONSTANT_STRING, type_array(factory->context, base, len));
  ret->constant.string.value = value;
  return ir_constant_intern(factory, ret);
}

static int ir_constant_comparator(const void *lhs, const void *rhs)
{
  const ir_constant_t *l = lhs, *r = rhs;
  if (l->kind != r->kind) {
    return 0;
  }
  switch (l->kind) {
  case IR_CONSTANT_NUMBER:
    return l->constant.number.value == r->constant.number.value;
  case IR_CONSTANT_BOOLEAN:
    return l->constant.boolean.value == r->constant.boolean.value;
  case IR_CONSTANT_CHAR:
    return l->constant.char_.value == r->constant.char_.value;
  case IR_CONSTANT_STRING:
    return l->constant.string.value == r->constant.string.value;
  default:
    unreachable();
  }
}

static unsigned long ir_constant_hasher(const void *ptr)
{
  const ir_constant_t *p    = ptr;
  unsigned long        hash = fnv1a(FNV1A_SEED, &p->kind, sizeof(ir_constant_kind_t));
  switch (p->kind) {
  case IR_CONSTANT_NUMBER:
    hash = fnv1a(hash, &p->constant.number.value, sizeof(unsigned long));
    break;
  case IR_CONSTANT_BOOLEAN:
    hash = fnv1a(hash, &p->constant.boolean.value, sizeof(int));
    break;
  case IR_CONSTANT_CHAR:
    hash = fnv1a(hash, &p->constant.char_.value, sizeof(int));
    break;
  case IR_CONSTANT_STRING:
    hash = fnv1a(hash, &p->constant.string.value, sizeof(symbol_t *));
    break;
  }
  return hash;
}

const type_t *ir_constant_type(const ir_constant_t *constant)
{
  return constant->type;
}

static ir_operand_t *new_ir_operand(ir_operand_kind_t kind)
{
  ir_operand_t *ret = xmalloc(sizeof(ir_operand_t));
  ret->kind         = kind;
  ret->next         = NULL;
  return ret;
}

ir_operand_t *new_ir_place_operand(ir_place_t *place)
{
  ir_operand_t *ret        = new_ir_operand(IR_OPERAND_PLACE);
  ret->operand.place.place = place;
  return ret;
}

ir_operand_t *new_ir_constant_operand(const ir_constant_t *constant)
{
  ir_operand_t *ret              = new_ir_operand(IR_OPERAND_CONSTANT);
  ret->operand.constant.constant = constant;
  return ret;
}

const type_t *ir_operand_type(ir_operand_t *operand)
{
  assert(operand);

  switch (operand->kind) {
  case IR_OPERAND_PLACE:
    return ir_place_type(operand->operand.place.place);
  case IR_OPERAND_CONSTANT:
    return ir_constant_type(operand->operand.constant.constant);
  default:
    unreachable();
  }
}

void delete_ir_operand(ir_operand_t *operand)
{
  if (!operand) {
    return;
  }
  switch (operand->kind) {
  case IR_OPERAND_PLACE:
    delete_ir_place(operand->operand.place.place);
    break;
  default:
    /* do nothing */
    break;
  }
  delete_ir_operand(operand->next);
  free(operand);
}

static ir_rvalue_t *new_ir_rvalue(ir_rvalue_kind_t kind)
{
  ir_rvalue_t *ret = xmalloc(sizeof(ir_rvalue_t));
  ret->kind        = kind;
  return ret;
}

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *operand)
{
  ir_rvalue_t *ret        = new_ir_rvalue(IR_RVALUE_USE);
  ret->rvalue.use.operand = operand;
  return ret;
}

ir_rvalue_t *new_ir_binary_op_rvalue(ast_expr_binary_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs)
{
  ir_rvalue_t *ret        = new_ir_rvalue(IR_RVALUE_BINARY);
  ret->rvalue.binary.kind = kind;
  ret->rvalue.binary.lhs  = lhs;
  ret->rvalue.binary.rhs  = rhs;
  return ret;
}

ir_rvalue_t *new_ir_not_rvalue(ir_operand_t *value)
{
  ir_rvalue_t *ret       = new_ir_rvalue(IR_RVALUE_NOT);
  ret->rvalue.not_.value = value;
  return ret;
}

ir_rvalue_t *new_ir_cast_rvalue(const type_t *type, ir_operand_t *value)
{
  ir_rvalue_t *ret       = new_ir_rvalue(IR_RVALUE_CAST);
  ret->rvalue.cast.type  = type;
  ret->rvalue.cast.value = value;
  return ret;
}

void delete_ir_rvalue(ir_rvalue_t *rvalue)
{
  if (!rvalue) {
    return;
  }
  switch (rvalue->kind) {
  case IR_RVALUE_USE:
    delete_ir_operand(rvalue->rvalue.use.operand);
    break;
  case IR_RVALUE_BINARY:
    delete_ir_operand(rvalue->rvalue.binary.lhs);
    delete_ir_operand(rvalue->rvalue.binary.rhs);
    break;
  case IR_RVALUE_NOT:
    delete_ir_operand(rvalue->rvalue.not_.value);
    break;
  case IR_RVALUE_CAST:
    delete_ir_operand(rvalue->rvalue.cast.value);
    break;
  }
  free(rvalue);
}

static ir_stmt_t *ir_stmt_new(ir_stmt_kind_t kind)
{
  ir_stmt_t *ret = xmalloc(sizeof(ir_stmt_t));
  ret->kind      = kind;
  ret->next      = NULL;
  return ret;
}

void delete_ir_stmt(ir_stmt_t *stmt)
{
  if (stmt) {
    switch (stmt->kind) {
    case IR_STMT_ASSIGN:
      delete_ir_place(stmt->stmt.assign.lhs);
      delete_ir_rvalue(stmt->stmt.assign.rhs);
      break;
    case IR_STMT_CALL:
      delete_ir_place(stmt->stmt.call.func);
      delete_ir_operand(stmt->stmt.call.args);
      break;
    case IR_STMT_READ:
      delete_ir_place(stmt->stmt.read.ref);
      break;
    case IR_STMT_WRITE:
      delete_ir_operand(stmt->stmt.write.value);
      break;
    default:
      /* do nothing */
      break;
    }
    delete_ir_stmt(stmt->next);
  }
  free(stmt);
}

ir_block_t *ir_block(ir_factory_t *factory)
{
  ir_block_t *ret       = xmalloc(sizeof(ir_block_t));
  ret->stmt             = NULL;
  ret->stmt_tail        = &ret->stmt;
  ret->next             = NULL;
  ret->termn            = NULL;
  *factory->blocks.tail = ret;
  factory->blocks.tail  = &ret->next;
  return ret;
}

static void ir_block_append_block(ir_block_t *block, ir_stmt_t *stmt)
{
  assert(block && stmt);
  *block->stmt_tail = stmt;
  block->stmt_tail  = &stmt->next;
}

void ir_block_push_assign(ir_block_t *block, ir_place_t *lhs, ir_rvalue_t *rhs)
{
  ir_stmt_assign_t *stmt = (ir_stmt_assign_t *) ir_stmt_new(IR_STMT_ASSIGN);
  stmt->lhs              = lhs;
  stmt->rhs              = rhs;
  ir_block_append_block(block, (ir_stmt_t *) stmt);
}

void ir_block_push_call(ir_block_t *block, ir_place_t *func, ir_operand_t *args)
{
  ir_stmt_call_t *stmt = (ir_stmt_call_t *) ir_stmt_new(IR_STMT_CALL);
  stmt->func           = func;
  stmt->args           = args;
  ir_block_append_block(block, (ir_stmt_t *) stmt);
}

void ir_block_push_read(ir_block_t *block, ir_place_t *ref)
{
  ir_stmt_read_t *stmt = (ir_stmt_read_t *) ir_stmt_new(IR_STMT_READ);
  stmt->ref            = ref;
  ir_block_append_block(block, (ir_stmt_t *) stmt);
}

void ir_block_push_readln(ir_block_t *block)
{
  ir_block_append_block(block, ir_stmt_new(IR_STMT_READLN));
}

void ir_block_push_write(ir_block_t *block, ir_operand_t *value, const ir_constant_t *len)
{
  ir_stmt_write_t *stmt = (ir_stmt_write_t *) ir_stmt_new(IR_STMT_WRITE);
  stmt->value           = value;
  stmt->len             = len;
  ir_block_append_block(block, (ir_stmt_t *) stmt);
}

void ir_block_push_writeln(ir_block_t *block)
{
  ir_block_append_block(block, ir_stmt_new(IR_STMT_WRITELN));
}

static ir_termn_t *ir_block_terminate(ir_block_t *block, ir_termn_kind_t kind)
{
  block->termn       = xmalloc(sizeof(ir_termn_t));
  block->termn->kind = kind;
  return block->termn;
}

void ir_block_terminate_goto(ir_block_t *block, const ir_block_t *next)
{
  ir_termn_goto_t *termn = (ir_termn_goto_t *) ir_block_terminate(block, IR_TERMN_GOTO);
  termn->next            = next;
}

void ir_block_terminate_if(ir_block_t *block, ir_operand_t *cond, const ir_block_t *then, const ir_block_t *els)
{
  ir_termn_if_t *termn = (ir_termn_if_t *) ir_block_terminate(block, IR_TERMN_IF);
  termn->cond          = cond;
  termn->then          = then;
  termn->els           = els;
}

void ir_block_terminate_return(ir_block_t *block)
{
  ir_block_terminate(block, IR_TERMN_RETURN);
}

void ir_block_terminate_arg(ir_block_t *block, const ir_operand_t *arg, const ir_block_t *next)
{
  ir_termn_arg_t *termn = (ir_termn_arg_t *) ir_block_terminate(block, IR_TERMN_ARG);
  termn->arg            = arg;
  termn->next           = next;
}

void delete_ir_block(ir_block_t *block)
{
  if (block) {
    if (block->termn) {
      switch (block->termn->kind) {
      case IR_TERMN_IF: {
        ir_termn_if_t *if_ = (ir_termn_if_t *) block->termn;
        delete_ir_operand(if_->cond);
        break;
      }
      default:
        /* do nothing */
        break;
      }
    }
    free(block->termn);
    delete_ir_stmt(block->stmt);
    delete_ir_block(block->next);
  }
  free(block);
}

ir_item_t *ir_item(ir_factory_t *factory, ir_item_kind_t kind, const symbol_t *symbol, region_t name_region, const type_t *type)
{
  ir_item_t *ret   = xmalloc(sizeof(ir_item_t));
  ret->kind        = kind;
  ret->type        = type;
  ret->symbol      = symbol;
  ret->body        = 0;
  ret->next        = NULL;
  ret->name_region = name_region;
  ret->refs.head   = NULL;
  ret->refs.tail   = &ret->refs.head;

  if (kind != IR_ITEM_PROGRAM) {
    assert(!ir_item_lookup_scope(factory->scope, symbol));
    hash_insert_unchecked(factory->scope->items.table, (void *) symbol, ret);
    *factory->scope->items.tail = ret;
    factory->scope->items.tail  = &ret->next;
  }
  return ret;
}

ir_item_t *ir_item_lookup_scope(ir_scope_t *scope, const symbol_t *symbol)
{
  const hash_entry_t *entry;
  assert(scope);

  entry = hash_find(scope->items.table, (void *) symbol);
  return entry ? entry->value : NULL;
}

ir_item_t *ir_item_lookup(ir_scope_t *scope, const symbol_t *symbol)
{
  assert(scope);

  while (scope) {
    ir_item_t *item = ir_item_lookup_scope(scope, symbol);
    if (item) {
      return item;
    }
    scope = scope->next;
  }
  return NULL;
}

static void delete_ir_item_pos(ir_item_pos_t *pos)
{
  if (!pos) {
    return;
  }
  delete_ir_item_pos(pos->next);
  free(pos);
}

static void delete_ir_item(ir_item_t *item)
{
  if (!item) {
    return;
  }
  if (item->body) {
    delete_ir_item(item->body->items);
    delete_ir_local(item->body->locals);
    free(item->body);
  }
  delete_ir_item_pos(item->refs.head);
  delete_ir_item(item->next);
  free(item);
}

ir_factory_t *new_ir_factory(context_t *context)
{
  ir_factory_t *ret = xmalloc(sizeof(ir_factory_t));
  ret->scope        = NULL;

  ret->blocks.head = NULL;
  ret->blocks.tail = &ret->blocks.head;

  ret->constants.head  = NULL;
  ret->constants.tail  = &ret->constants.head;
  ret->constants.table = hash_new(ir_constant_comparator, ir_constant_hasher);

  ret->context = context;
  return ret;
}

ir_t *new_ir(const source_t *source, ir_item_t *items, ir_factory_t *factory)
{
  ir_t *ret      = xmalloc(sizeof(ir_t));
  ret->source    = source;
  ret->items     = items;
  ret->blocks    = factory->blocks.head;
  ret->constants = factory->constants.head;
  ret->types     = factory->types.head;

  hash_delete(factory->constants.table, NULL, NULL);
  hash_delete(factory->types.table, NULL, NULL);
  free(factory);

  return ret;
}

void delete_ir(ir_t *ir)
{
  if (!ir) {
    return;
  }
  delete_ir_item(ir->items);
  delete_ir_block(ir->blocks);
  delete_ir_constant(ir->constants);
  free(ir);
}
