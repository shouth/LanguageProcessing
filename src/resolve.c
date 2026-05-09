/*
 * resolve.c -- name resolution
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "diag.h"
#include "driver.h"
#include "ds.h"
#include "syn.h"
#include "unit.h"

struct scope_entity {
  vec(unit_item_ref_t) items;
};

struct scope {
  struct scope *parent;
  unit_scope_ref_t scope;
  hm(struct sym const *, struct scope_entity) entities;
  vec(struct syn_tok const *) refs;
};

struct resolver {
  struct syn_program const *syn;
  struct diag *diag;
  struct file const *file;
  struct unit *unit;

  struct scope *scope;
  int error;
};

static void init(struct resolver *r, struct diag *diag, struct file const *file, struct syn_program const *syn, struct unit *unit)
{
  r->syn = syn;
  r->diag = diag;
  r->file = file;
  r->unit = unit;

  r->error = 0;
}

static void push(struct resolver *r)
{
  struct scope *scope = malloc(sizeof(struct scope));
  scope->parent = r->scope;
  scope->scope = unit_add_scope(r->unit, r->scope ? r->scope->scope : UNIT_SCOPE_REF_NULL);
  ht_init(&scope->entities, sym_hash, sym_eq);
  vec_init(&scope->refs);
  r->scope = scope;
}

static void pop(struct resolver *r)
{
  size_t i, j;
  struct ht_entry e;
  struct scope *s;

  for (i = 0; i < r->scope->refs.count; ++i) {
    struct syn_tok const *ref = r->scope->refs.data[i];

    for (s = r->scope; s; s = s->parent) {
      if (ht_entry(&s->entities, &ref->text, &e)) {
        struct scope_entity *entity = &ht_at(&s->entities, &e)->value;
        if (entity->items.count == 1) {
          struct unit_item const *item = unit_get_item(r->unit, *vec_front(&entity->items));
          if (syn_text_off(item->ident) > syn_text_off(&ref->node)) {
            size_t off = syn_text_off(&ref->node) + syn_triv_text_len(&ref->node);
            size_t len = syn_text_off(&ref->node) - syn_triv_text_len(&ref->node);
            size_t decl_off = syn_text_off(item->ident) - syn_triv_text_len(item->ident);
            diag_error_use_before_decl(r->diag, r->file, off, len, decl_off);
            r->error = 1;
          }
          unit_use(r->unit, &ref->node, *vec_front(&entity->items));
        }
        break;
      }
    }
    if (!s) {
      if (r->scope->parent) {
        vec_push(&r->scope->parent->refs, &ref);
      } else {
        unit_add_unresolved(r->unit, &ref->node);
      }
    }
  }
  vec_deinit(&r->scope->refs);

  for (ht_entry(&r->scope->entities, NULL, &e); ht_next(&r->scope->entities, &e);) {
    struct scope_entity *entity = &ht_at(&r->scope->entities, &e)->value;
    if (entity->items.count > 1) {
      struct unit_item const *item;
      vec(size_t) offs;
      size_t off, len;

      vec_init(&offs);
      for (j = 0; j < entity->items.count - 1; ++j) {
        item = unit_get_item(r->unit, entity->items.data[j]);
        off = syn_text_off(item->ident) + syn_triv_text_len(item->ident);
        vec_push(&offs, &off);
      }
      item = unit_get_item(r->unit, *vec_back(&entity->items));
      off = syn_text_off(item->ident) + syn_triv_text_len(item->ident);
      len = syn_text_len(item->ident) - syn_triv_text_len(item->ident);

      diag_error_conflict(r->diag, r->file, off, len, item->name->str, offs.data, offs.count);
      r->error = 1;
      vec_deinit(&offs);
    }
    vec_deinit(&entity->items);
  }

  ht_deinit(&r->scope->entities);
  s = r->scope;
  r->scope = s->parent;
  free(s);
}

static unit_item_ref_t populate(struct resolver *r, struct sym const *name, enum unit_item_kind kind, struct syn_node const *ident, struct syn_node const *decl, unit_item_ref_t parent)
{
  struct ht_entry e;
  struct scope_entity *entity;
  
  unit_item_ref_t item = unit_add_item(r->unit, name, kind, ident, decl, parent);
  if (r->scope) {
    unit_populate(r->unit, r->scope->scope, item);

    if (!ht_entry(&r->scope->entities, &name, &e)) {
      struct scope_entity x;
      vec_init(&x.items);
      ht_occupy(&r->scope->entities, &e, &name);
    }
    entity = &ht_at(&r->scope->entities, &e)->value;
    vec_push(&entity->items, &item);
  }

  return item;
}

static void use(struct resolver *r, struct syn_tok *ref)
{
  vec_push(&r->scope->refs, &ref);
}

static void resolve_expr(struct resolver *r, struct any_syn_expr *expr)
{
  switch (any_syn_expr_kind(expr)) {
  case ANY_SYN_EXPR_ENTIRE_VAR: {
    struct syn_entire_var_expr *e = (struct syn_entire_var_expr *) expr;
    use(r, e->name);
    break;
  }

  case ANY_SYN_EXPR_IDX_VAR: {
    struct syn_idx_var_expr *e = (struct syn_idx_var_expr *) expr;
    use(r, e->name);
    resolve_expr(r, e->index);
    break;
  }

  case ANY_SYN_EXPR_PAREN: {
    struct syn_paren_expr *e = (struct syn_paren_expr *) expr;
    resolve_expr(r, e->expr);
    break;
  }

  case ANY_SYN_EXPR_CAST: {
    struct syn_cast_expr *e = (struct syn_cast_expr *) expr;
    resolve_expr(r, e->expr);
    break;
  }

  case ANY_SYN_EXPR_UNARY: {
    struct syn_unary_expr *e = (struct syn_unary_expr *) expr;
    resolve_expr(r, e->expr);
    break;
  }

  case ANY_SYN_EXPR_BINARY: {
    struct syn_binary_expr *e = (struct syn_binary_expr *) expr;
    resolve_expr(r, e->lhs);
    resolve_expr(r, e->rhs);
    break;
  }

  default:
    /* do nothing */
    break;
  }
}

