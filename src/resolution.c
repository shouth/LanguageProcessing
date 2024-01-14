#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "resolution.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "utility.h"

struct Res {
  Map *node_to_def;
  Map *ref_to_def;
};

Res *res_new(void)
{
  Res *res         = xmalloc(sizeof(Res));
  res->node_to_def = map_new(NULL, NULL);
  res->ref_to_def  = map_new(NULL, NULL);
  return res;
}

void res_free(Res *res)
{
  if (res) {
    MapIndex index;
    for (map_index(res->node_to_def, &index); map_next(res->node_to_def, &index);) {
      Def *def = map_value(res->node_to_def, &index);
      syntax_tree_free((SyntaxTree *) def->id);
      syntax_tree_free((SyntaxTree *) def->body);
      free(def);
    }
    map_free(res->node_to_def);
    map_free(res->ref_to_def);
    free(res);
  }
}

const Def *res_create_def(Res *res, DefKind kind, Binding *binding, const SyntaxTree *id, const SyntaxTree *body, unsigned long offset)
{
  MapIndex index;
  if (map_find(res->node_to_def, (void *) syntax_tree_raw(id), &index)) {
    unreachable();
  } else {
    Def *def     = xmalloc(sizeof(Def));
    def->kind    = kind;
    def->id      = syntax_tree_subtree(id);
    def->body    = syntax_tree_subtree(body);
    def->offset  = offset;
    def->binding = *binding;
    map_update(res->node_to_def, &index, (void *) syntax_tree_raw(id), def);
    return def;
  }
}

const Def *res_get_def(const Res *res, const TokenNode *node)
{
  MapIndex index;
  if (map_find(res->node_to_def, (void *) node, &index)) {
    return map_value(res->node_to_def, &index);
  } else {
    return NULL;
  }
}

const Def *res_get_ref(const Res *res, const TokenNode *node)
{
  MapIndex index;
  if (map_find(res->ref_to_def, (void *) node, &index)) {
    return map_value(res->ref_to_def, &index);
  } else {
    return NULL;
  }
}

void res_record_ref(Res *res, const TokenNode *node, const Def *def)
{
  MapIndex index;
  map_find(res->ref_to_def, (void *) node, &index);
  map_update(res->ref_to_def, &index, (void *) node, (void *) def);
}
