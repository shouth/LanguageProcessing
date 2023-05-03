#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "context.h"
#include "ir.h"
#include "mppl.h"
#include "utility.h"

typedef long addr_t;

typedef struct {
  FILE *file;
  struct {
    addr_t  cnt;
    hash_t *table;
  } addr;
  char label[16];
  struct {
    int e_ov, e_rng, e_div0;
    int r_int, r_char, r_ln;
    int w_int, w_char, w_bool, w_str;
  } builtin;
} emitter_t;

static addr_t lookup_addr(emitter_t *emitter, const void *ptr)
{
  const hash_entry_t *entry;
  if (ptr && (entry = hash_find(emitter->addr.table, ptr))) {
    return (addr_t) entry->value;
  } else {
    return 0;
  }
}

static addr_t addr_of(emitter_t *emitter, const void *ptr)
{
  addr_t addr;
  if (!(addr = lookup_addr(emitter, ptr))) {
    addr = ++emitter->addr.cnt;
    if (ptr) {
      hash_insert_unchecked(emitter->addr.table, (void *) ptr, (void *) addr);
    }
  }
  return addr;
}

static const char *addr_label(addr_t addr)
{
  static char buf[16];
  sprintf(buf, "L%ld", addr);
  return buf;
}

static const char *item_label(emitter_t *emitter, const void *ptr)
{
  return addr_label(addr_of(emitter, ptr));
}

static void emit_inst(emitter_t *emitter, const char *instruction, const char *operand_format, ...)
{
  va_list arg;
  va_start(arg, operand_format);
  fprintf(emitter->file, "%-10s", emitter->label);
  if (operand_format) {
    fprintf(emitter->file, "%-8s", instruction);
    vfprintf(emitter->file, operand_format, arg);
  } else {
    fprintf(emitter->file, "%s", instruction);
  }
  fprintf(emitter->file, "\n");
  va_end(arg);
  emitter->label[0] = '\0';
}

static void emit_label(emitter_t *emitter, const char *label)
{
  if (emitter->label[0]) {
    emit_inst(emitter, "DS", "0");
  }
  strcpy(emitter->label, label);
}

static void emit_constant(emitter_t *emitter, const ir_constant_t *constant)
{
  while (constant) {
    switch (constant->kind) {
    case IR_CONSTANT_STRING: {
      const symbol_t *symbol = constant->constant.string.value;
      emit_label(emitter, item_label(emitter, constant));
      emit_inst(emitter, "DC", "\'%.*s\'", (int) symbol->len, symbol->ptr);
      break;
    default:
      /* do nothing */
      break;
    }
    }
    constant = constant->next;
  }
}

static void emit_range_check(emitter_t *emitter, const char *reg, const ir_local_t *local)
{
  const type_t *type = ir_local_type(local);
  ++emitter->builtin.e_rng;
  if (local->kind == IR_LOCAL_VAR && type->kind == TYPE_ARRAY) {
    const type_array_t *array = (type_array_t *) type;
    const type_t       *base  = array->base->types[0];
    if (base->kind == TYPE_INTEGER || base->kind == TYPE_CHAR || type->kind == TYPE_BOOLEAN) {
      if (type->type.array.size) {
        emit_inst(emitter, "LD", "GR0, %s", reg);
        emit_inst(emitter, "JMI", "ERNG");
        emit_inst(emitter, "LAD", "GR0, %ld", type->type.array.size - 1);
        emit_inst(emitter, "CPA", "%s, GR0", reg);
        emit_inst(emitter, "JPL", "ERNG");
      } else {
        emit_inst(emitter, "JPL", "ERNG");
      }
    } else {
      unreachable();
    }
  } else {
    unreachable();
  }
}

static void emit_load(emitter_t *emitter, const char *reg, const ir_operand_t *operand);

static void emit_load_constant(emitter_t *emitter, const char *reg, const ir_constant_t *constant)
{
  switch (constant->kind) {
  case IR_CONSTANT_NUMBER:
    emit_inst(emitter, "LAD", "%s, %ld", reg, constant->constant.number.value);
    break;
  case IR_CONSTANT_CHAR:
    emit_inst(emitter, "LAD", "%s, #%04X", reg, constant->constant.char_.value);
    break;
  case IR_CONSTANT_BOOLEAN:
    emit_inst(emitter, "LAD", "%s, %d", reg, constant->constant.boolean.value);
    break;
  default:
    unreachable();
  }
}