static void resolve_args(struct resolver *r, struct syn_act_params *args)
{
  size_t i;
  for (i = 0; i < args->expr_list->count; ++i) {
    resolve_expr(r, args->expr_list->children[i]->expr);
  }
}

static void resolve_output(struct resolver *r, struct syn_output_values *args)
{
  size_t i;
  for (i = 0; i < args->output_value_list->count; ++i) {
    struct syn_output_value *value = args->output_value_list->children[i]->output_value;
    resolve_expr(r, value->expr);
    if (value->output_fmt) {
      resolve_expr(r, value->output_fmt->width);
    }
  }
}

static void resolve_stmt(struct resolver *r, struct any_syn_stmt *stmt)
{
  switch (any_syn_stmt_kind(stmt)) {
  case ANY_SYN_STMT_ASSIGN: {
    struct syn_assign_stmt *s = (struct syn_assign_stmt *) stmt;
    resolve_expr(r, s->lhs);
    resolve_expr(r, s->lhs);
    break;
  }

  case ANY_SYN_STMT_IF: {
    struct syn_if_stmt *s = (struct syn_if_stmt *) stmt;
    resolve_expr(r, s->cond);
    resolve_stmt(r, s->then_stmt);
    if (s->else_clause) {
      resolve_stmt(r, s->else_clause->else_stmt);
    }
    break;
  }

  case ANY_SYN_STMT_WHILE: {
    struct syn_while_stmt *s = (struct syn_while_stmt *) stmt;
    resolve_expr(r, s->cond);
    resolve_stmt(r, s->do_stmt);
    break;
  }

  case ANY_SYN_STMT_CALL: {
    struct syn_call_stmt *s = (struct syn_call_stmt *) stmt;
    use(r, s->name);
    if (s->act_params) {
      resolve_args(r, s->act_params);
    }
    break;
  }

  case ANY_SYN_STMT_INPUT: {
    struct syn_input_stmt *s = (struct syn_input_stmt *) stmt;
    if (s->act_params) {
      resolve_args(r, s->act_params);
    }
    break;
  }

  case ANY_SYN_STMT_OUTPUT: {
    struct syn_output_stmt *s = (struct syn_output_stmt *) stmt;
    if (s->output_values) {
      resolve_output(r, s->output_values);
    }
    break;
  }

  case ANY_SYN_STMT_COMP: {
    size_t i;
    struct syn_comp_stmt *s = (struct syn_comp_stmt *) stmt;
    for (i = 0; i < s->stmt_list->count; ++i) {
      resolve_stmt(r, s->stmt_list->children[i]->stmt);
    }
    break;
  }

  default:
    /* do nothing */
    break;
  }
}

