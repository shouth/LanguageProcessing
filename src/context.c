#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "context.h"
#include "context_fwd.h"
#include "map.h"
#include "string.h"
#include "syntax_tree.h"
#include "utility.h"

struct String {
  const char   *data;
  unsigned long length;
};

struct TypeList {
  const Type  **types;
  unsigned long length;
};

struct Type {
  TypeKind kind;
};

struct ArrayType {
  TypeKind      kind;
  const Type   *base;
  unsigned long length;
};

struct ProcType {
  TypeKind        kind;
  const TypeList *params;
};

struct Def {
  DefKind           kind;
  const String     *name;
  const SyntaxTree *syntax;
};

struct Ctx {
  Map   *strings;
  Map   *type_lists;
  Map   *types;
  Array *defs;
  Map   *resolved;
  Map   *syntax_type;
};

const TypeList  CTX_TYPE_LIST_EMPTY_INSTANCE = { NULL, 0 };
const TypeList *CTX_TYPE_LIST_EMPTY          = &CTX_TYPE_LIST_EMPTY_INSTANCE;

const Type  CTX_TYPE_BOOLEAN_INSTANCE = { TYPE_BOOLEAN };
const Type  CTX_TYPE_CHAR_INSTANCE    = { TYPE_CHAR };
const Type  CTX_TYPE_INTEGER_INSTANCE = { TYPE_INTEGER };
const Type  CTX_TYPE_STRING_INSTANCE  = { TYPE_STRING };
const Type *CTX_TYPE_BOOLEAN          = &CTX_TYPE_BOOLEAN_INSTANCE;
const Type *CTX_TYPE_CHAR             = &CTX_TYPE_CHAR_INSTANCE;
const Type *CTX_TYPE_INTEGER          = &CTX_TYPE_INTEGER_INSTANCE;
const Type *CTX_TYPE_STRING           = &CTX_TYPE_STRING_INSTANCE;

static unsigned long string_hash(const void *value)
{
  const String *x = value;
  return fnv1a(FNV1A_INIT, x->data, x->length);
}

static int string_equal(const void *left, const void *right)
{
  const String *l = left;
  const String *r = right;
  return l->length == r->length && memcmp(l->data, r->data, l->length) == 0;
}

static unsigned long type_list_hash_core(unsigned long hash, const TypeList *list);
static unsigned long type_hash_core(unsigned long hash, const Type *type);

static unsigned long type_list_hash_core(unsigned long hash, const TypeList *list)
{
  unsigned long i;
  for (i = 0; i < list->length; i++) {
    hash = type_hash_core(hash, list->types[i]);
  }
  return hash;
}

static unsigned long type_hash_core(unsigned long hash, const Type *type)
{
  hash = fnv1a(hash, &type->kind, sizeof(type->kind));
  switch (type->kind) {
  case TYPE_ARRAY: {
    const ArrayType *array = (const ArrayType *) type;
    hash                   = type_hash_core(hash, array->base);
    hash                   = fnv1a(hash, &array->length, sizeof(array->length));
    return hash;
  }
  case TYPE_PROC: {
    const ProcType *proc = (const ProcType *) type;
    hash                 = type_list_hash_core(hash, proc->params);
    return hash;
  }
  default:
    return hash;
  }
}

static unsigned long type_list_hash(const void *value)
{
  return type_list_hash_core(FNV1A_INIT, value);
}

static int type_list_equal(const void *left, const void *right)
{
  const TypeList *l = left;
  const TypeList *r = right;

  return l->length == r->length && !memcmp(l->types, r->types, sizeof(Type *) * l->length);
}

static unsigned long type_hash(const void *value)
{
  return type_hash_core(FNV1A_INIT, value);
}

static int type_equal(const void *left, const void *right)
{
  const Type *l = left;
  const Type *r = right;
  if (l->kind != r->kind) {
    return 0;
  }
  switch (l->kind) {
  case TYPE_ARRAY: {
    const ArrayType *larray = (const ArrayType *) l;
    const ArrayType *rarray = (const ArrayType *) r;
    return larray->base == rarray->base && larray->length == rarray->length;
  }
  case TYPE_PROC: {
    const ProcType *lproc = (const ProcType *) l;
    const ProcType *rproc = (const ProcType *) r;
    return type_list_equal(lproc->params, rproc->params);
  }
  default:
    return 1;
  }
}