static void emit_load_place(emitter_t *emitter, const char *reg, const ir_place_t *place)
{
  switch (place->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *plain = (ir_place_plain_t *) place;
    switch (plain->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "LD", "%s, %s", reg, item_label(emitter, plain->local->local.var.item));
      break;
    case IR_LOCAL_ARG:
      emit_inst(emitter, "LD", "GR7, %s", item_label(emitter, plain->local->local.arg.item));
      emit_inst(emitter, "LD", "%s, 0, GR7", reg);
      break;
    case IR_LOCAL_TEMP:
      emit_inst(emitter, "POP", "%s", reg);
      break;
    default:
      unreachable();
    }
    break;
  }
  case IR_PLACE_KIND_INDEXED: {
    ir_place_indexed_t *indexed = (ir_place_indexed_t *) place;
    emit_load(emitter, "GR7", indexed->index);
    emit_range_check(emitter, "GR7", indexed->local);
    switch (indexed->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "LD", "%s, %s, GR7", reg, item_label(emitter, indexed->local->local.var.item));
      break;
    default:
      unreachable();
    }
    break;
  }
  }
}

static void emit_load(emitter_t *emitter, const char *reg, const ir_operand_t *operand)
{
  switch (operand->kind) {
  case IR_OPERAND_CONSTANT: {
    emit_load_constant(emitter, reg, operand->operand.constant.constant);
    break;
  }
  case IR_OPERAND_PLACE: {
    emit_load_place(emitter, reg, operand->operand.place.place);
    break;
  }
  default:
    unreachable();
  }
}

static void emit_store(emitter_t *emitter, const char *reg, const ir_place_t *place)
{
  switch (place->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *plain = (ir_place_plain_t *) place;
    switch (plain->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "ST", "%s, %s", reg, item_label(emitter, plain->local->local.var.item));
      break;
    case IR_LOCAL_ARG:
      emit_inst(emitter, "LD", "GR7, %s", item_label(emitter, plain->local->local.arg.item));
      emit_inst(emitter, "ST", "%s, 0, GR7", reg);
      break;
    case IR_LOCAL_TEMP:
      emit_inst(emitter, "PUSH", "0, %s", reg);
      break;
    default:
      unreachable();
    }
    break;
  }
  case IR_PLACE_KIND_INDEXED: {
    ir_place_indexed_t *indexed = (ir_place_indexed_t *) place;
    emit_load(emitter, "GR7", indexed->index);
    emit_range_check(emitter, "GR7", indexed->local);
    switch (indexed->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "ST", "%s, %s, GR7", reg, item_label(emitter, indexed->local->local.var.item));
      break;
    default:
      unreachable();
    }
    break;
  }
  }
}

