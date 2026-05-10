/*
 * unit.c -- compilation unit
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include "unit.h"
#include "ds.h"
#include "sym.h"
#include "syn.h"

size_t const UNIT_INDEX_NULL = -1ul;

unit_item_ref_t const UNIT_ITEM_REF_NULL = { UNIT_INDEX_NULL };
unit_scope_ref_t const UNIT_SCOPE_REF_NULL = { UNIT_INDEX_NULL };

void unit_init(struct unit *unit)
{
  vec_init(&unit->items);
  vec_init(&unit->scopes);
  ht_init(&unit->usages, syn_hash, syn_eq);
  vec_init(&unit->unresolved);
}

void unit_deinit(struct unit *unit)
{
  size_t i;

  for (i = 0; i < unit->items.count; ++i) {
    struct unit_item *item = &unit->items.data[i];
    vec_deinit(&item->children);
    vec_deinit(&item->users);
  }
  vec_deinit(&unit->items);

  for (i = 0; i < unit->scopes.count; ++i) {
    struct unit_scope *scope = &unit->scopes.data[i];
    vec_deinit(&scope->items);
    vec_deinit(&scope->children);
  }
  vec_deinit(&unit->scopes);

  ht_deinit(&unit->usages);
  vec_deinit(&unit->unresolved);
}

unit_item_ref_t unit_add_item(struct unit *unit, struct sym const *name, enum unit_item_kind kind, struct syn_node const *ident, struct syn_node const *decl, unit_item_ref_t parent)
{
  struct ht_entry e;
  unit_item_ref_t ref;

  struct unit_item item;
  item.name = name;
  item.kind = kind;
  item.ident = ident;
  item.decl = decl;
  item.parent = parent;
  vec_init(&item.children);
  vec_init(&item.users);

  ref.index = unit->items.count;
  vec_push(&unit->items, &item);

  if (parent.index != UNIT_INDEX_NULL) {
    struct unit_item *x = vec_at(&unit->items, parent.index);
    vec_push(&x->children, &ref);
  }

  ht_entry(&unit->defs, &ident, &e);
  ht_occupy(&unit->defs, &e, &ident);
  ht_at(&unit->defs, &e)->value = ref;

  return ref;
}

unit_scope_ref_t unit_add_scope(struct unit *unit, unit_scope_ref_t parent)
{
  unit_scope_ref_t ref;

  struct unit_scope scope;
  vec_init(&scope.items);
  scope.parent = parent;
  vec_init(&scope.children);

  ref.index = unit->scopes.count;
  vec_push(&unit->scopes, &scope);

  if (parent.index != UNIT_INDEX_NULL) {
    struct unit_scope *x = vec_at(&unit->scopes, parent.index);
    vec_push(&x->children, &ref);
  }

  return ref;
}

struct unit_item const *unit_get_item(struct unit const *unit, unit_item_ref_t ref)
{
  return &unit->items.data[ref.index];
}

struct unit_scope const *unit_get_scope(struct unit const *unit, unit_scope_ref_t ref)
{
  return &unit->scopes.data[ref.index];
}

void unit_add_unresolved(struct unit *unit, struct syn_node const *node)
{
  vec_push(&unit->unresolved, &node);
}

void unit_populate(struct unit *unit, unit_scope_ref_t scope, unit_item_ref_t item)
{
  struct unit_scope *x = vec_at(&unit->scopes, scope.index);
  vec_push(&x->items, &item);
}

void unit_add_usage(struct unit *unit, struct syn_node const *ident, unit_item_ref_t item)
{
  struct ht_entry e;
  struct unit_item *x = vec_at(&unit->items, item.index);

  ht_entry(&unit->usages, &ident, &e);
  ht_occupy(&unit->usages, &e, &ident);
  ht_at(&unit->usages, &e)->value = item;
  vec_push(&x->users, &ident);
}

struct unit_item const *unit_get_usage(struct unit const *unit, struct syn_node const *node)
{
  struct ht_entry e;
  if (ht_entry(&unit->usages, &node, &e)) {
    unit_item_ref_t ref = ht_at(&unit->usages, &e)->value;
    return unit_get_item(unit, ref);
  } else {
    return NULL;
  }
}

struct unit_item const *unit_get_def(struct unit *unit, struct syn_node const *ident)
{
  struct ht_entry e;
  if (ht_entry(&unit->defs, &ident, &e)) {
    unit_item_ref_t ref = ht_at(&unit->defs, &e)->value;
    return unit_get_item(unit, ref);
  } else {
    return NULL;
  }
}