static const void *intern(Map *interner, const void *value, unsigned long size, void *(*clone)(const void *) )
{
  MapIndex index;
  if (map_entry(interner, (void *) value, &index)) {
    return map_value(interner, &index);
  } else {
    void *instance;
    if (clone) {
      instance = clone(value);
    } else {
      instance = xmalloc(size);
      memcpy(instance, value, size);
    }
    map_update(interner, &index, instance, instance);
    return instance;
  }
}

Ctx *ctx_new(void)
{
  Ctx *ctx         = xmalloc(sizeof(Ctx));
  ctx->strings     = map_new(&string_hash, &string_equal);
  ctx->type_lists  = map_new(&type_list_hash, &type_list_equal);
  ctx->types       = map_new(&type_hash, &type_equal);
  ctx->defs        = array_new(sizeof(Def *));
  ctx->resolved    = map_new(NULL, NULL);
  ctx->syntax_type = map_new(NULL, NULL);

  intern(ctx->type_lists, CTX_TYPE_LIST_EMPTY, sizeof(TypeList), NULL);

  intern(ctx->types, CTX_TYPE_BOOLEAN, sizeof(Type), NULL);
  intern(ctx->types, CTX_TYPE_CHAR, sizeof(Type), NULL);
  intern(ctx->types, CTX_TYPE_INTEGER, sizeof(Type), NULL);
  intern(ctx->types, CTX_TYPE_STRING, sizeof(Type), NULL);

  return ctx;
}

void ctx_free(Ctx *ctx)
{
  if (ctx) {
    unsigned long i;
    MapIndex      index;

    for (map_iterator(ctx->strings, &index); map_next(ctx->strings, &index);) {
      const String *string = map_value(ctx->strings, &index);
      free((void *) string->data);
      free((void *) string);
    }
    map_free(ctx->strings);

    for (map_iterator(ctx->type_lists, &index); map_next(ctx->type_lists, &index);) {
      const TypeList *list = map_value(ctx->type_lists, &index);
      free((void *) list->types);
      free((void *) list);
    }
    map_free(ctx->type_lists);

    for (map_iterator(ctx->types, &index); map_next(ctx->types, &index);) {
      const Type *type = map_value(ctx->types, &index);
      free((void *) type);
    }
    map_free(ctx->types);

    for (i = 0; i < array_count(ctx->defs); i++) {
      Def *def = *(Def **) array_at(ctx->defs, i);
      syntax_tree_unref(def->syntax);
      free(def);
    }
    array_free(ctx->defs);

    map_free(ctx->resolved);
    map_free(ctx->syntax_type);
    free(ctx);
  }
}

static void *clone_string(const void *value)
{
  const String *string = value;
  char         *data   = xmalloc(string->length + 1);
  String       *clone  = xmalloc(sizeof(String));

  memcpy(data, string->data, string->length);
  data[string->length] = '\0';

  clone->length = string->length;
  clone->data   = data;
  return clone;
}

const String *ctx_string(Ctx *ctx, const char *data, unsigned long length)
{
  String string;
  string.data   = data;
  string.length = length;
  return intern(ctx->strings, &string, sizeof(String), &clone_string);
}

const Type *ctx_array_type(Ctx *ctx, const Type *base, unsigned long length)
{
  ArrayType type;
  type.kind   = TYPE_ARRAY;
  type.base   = base;
  type.length = length;
  return intern(ctx->types, &type, sizeof(ArrayType), NULL);
}

const Type *ctx_proc_type(Ctx *ctx, const TypeList *params)
{
  ProcType type;
  type.kind   = TYPE_PROC;
  type.params = params;
  return intern(ctx->types, &type, sizeof(ProcType), NULL);
}

const TypeList *ctx_type_list(Ctx *ctx, const Type **types, unsigned long length)
{
  const Type **copy = xmalloc(sizeof(Type *) * length);
  memcpy(copy, types, sizeof(Type *) * length);
  return ctx_take_type_list(ctx, copy, length);
}