static void emit_stmt_assign(emitter_t *emitter, const ir_stmt_assign_t *stmt)
{
  switch (stmt->rhs->kind) {
  case IR_RVALUE_USE: {
    ir_rvalue_use_t *use = (ir_rvalue_use_t *) stmt->rhs;
    emit_load(emitter, "GR1", use->operand);
    break;
  }
  case IR_RVALUE_BINARY: {
    ir_rvalue_binary_t *binary = (ir_rvalue_binary_t *) stmt->rhs;
    emit_load(emitter, "GR2", binary->rhs);
    emit_load(emitter, "GR1", binary->lhs);
    switch (binary->kind) {
    case AST_EXPR_BINARY_KIND_PLUS:
      ++emitter->builtin.e_ov;
      emit_inst(emitter, "ADDA", "GR1, GR2");
      emit_inst(emitter, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_MINUS:
      ++emitter->builtin.e_ov;
      emit_inst(emitter, "SUBA", "GR1, GR2");
      emit_inst(emitter, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_STAR:
      ++emitter->builtin.e_ov;
      emit_inst(emitter, "MULA", "GR1, GR2");
      emit_inst(emitter, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_DIV:
      ++emitter->builtin.e_div0;
      emit_inst(emitter, "LD", "GR2, GR2");
      emit_inst(emitter, "JZE", "EDIV0");
      emit_inst(emitter, "DIVA", "GR1, GR2");
      break;
    case AST_EXPR_BINARY_KIND_EQUAL: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "CPA", "GR1, GR2");
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_inst(emitter, "JZE", "%s", jmp);
      emit_inst(emitter, "XOR", "GR1, GR1");
      emit_label(emitter, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_NOTEQ: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "SUBA", "GR1, GR2");
      emit_inst(emitter, "JZE", "%s", jmp);
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_label(emitter, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_LE: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "CPA", "GR1, GR2");
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_inst(emitter, "JMI", "%s", jmp);
      emit_inst(emitter, "XOR", "GR1, GR1");
      emit_label(emitter, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_LEEQ: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "CPA", "GR2, GR1");
      emit_inst(emitter, "XOR", "GR1, GR1");
      emit_inst(emitter, "JMI", "%s", jmp);
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_label(emitter, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_GR: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "CPA", "GR2, GR1");
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_inst(emitter, "JMI", "%s", jmp);
      emit_inst(emitter, "XOR", "GR1, GR1");
      emit_label(emitter, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_GREQ: {
      const char *jmp = item_label(emitter, NULL);
      emit_inst(emitter, "CPA", "GR1, GR2");
      emit_inst(emitter, "XOR", "GR1, GR1");
      emit_inst(emitter, "JMI", "%s", jmp);
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_label(emitter, jmp);
      break;
    }
    default:
      unreachable();
    }
    break;
  }
  case IR_RVALUE_NOT: {
    ir_rvalue_not_t *not_ = (ir_rvalue_not_t *) stmt->rhs;
    emit_load(emitter, "GR1", not_->value);
    emit_inst(emitter, "XOR", "GR1, BC1");
    break;
  }
  case IR_RVALUE_CAST: {
    ir_rvalue_cast_t *cast = (ir_rvalue_cast_t *) stmt->rhs;
    emit_load(emitter, "GR1", cast->value);
    switch (ir_operand_type(cast->value)->kind) {
    case TYPE_INTEGER:
      switch (cast->type->kind) {
      case TYPE_BOOLEAN: {
        const char *jmp = item_label(emitter, NULL);
        emit_inst(emitter, "LD", "GR1, GR1");
        emit_inst(emitter, "JZE", "%s", jmp);
        emit_inst(emitter, "LAD", "GR1, 1");
        emit_label(emitter, jmp);
        break;
      }
      case TYPE_INTEGER:
      case TYPE_CHAR:
        emit_inst(emitter, "LAD", "GR2, #007f");
        emit_inst(emitter, "AND", "GR1, GR2");
        break;
      default:
        unreachable();
      }
      break;
    case TYPE_CHAR:
      switch (cast->type->kind) {
      case TYPE_BOOLEAN: {
        const char *jmp = item_label(emitter, NULL);
        emit_inst(emitter, "LD", "GR1, GR1");
        emit_inst(emitter, "JZE", "%s", jmp);
        emit_inst(emitter, "LAD", "GR1, 1");
        emit_label(emitter, jmp);
        break;
      }
      case TYPE_INTEGER:
      case TYPE_CHAR:
        /* do nothing */
        break;
      default:
        unreachable();
      }
      break;
    case TYPE_BOOLEAN:
      /* do nothing */
      break;
    default:
      unreachable();
    }
    break;
  }
  }
  emit_store(emitter, "GR1", stmt->lhs);
}

static void emit_stmt_call(emitter_t *emitter, const ir_stmt_call_t *stmt)
{
  switch (stmt->func->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *func = (ir_place_plain_t *) stmt->func;
    switch (func->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "CALL", "%s", item_label(emitter, func->local->local.var.item->body));
      break;
    default:
      unreachable();
    }
    break;
  default:
    unreachable();
    break;
  }
  }
}

static void emit_push_constant_address(emitter_t *emitter, const ir_constant_t *constant)
{
  switch (constant->kind) {
  case IR_CONSTANT_NUMBER: {
    addr_t tmp   = addr_of(emitter, NULL);
    addr_t label = addr_of(emitter, NULL);
    emit_inst(emitter, "JUMP", addr_label(label));
    emit_label(emitter, addr_label(tmp));
    emit_inst(emitter, "DS", "1");
    emit_label(emitter, addr_label(label));
    emit_inst(emitter, "LAD", "GR0, %ld", constant->constant.number.value);
    emit_inst(emitter, "ST", "GR0, %s", addr_label(tmp));
    emit_inst(emitter, "PUSH", "%s", addr_label(tmp));
    break;
  }
  case IR_CONSTANT_CHAR: {
    addr_t tmp   = addr_of(emitter, NULL);
    addr_t label = addr_of(emitter, NULL);
    emit_inst(emitter, "JUMP", addr_label(label));
    emit_label(emitter, addr_label(tmp));
    emit_inst(emitter, "DS", "1");
    emit_label(emitter, addr_label(label));
    emit_inst(emitter, "LAD", "GR0, #%04x", constant->constant.char_.value);
    emit_inst(emitter, "ST", "GR0, %s", addr_label(tmp));
    emit_inst(emitter, "PUSH", "%s", addr_label(tmp));
    break;
  }
  case IR_CONSTANT_BOOLEAN: {
    addr_t tmp   = addr_of(emitter, NULL);
    addr_t label = addr_of(emitter, NULL);
    emit_inst(emitter, "JUMP", addr_label(label));
    emit_label(emitter, addr_label(tmp));
    emit_inst(emitter, "DS", "1");
    emit_label(emitter, addr_label(label));
    emit_inst(emitter, "LAD", "GR0, %ld", constant->constant.boolean.value);
    emit_inst(emitter, "ST", "GR0, %s", addr_label(tmp));
    emit_inst(emitter, "PUSH", "%s", addr_label(tmp));
    break;
  }
  default:
    unreachable();
  }
}

static void emit_push_place_address(emitter_t *emitter, const ir_place_t *place)
{
  switch (place->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *plain = (ir_place_plain_t *) place;
    switch (plain->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "PUSH", "%s", item_label(emitter, plain->local->local.var.item));
      break;
    case IR_LOCAL_ARG:
      emit_inst(emitter, "LD", "GR7, %s", item_label(emitter, plain->local->local.arg.item));
      emit_inst(emitter, "PUSH", "0, GR7");
      break;
    case IR_LOCAL_TEMP: {
      addr_t tmp   = addr_of(emitter, NULL);
      addr_t label = addr_of(emitter, NULL);
      emit_inst(emitter, "JUMP", "%s", addr_label(label));
      emit_label(emitter, addr_label(tmp));
      emit_inst(emitter, "DS", "1");
      emit_label(emitter, addr_label(label));
      emit_inst(emitter, "POP", "GR1");
      emit_inst(emitter, "ST", "GR1, %s", addr_label(tmp));
      emit_inst(emitter, "PUSH", "%s", addr_label(tmp));
      break;
    }
    default:
      unreachable();
    }
    break;
  }
  case IR_PLACE_KIND_INDEXED: {
    ir_place_indexed_t *indexed = (ir_place_indexed_t *) place;
    emit_load(emitter, "GR7", indexed->index);
    emit_range_check(emitter, "GR7", indexed->local);
    switch (indexed->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "PUSH", "%s, GR7", item_label(emitter, indexed->local->local.var.item));
      break;
    default:
      unreachable();
    }
    break;
  }
  }
}