static void resolve_var(struct resolver *r, struct syn_var_decl_part *var, enum unit_item_kind kind, unit_item_ref_t parent)
{
  size_t i, j;
  for (i = 0; i < var->var_decl_list->count; ++i) {
    struct syn_var_decl *var_decl = var->var_decl_list->children[i];
    for (j = 0; j < var_decl->ident_list->count; ++j) {
      struct syn_ident_list_item *item = var_decl->ident_list->children[j];
      populate(r, item->name->text, kind, &item->name->node, &var_decl->syn.node, parent);
    }
  }
}

static void resolve_params(struct resolver *r, struct syn_fml_params *params, unit_item_ref_t parent)
{
  size_t i, j;
  for (i = 0; i < params->fml_param_list->count; ++i) {
    struct syn_fml_param_sec *fml_param_sec = params->fml_param_list->children[i];
    for (j = 0; j < fml_param_sec->ident_list->count; ++j) {
      struct syn_ident_list_item *item = fml_param_sec->ident_list->children[j];
      populate(r, item->name->text, UNIT_ITEM_PARAM, &item->name->node, &fml_param_sec->syn.node, parent);
    }
  }
}

static void resolve_proc(struct resolver *r, struct syn_proc_decl_part *proc, unit_item_ref_t parent)
{
  unit_item_ref_t item = populate(r, proc->proc_decl_head->name->text, UNIT_ITEM_PROC, &proc->proc_decl_head->name->node, &proc->syn.node, parent);
  push(r);

  if (proc->proc_decl_head->fml_params) {
    resolve_params(r, proc->proc_decl_head->fml_params, item);
  }

  if (proc->var_decl_part) {
    resolve_var(r, proc->var_decl_part, UNIT_ITEM_LOCAL, item);
  }

  resolve_stmt(r, (struct any_syn_stmt *) proc->comp_stmt);
  pop(r);
}

static void resolve_program(struct resolver *r, struct syn_program *program)
{
  size_t i;
  
  unit_item_ref_t item = populate(r, program->name->text, UNIT_ITEM_PROGRAM, &program->name->node, &program->syn.node, UNIT_ITEM_REF_NULL);
  push(r);

  for (i = 0; i < program->block->decl_part_list->count; ++i) {
    struct any_syn_decl_part *decl_part = program->block->decl_part_list->children[i];
    switch (any_syn_decl_part_kind(decl_part)) {
    case ANY_SYN_DECL_PART_PROC:
      resolve_proc(r, (struct syn_proc_decl_part *) decl_part, item);
      break;

    case ANY_SYN_DECL_PART_VAR:
      resolve_var(r, (struct syn_var_decl_part *) decl_part, UNIT_ITEM_VAR, item);
      break;

    default:
      /* do nothing */
      break;
    }
  }

  resolve_stmt(r, (struct any_syn_stmt *) program->block->comp_stmt);
  pop(r);
}

struct q_resolve const *q_resolve(struct q_ctxt *q, struct file const *file)
{
  struct ht_entry e;
  if (!ht_entry(&q->resolve, &file, &e)) {
    struct resolver r;
    struct q_parse const *parse = q_parse(q, file);

    struct q_resolve resolve;
    resolve.file = file;
    resolve.unit = malloc(sizeof(struct unit));
    unit_init(resolve.unit);

    resolve.diag = malloc(sizeof(struct diag));
    diag_init(resolve.diag, parse->diag);
    init(&r, resolve.diag, file, parse->syn, resolve.unit);
    resolve_program(&r, parse->syn);

    if (parse->status == Q_OK && !r.error) {
      resolve.status = Q_OK;
    } else {
      resolve.status = Q_BAD_SYNTAX;
    }

    ht_occupy(&q->resolve, &e, &resolve);
  }
  return ht_at(&q->resolve, &e);
}