const TypeList *ctx_take_type_list(Ctx *ctx, const Type **types, unsigned long length)
{
  TypeList list;
  list.types  = types;
  list.length = length;
  return intern(ctx->type_lists, &list, sizeof(TypeList), NULL);
}

const Def *ctx_define(Ctx *ctx, DefKind kind, const String *name, const SyntaxTree *syntax)
{
  Def *def    = xmalloc(sizeof(Def));
  def->kind   = kind;
  def->name   = name;
  def->syntax = syntax_tree_ref(syntax);
  array_push(ctx->defs, &def);
  return def;
}

const Def *ctx_resolve(Ctx *ctx, const SyntaxTree *syntax, const Def *def)
{
  MapIndex index;

  const RawSyntaxNode *node = syntax_tree_raw(syntax);
  if (def) {
    if (map_entry(ctx->resolved, (void *) node, &index)) {
      unreachable();
    } else {
      map_update(ctx->resolved, &index, (void *) node, (void *) def);
      return def;
    }
  } else {
    if (map_entry(ctx->resolved, (void *) node, &index)) {
      return map_value(ctx->resolved, &index);
    } else {
      return NULL;
    }
  }
}

const Type *ctx_type_of(const Ctx *ctx, const SyntaxTree *syntax, const Type *type)
{
  MapIndex index;

  const RawSyntaxNode *node = syntax_tree_raw(syntax);
  if (type) {
    if (map_entry(ctx->syntax_type, (void *) node, &index)) {
      unreachable();
    } else {
      map_update(ctx->syntax_type, &index, (void *) node, (void *) type);
      return type;
    }
  } else {
    if (map_entry(ctx->syntax_type, (void *) node, &index)) {
      return map_value(ctx->syntax_type, &index);
    } else {
      return NULL;
    }
  }
}

const char *string_data(const String *string)
{
  return string->data;
}

unsigned long string_length(const String *string)
{
  return string->length;
}

const Type *type_list_at(const TypeList *list, unsigned long index)
{
  return list->types[index];
}

unsigned long type_list_count(const TypeList *list)
{
  return list->length;
}

TypeKind type_kind(const Type *type)
{
  return type->kind;
}

int type_is_std(const Type *type)
{
  return type->kind == TYPE_BOOLEAN || type->kind == TYPE_CHAR || type->kind == TYPE_INTEGER;
}

static unsigned long type_to_string_core(FILE *stream, const Type *type)
{
  switch (type->kind) {
  case TYPE_BOOLEAN:
    return fprintf(stream, "boolean");
  case TYPE_CHAR:
    return fprintf(stream, "char");
  case TYPE_INTEGER:
    return fprintf(stream, "integer");
  case TYPE_STRING:
    return fprintf(stream, "string");
  case TYPE_ARRAY: {
    const ArrayType *array = (const ArrayType *) type;
    unsigned long    count = fprintf(stream, "array[%lu] of", array->length);
    count += type_to_string_core(stream, array->base);
    return count;
  }
  case TYPE_PROC: {
    const ProcType *proc  = (const ProcType *) type;
    unsigned long   count = fprintf(stream, "procedure(");
    unsigned long   i;
    for (i = 0; i < proc->params->length; i++) {
      if (i > 0) {
        count += fprintf(stream, ", ");
      }
      count += type_to_string_core(stream, proc->params->types[i]);
    }
    count += fprintf(stream, ")");
    return count;
  }
  default:
    unreachable();
  }
}

char *type_to_string(const Type *type)
{
  FILE         *stream = tmpfile();
  unsigned long length = type_to_string_core(stream, type);
  char         *result = xmalloc(length + 1);
  fseek(stream, 0, SEEK_SET);
  fread(result, 1, length, stream);
  result[length] = '\0';
  fclose(stream);
  return result;
}

const Type *array_type_base(const ArrayType *type)
{
  return type->base;
}

unsigned long array_type_length(const ArrayType *type)
{
  return type->length;
}

const TypeList *proc_type_params(const ProcType *type)
{
  return type->params;
}

DefKind def_kind(const Def *def)
{
  return def->kind;
}

const String *def_name(const Def *def)
{
  return def->name;
}

const SyntaxTree *def_syntax(const Def *def)
{
  return def->syntax;
}