static void emit_push_operand_address(emitter_t *emitter, const ir_operand_t *operand)
{
  switch (operand->kind) {
  case IR_OPERAND_CONSTANT:
    emit_push_constant_address(emitter, operand->operand.constant.constant);
    break;
  case IR_OPERAND_PLACE:
    emit_push_place_address(emitter, operand->operand.place.place);
    break;
  default:
    unreachable();
  }
}

static void emit_stmt_read(emitter_t *emitter, const ir_stmt_read_t *stmt)
{
  switch (stmt->ref->kind) {
  case IR_PLACE_KIND_PLAIN: {
    ir_place_plain_t *plain = (ir_place_plain_t *) stmt->ref;
    switch (plain->local->kind) {
    case IR_LOCAL_VAR:
      emit_inst(emitter, "LAD", "GR7, %s", item_label(emitter, plain->local->local.var.item));
      break;
    case IR_LOCAL_ARG:
      emit_inst(emitter, "LD", "GR7, %s", item_label(emitter, plain->local->local.arg.item));
      break;
    default:
      unreachable();
    }
    break;
  }
  case IR_PLACE_KIND_INDEXED: {
    ir_place_indexed_t *indexed = (ir_place_indexed_t *) stmt->ref;
    switch (indexed->local->kind) {
    case IR_LOCAL_VAR:
      emit_load(emitter, "GR7", stmt->ref->place.indexed.index);
      break;
    default:
      unreachable();
    }
    break;
  }
  }

  /* call builtin `read` functions for each types */
  switch (ir_place_type(stmt->ref)->kind) {
  case TYPE_INTEGER:
    emitter->builtin.r_int++;
    emit_inst(emitter, "CALL", "BRINT");
    break;
  case TYPE_CHAR:
    emitter->builtin.r_char++;
    emit_inst(emitter, "CALL", "BRCHAR");
    break;
  default:
    unreachable();
  }
}

static void emit_stmt_write(emitter_t *emitter, const ir_stmt_write_t *stmt)
{
  /* call builtin `write` functions for each types */
  switch (ir_operand_type(stmt->value)->kind) {
  case TYPE_INTEGER:
    emitter->builtin.w_int++;
    emit_load(emitter, "GR1", stmt->value);
    emit_inst(emitter, "CALL", "BSINT");
    if (stmt->len) {
      emit_load_constant(emitter, "GR1", stmt->len);
    } else {
      emit_inst(emitter, "LD", "GR1, GR2");
    }
    emit_inst(emitter, "CALL", "BWSTR");
    break;
  case TYPE_BOOLEAN:
    emitter->builtin.w_bool++;
    emit_load(emitter, "GR1", stmt->value);
    emit_inst(emitter, "CALL", "BSBOOL");
    if (stmt->len) {
      emit_load_constant(emitter, "GR1", stmt->len);
    } else {
      emit_inst(emitter, "LD", "GR1, GR2");
    }
    emit_inst(emitter, "CALL", "BWSTR");
    break;
  case TYPE_CHAR:
    emitter->builtin.w_char++;
    emit_load(emitter, "GR1", stmt->value);
    emit_inst(emitter, "CALL", "BSCHAR");
    if (stmt->len) {
      emit_load_constant(emitter, "GR1", stmt->len);
    } else {
      emit_inst(emitter, "LAD", "GR1, 1");
    }
    emit_inst(emitter, "CALL", "BWSTR");
    break;
  case TYPE_ARRAY: {
    const ir_constant_t *constant = stmt->value->operand.constant.constant;
    const type_t        *type     = ir_constant_type(constant);
    emitter->builtin.w_str++;
    emit_inst(emitter, "LAD", "GR2, %ld", type->type.array.size);
    emit_inst(emitter, "LAD", "GR3, %s", item_label(emitter, constant));
    emit_inst(emitter, "LD", "GR1, GR2");
    emit_inst(emitter, "CALL", "BWSTR");
    break;
  }
  default:
    unreachable();
  }
}

