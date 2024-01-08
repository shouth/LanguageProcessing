#include "inference.h"
#include "map.h"
#include "utility.h"

struct Infer {
  Map *def_types;
  Map *expr_types;
};

Infer *infer_new(void)
{
  Infer *infer      = xmalloc(sizeof(Infer));
  infer->def_types  = map_new(NULL, NULL);
  infer->expr_types = map_new(NULL, NULL);
  return infer;
}

void infer_free(Infer *infer)
{
  MapIndex index;
  if (infer) {
    for (map_index(infer->def_types, &index); map_next(infer->def_types, &index);) {
      type_free(map_value(infer->def_types, &index));
    }
    map_free(infer->def_types);
    for (map_index(infer->expr_types, &index); map_next(infer->expr_types, &index);) {
      type_free(map_value(infer->expr_types, &index));
    }
    map_free(infer->expr_types);
    free(infer);
  }
}

Type *infer_get_def_type(const Infer *infer, const Def *def)
{
  MapIndex index;
  if (map_find(infer->def_types, (void *) def, &index)) {
    return map_value(infer->def_types, &index);
  } else {
    return NULL;
  }
}

Type *infer_get_expr_type(const Infer *infer, const TokenNode *node)
{
  MapIndex index;
  if (map_find(infer->expr_types, (void *) node, &index)) {
    return map_value(infer->expr_types, &index);
  } else {
    return NULL;
  }
}

void infer_record_def_type(Infer *infer, const Def *def, Type *type)
{
  MapIndex index;
  if (map_find(infer->def_types, (void *) def, &index)) {
    unreachable();
  } else {
    map_update(infer->def_types, &index, (void *) def, type);
  }
}

void infer_record_expr_type(Infer *infer, const TokenNode *node, Type *type)
{
  MapIndex index;
  map_find(infer->expr_types, (void *) node, &index);
  map_update(infer->expr_types, &index, (void *) node, type);
}
