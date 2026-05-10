/*
 * unit.h -- compilation unit
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UNIT_H
#define UNIT_H

#include <stddef.h>

#include "ds.h"
#include "sym.h"
#include "syn.h"

typedef struct { size_t index; } unit_item_ref_t;
typedef struct { size_t index; } unit_scope_ref_t;

extern unit_item_ref_t const UNIT_ITEM_REF_NULL;
extern unit_scope_ref_t const UNIT_SCOPE_REF_NULL;

enum unit_item_kind {
  UNIT_ITEM_PROGRAM,
  UNIT_ITEM_PROC,
  UNIT_ITEM_VAR,
  UNIT_ITEM_PARAM,
  UNIT_ITEM_LOCAL
};

struct unit_item {
  struct sym const *name;
  enum unit_item_kind kind;
  struct syn_node const *ident;
  struct syn_node const *decl;
  unit_item_ref_t parent;
  vec(unit_item_ref_t) children;
  vec(struct syn_node const *) users;
};

struct unit_scope {
  vec(unit_item_ref_t) items;
  unit_scope_ref_t parent;
  vec(unit_scope_ref_t) children;
};

struct unit {
  vec(struct unit_item) items;
  vec(struct unit_scope) scopes;
  hm(struct syn_node const *, unit_item_ref_t) usages;
  hm(struct syn_node const *, unit_item_ref_t) defs;
  vec(struct syn_node const *) unresolved;
};

void unit_init(struct unit *unit);

void unit_deinit(struct unit *unit);

unit_item_ref_t unit_add_item(struct unit *unit, struct sym const *name, enum unit_item_kind kind, struct syn_node const *ident, struct syn_node const *decl, unit_item_ref_t parent);

unit_scope_ref_t unit_add_scope(struct unit *unit, unit_scope_ref_t parent);

struct unit_item const *unit_get_item(struct unit const *unit, unit_item_ref_t ref);

struct unit_scope const *unit_get_scope(struct unit const *unit, unit_scope_ref_t ref);

void unit_add_unresolved(struct unit *unit, struct syn_node const *node);

void unit_populate(struct unit *unit, unit_scope_ref_t scope, unit_item_ref_t item);

void unit_add_usage(struct unit *unit, struct syn_node const *ident, unit_item_ref_t item);

struct unit_item const *unit_get_usage(struct unit const *unit, struct syn_node const *ident);

struct unit_item const *unit_get_def(struct unit *unit, struct syn_node const *ident);

#endif /* UNIT_H */