static void emit_stmt(emitter_t *emitter, const ir_stmt_t *stmt)
{
  for (; stmt; stmt = stmt->next) {
    switch (stmt->kind) {
    case IR_STMT_ASSIGN:
      emit_stmt_assign(emitter, (ir_stmt_assign_t *) stmt);
      break;
    case IR_STMT_CALL:
      emit_stmt_call(emitter, (ir_stmt_call_t *) stmt);
      break;
    case IR_STMT_READ:
      emit_stmt_read(emitter, (ir_stmt_read_t *) stmt);
      break;
    case IR_STMT_READLN:
      emitter->builtin.r_ln++;
      emit_inst(emitter, "CALL", "BRLN");
      break;
    case IR_STMT_WRITE:
      emit_stmt_write(emitter, (ir_stmt_write_t *) stmt);
      break;
    case IR_STMT_WRITELN:
      emitter->builtin.w_char++;
      emit_inst(emitter, "LD", "GR1, BCLF");
      emit_inst(emitter, "CALL", "BSCHAR");
      emit_inst(emitter, "LAD", "GR1, 1");
      emit_inst(emitter, "CALL", "BWSTR");
      break;
    }
  }
}

static void emit_block(emitter_t *emitter, const ir_block_t *block)
{
  emit_label(emitter, item_label(emitter, block));
  emit_stmt(emitter, block->stmt);
  switch (block->termn->kind) {
  case IR_TERMN_GOTO: {
    ir_termn_goto_t *goto_ = (ir_termn_goto_t *) block->termn;
    if (lookup_addr(emitter, goto_->next)) {
      emit_inst(emitter, "JUMP", "%s", item_label(emitter, goto_->next));
    } else {
      emit_block(emitter, goto_->next);
    }
    break;
  }
  case IR_TERMN_IF: {
    ir_termn_if_t *if_ = (ir_termn_if_t *) block->termn;
    emit_load(emitter, "GR1", if_->cond);
    emit_inst(emitter, "LD", "GR1, GR1");
    if (lookup_addr(emitter, if_->then)) {
      emit_inst(emitter, "JNZ", "%s", item_label(emitter, if_->then));
      if (lookup_addr(emitter, if_->els)) {
        emit_inst(emitter, "JUMP", "%s", item_label(emitter, if_->els));
      } else {
        emit_block(emitter, if_->els);
      }
    } else {
      if (lookup_addr(emitter, if_->els)) {
        emit_inst(emitter, "JZE", "%s", item_label(emitter, if_->els));
        emit_block(emitter, if_->then);
      } else {
        emit_inst(emitter, "JZE", "%s", item_label(emitter, if_->els));
        emit_block(emitter, if_->then);
        emit_block(emitter, if_->els);
      }
    }
    break;
  }
  case IR_TERMN_RETURN:
    emit_inst(emitter, "RET", NULL);
    break;
  case IR_TERMN_ARG: {
    ir_termn_arg_t *arg = (ir_termn_arg_t *) block->termn;
    emit_push_operand_address(emitter, arg->arg);
    emit_block(emitter, arg->next);
    break;
  }
  default:
    unreachable();
  }
}

