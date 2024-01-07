#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "map.h"
#include "resolution.h"
#include "token_tree.h"
#include "utility.h"

struct Res {
  Array *defs;
  Map   *refs;
};

Res *res_new(void)
{
  Res *res  = xmalloc(sizeof(Res));
  res->defs = array_new(sizeof(Def *));
  res->refs = map_new(NULL, NULL);
  return res;
}

void res_free(Res *res)
{
  unsigned long i;

  if (res) {
    for (i = 0; i < array_count(res->defs); ++i) {
      Def *def = *(Def **) array_at(res->defs, i);
      free(def);
    }
    array_free(res->defs);
    map_free(res->refs);
  }
}

const Def *res_create_def(Res *res, DefKind kind, Binding *binding, const TokenNode *node)
{
  Def *def     = xmalloc(sizeof(Def));
  def->kind    = kind;
  def->node    = node;
  def->binding = *binding;
  array_push(res->defs, &def);
  return def;
}

const Def *res_get_def(const Res *res, const TokenNode *node)
{
  MapIndex index;
  if (map_find(res->refs, (void *) node, &index)) {
    return map_value(res->refs, &index);
  } else {
    return NULL;
  }
}

void res_record_ref(Res *res, const TokenNode *node, const Def *def)
{
  MapIndex index;
  map_find(res->refs, (void *) node, &index);
  map_update(res->refs, &index, (void *) node, (void *) def);
}