static void emit_item(emitter_t *emitter, const ir_item_t *item)
{
  while (item) {
    switch (item->kind) {
    case IR_ITEM_PROGRAM:
      emit_inst(emitter, "CALL", item_label(emitter, item->body->inner));
      emit_inst(emitter, "SVC", "0");
      emit_item(emitter, item->body->items);
      emit_block(emitter, item->body->inner);
      break;
    case IR_ITEM_PROCEDURE:
      emit_item(emitter, item->body->items);
      emit_label(emitter, item_label(emitter, item->body));
      {
        ir_item_t *items = item->body->items;
        emit_inst(emitter, "POP", "GR2");
        while (items) {
          if (items->kind == IR_ITEM_ARG_VAR) {
            emit_inst(emitter, "POP", "GR1");
            emit_inst(emitter, "ST", "GR1, %s", item_label(emitter, items));
          }
          items = items->next;
        }
        emit_inst(emitter, "PUSH", "0, GR2");
        emit_block(emitter, item->body->inner);
      }
      break;
    case IR_ITEM_VAR:
    case IR_ITEM_LOCAL_VAR:
      emit_label(emitter, item_label(emitter, item));
      switch (item->type->kind) {
      case TYPE_INTEGER:
      case TYPE_BOOLEAN:
      case TYPE_CHAR:
        emit_inst(emitter, "DS", "1");
        break;
      case TYPE_ARRAY:
        emit_inst(emitter, "DS", "%ld", item->type->type.array.size);
        break;
      default:
        unreachable();
      }
      break;
    case IR_ITEM_ARG_VAR:
      emit_label(emitter, item_label(emitter, item));
      switch (item->type->kind) {
      case TYPE_INTEGER:
      case TYPE_BOOLEAN:
      case TYPE_CHAR:
        emit_inst(emitter, "DS", "1");
        break;
      default:
        unreachable();
      }
      break;
    }
    item = item->next;
  }
}

static void emit_builtin(emitter_t *emitter)
{
  int builtin_write = 0;
  int builtin_read  = 0;
  emit_label(emitter, "BCSP");
  emit_inst(emitter, "DC", "#%04X", (int) ' ');
  emit_label(emitter, "BCLF");
  emit_inst(emitter, "DC", "#%04X", (int) '\n');
  emit_label(emitter, "BCTAB");
  emit_inst(emitter, "DC", "#%04X", (int) '\t');
  emit_label(emitter, "BC1");
  emit_inst(emitter, "DC", "1");
  emit_label(emitter, "BC10");
  emit_inst(emitter, "DC", "10");
  emit_label(emitter, "BCH30");
  emit_inst(emitter, "DC", "#0030");

  if (emitter->builtin.w_int) {
    builtin_write = 1;
    emit_label(emitter, "BSINT");
    emit_inst(emitter, "LAD", "GR4, 6");
    emit_inst(emitter, "XOR", "GR5, GR5");
    emit_inst(emitter, "CPA", "GR1, GR5");
    emit_inst(emitter, "JPL", "BSINT0");
    emit_inst(emitter, "SUBA", "GR5, GR1");
    emit_inst(emitter, "LD", "GR1, GR5");

    emit_label(emitter, "BSINT0");
    emit_inst(emitter, "LD", "GR2, GR1");
    emit_inst(emitter, "LD", "GR3, GR1");
    emit_inst(emitter, "DIVA", "GR3, BC10");
    emit_inst(emitter, "MULA", "GR3, BC10");
    emit_inst(emitter, "SUBA", "GR2, GR3");
    emit_inst(emitter, "ADDA", "GR2, BCH30");
    emit_inst(emitter, "SUBA", "GR4, BC1");
    emit_inst(emitter, "ST", "GR2, BSBUF, GR4");
    emit_inst(emitter, "DIVA", "GR1, BC10");
    emit_inst(emitter, "JNZ", "BSINT0");
    emit_inst(emitter, "LD", "GR5, GR5");
    emit_inst(emitter, "JZE", "BSINT1");
    emit_inst(emitter, "LAD", "GR2, #%04X", (int) '-');
    emit_inst(emitter, "SUBA", "GR4, BC1");
    emit_inst(emitter, "ST", "GR2, BSBUF, GR4");

    emit_label(emitter, "BSINT1");
    emit_inst(emitter, "LAD", "GR2, 6");
    emit_inst(emitter, "SUBA", "GR2, GR4");
    emit_inst(emitter, "LAD", "GR3, BSBUF, GR4");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.w_bool) {
    builtin_write = 1;
    emit_label(emitter, "BCTRUE");
    emit_inst(emitter, "DC", "'TRUE'");
    emit_label(emitter, "BCFALSE");
    emit_inst(emitter, "DC", "'FALSE'");

    emit_label(emitter, "BSBOOL");
    emit_inst(emitter, "LD", "GR1, GR1");
    emit_inst(emitter, "JNZ", "BSBOOL0");
    emit_inst(emitter, "LAD", "GR3, BCFALSE");
    emit_inst(emitter, "LAD", "GR2, 5");
    emit_inst(emitter, "RET", NULL);
    emit_label(emitter, "BSBOOL0");
    emit_inst(emitter, "LAD", "GR3, BCTRUE");
    emit_inst(emitter, "LAD", "GR2, 4");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.w_char) {
    builtin_write = 1;
    emit_label(emitter, "BSCHAR");
    emit_inst(emitter, "ST", "GR1, BSBUF");
    emit_inst(emitter, "LAD", "GR3, BSBUF");
    emit_inst(emitter, "LAD", "GR2, 1");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.w_str) {
    builtin_write = 1;
  }

  if (builtin_write) {
    emit_label(emitter, "BSBUF");
    emit_inst(emitter, "DS", "6");
    emit_label(emitter, "BOBUF");
    emit_inst(emitter, "DS", "256");
    emit_label(emitter, "BOCUR");
    emit_inst(emitter, "DC", "0");

    emit_label(emitter, "BFLUSH");
    emit_inst(emitter, "OUT", "BOBUF, BOCUR");
    emit_inst(emitter, "XOR", "GR0, GR0");
    emit_inst(emitter, "ST", "GR0, BOCUR");
    emit_inst(emitter, "RET", NULL);

    emit_label(emitter, "BWSTR");
    emit_inst(emitter, "LD", "GR1, GR1");
    emit_inst(emitter, "JPL", "BWSTR0");
    emit_inst(emitter, "RET", NULL);

    emit_label(emitter, "BWSTR0");
    emit_inst(emitter, "CPA", "GR2, GR1");
    emit_inst(emitter, "JMI", "BWSTR1");
    emit_inst(emitter, "LD", "GR4, 0, GR3");
    emit_inst(emitter, "ADDA", "GR3, BC1");
    emit_inst(emitter, "JUMP", "BWSTR2");

    emit_label(emitter, "BWSTR1");
    emit_inst(emitter, "LD", "GR4, BCSP");

    emit_label(emitter, "BWSTR2");
    emit_inst(emitter, "SUBA", "GR1, BC1");
    emit_inst(emitter, "LD", "GR5, BOCUR");
    emit_inst(emitter, "ST", "GR4, BOBUF, GR5");
    emit_inst(emitter, "ADDA", "GR5, BC1");
    emit_inst(emitter, "ST", "GR5, BOCUR");
    emit_inst(emitter, "CPA", "GR4, BCLF");
    emit_inst(emitter, "JNZ", "BWSTR3");
    emit_inst(emitter, "CALL", "BFLUSH");
    emit_inst(emitter, "JUMP", "BWSTR4");

    emit_label(emitter, "BWSTR3");
    emit_inst(emitter, "LAD", "GR4, 256");
    emit_inst(emitter, "CPA", "GR5, GR4");
    emit_inst(emitter, "JNZ", "BWSTR4");
    emit_inst(emitter, "CALL", "BFLUSH");

    emit_label(emitter, "BWSTR4");
    emit_inst(emitter, "JUMP", "BWSTR");
  }

  if (emitter->builtin.r_int) {
    builtin_read = 1;
    emit_label(emitter, "BRINT");
    emit_inst(emitter, "XOR", "GR0, GR0");
    emit_inst(emitter, "CALL", "BRREAD");

    emit_label(emitter, "BRINT0");
    emit_inst(emitter, "CALL", "BRTOP");
    emit_inst(emitter, "CPA", "GR1, BCSP");
    emit_inst(emitter, "JZE", "BRINT1");
    emit_inst(emitter, "CPA", "GR1, BCLF");
    emit_inst(emitter, "JZE", "BRINT1");
    emit_inst(emitter, "CPA", "GR1, BCTAB");
    emit_inst(emitter, "JZE", "BRINT1");
    emit_inst(emitter, "SUBA", "GR1, BCH30");
    emit_inst(emitter, "JMI", "BRINT2");
    emit_inst(emitter, "CPA", "GR1, BC10");
    emit_inst(emitter, "JPL", "BRINT2");
    emit_inst(emitter, "MULA", "GR0, BC10");
    emit_inst(emitter, "JOV", "EOV");
    emit_inst(emitter, "ADDA", "GR0, GR1");

    emit_label(emitter, "BRINT1");
    emit_inst(emitter, "ADDA", "GR2, BC1");
    emit_inst(emitter, "ST", "GR2, BICUR");
    emit_inst(emitter, "CPA", "GR2, BILEN");
    emit_inst(emitter, "JMI", "BRINT0");

    emit_label(emitter, "BRINT2");
    emit_inst(emitter, "ST", "GR0, 0, GR7");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.r_char) {
    builtin_read = 1;
    emit_label(emitter, "BRCHAR");
    emit_inst(emitter, "CALL", "BRREAD");
    emit_inst(emitter, "CALL", "BRTOP");
    emit_inst(emitter, "ADDA", "GR2, BC1");
    emit_inst(emitter, "ST", "GR2, BICUR");
    emit_inst(emitter, "ST", "GR1, 0, GR7");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.r_ln) {
    builtin_read = 1;
    emit_label(emitter, "BRLN");
    emit_inst(emitter, "XOR", "GR0, GR0");
    emit_inst(emitter, "ST", "GR0, BILEN");
    emit_inst(emitter, "ST", "GR0, BICUR");
    emit_inst(emitter, "RET", NULL);
  }

  if (builtin_read) {
    emit_label(emitter, "BIBUF");
    emit_inst(emitter, "DS", "256");
    emit_label(emitter, "BILEN");
    emit_inst(emitter, "DC", "0");
    emit_label(emitter, "BICUR");
    emit_inst(emitter, "DC", "0");

    emit_label(emitter, "BRREAD");
    emit_inst(emitter, "LD", "GR1, BICUR");
    emit_inst(emitter, "LD", "GR2, BICUR");
    emit_inst(emitter, "CPA", "GR1, BILEN");
    emit_inst(emitter, "JMI", "BRREAD0");
    emit_inst(emitter, "IN", "BIBUF, BILEN");
    emit_inst(emitter, "XOR", "GR0, GR0");
    emit_inst(emitter, "ST", "GR0, BICUR");

    emit_label(emitter, "BRREAD0");
    emit_inst(emitter, "RET", NULL);

    emit_label(emitter, "BRTOP");
    emit_inst(emitter, "LD", "GR1, BICUR");
    emit_inst(emitter, "LD", "GR2, BICUR");
    emit_inst(emitter, "CPA", "GR1, BILEN");
    emit_inst(emitter, "JMI", "BRTOP0");
    emit_inst(emitter, "XOR", "GR1, GR1");
    emit_inst(emitter, "RET", NULL);

    emit_label(emitter, "BRTOP0");
    emit_inst(emitter, "LD", "GR1, BIBUF, GR2");
    emit_inst(emitter, "RET", NULL);
  }

  if (emitter->builtin.e_ov) {
    const char *msg = "runtime error: overflow";
    emit_label(emitter, "EMOV");
    emit_inst(emitter, "DC", "'%s'", msg);
    emit_label(emitter, "EMLOV");
    emit_inst(emitter, "DC", "%ld", strlen(msg));
    emit_label(emitter, "EOV");
    emit_inst(emitter, "CALL", "BFLUSH");
    emit_inst(emitter, "OUT", "EMOV, EMLOV");
    emit_inst(emitter, "OUT", "BCLF, BC1");
    emit_inst(emitter, "SVC", "1");
  }

  if (emitter->builtin.e_div0) {
    const char *msg = "runtime error: division by 0";
    emit_label(emitter, "EMDIV0");
    emit_inst(emitter, "DC", "'%s'", msg);
    emit_label(emitter, "EMLDIV0");
    emit_inst(emitter, "DC", "%ld", strlen(msg));
    emit_label(emitter, "EDIV0");
    emit_inst(emitter, "CALL", "BFLUSH");
    emit_inst(emitter, "OUT", "EMDIV0, EMLDIV0");
    emit_inst(emitter, "OUT", "BCLF, BC1");
    emit_inst(emitter, "SVC", "2");
  }

  if (emitter->builtin.e_rng) {
    const char *msg = "runtime error: index out of range";
    emit_label(emitter, "EMRNG");
    emit_inst(emitter, "DC", "'%s'", msg);
    emit_label(emitter, "EMLRNG");
    emit_inst(emitter, "DC", "%ld", strlen(msg));
    emit_label(emitter, "ERNG");
    emit_inst(emitter, "CALL", "BFLUSH");
    emit_inst(emitter, "OUT", "EMRNG, EMLRNG");
    emit_inst(emitter, "OUT", "BCLF, BC1");
    emit_inst(emitter, "SVC", "3");
  }
}

static void emit_ir(emitter_t *codegen, const ir_t *ir)
{
  emit_label(codegen, "PROGRAM");
  emit_inst(codegen, "START", NULL);
  emit_item(codegen, ir->items);
  emit_builtin(codegen);
  emit_constant(codegen, ir->constants);
  emit_inst(codegen, "END", NULL);
}

void codegen_casl2(const ir_t *ir)
{
  emitter_t codegen;
  assert(ir);

  codegen.file           = fopen(ir->source->out_name, "w");
  codegen.addr.cnt       = 0;
  codegen.addr.table     = hash_new(hash_default_comp, hash_default_hasher);
  codegen.label[0]       = '\0';
  codegen.builtin.e_rng  = 0;
  codegen.builtin.e_ov   = 0;
  codegen.builtin.e_div0 = 0;
  codegen.builtin.r_int  = 0;
  codegen.builtin.r_char = 0;
  codegen.builtin.r_ln   = 0;
  codegen.builtin.w_int  = 0;
  codegen.builtin.w_char = 0;
  codegen.builtin.w_bool = 0;
  emit_ir(&codegen, ir);
  hash_delete(codegen.addr.table, NULL, NULL);
  fclose(codegen.file);
}
