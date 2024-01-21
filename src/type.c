#include <stdio.h>

#include "array.h"
#include "type.h"
#include "utility.h"

struct Type {
  TypeKind kind;
};

struct ArrayType {
  TypeKind      kind;
  Type         *base;
  unsigned long size;
};

struct ProcType {
  TypeKind  kind;
  TypeList *param;
};

struct TypeListBuilder {
  Array *types;
};

struct TypeList {
  Type        **types;
  unsigned long count;
};

Type *type_new(TypeKind kind)
{
  Type *type = xmalloc(sizeof(Type));
  type->kind = kind;
  return type;
}

Type *type_new_proc(TypeList *param)
{
  ProcType *type = xmalloc(sizeof(ProcType));
  type->kind     = TYPE_PROC;
  type->param    = param;
  return (Type *) type;
}

Type *type_new_array(Type *elem, unsigned long size)
{
  ArrayType *type = xmalloc(sizeof(ArrayType));
  type->kind      = TYPE_ARRAY;
  type->base      = elem;
  type->size      = size;
  return (Type *) type;
}

Type *type_clone(const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_PROC: {
    ProcType *proc = (ProcType *) type;
    return type_new_proc(type_list_clone(proc->param));
  }
  case TYPE_ARRAY: {
    ArrayType *array = (ArrayType *) type;
    return type_new_array(type_clone(array->base), array->size);
  }
  default:
    return type_new(type_kind(type));
  }
}

void type_free(Type *type)
{
  if (type) {
    switch (type_kind(type)) {
    case TYPE_PROC: {
      ProcType *proc = (ProcType *) type;
      type_list_free(proc->param);
      free(proc);
      break;
    }
    case TYPE_ARRAY: {
      ArrayType *array = (ArrayType *) type;
      type_free(array->base);
      free(array);
      break;
    }
    default:
      free(type);
      break;
    }
  }
}

int type_equal(const Type *left, const Type *right)
{
  TypeKind left_kind  = type_kind(left);
  TypeKind right_kind = type_kind(right);

  if (left_kind != right_kind) {
    return 0;
  } else {
    switch (left_kind) {
    case TYPE_PROC: {
      ProcType *left_proc  = (ProcType *) left;
      ProcType *right_proc = (ProcType *) right;
      return type_list_equal(left_proc->param, right_proc->param);
    }
    case TYPE_ARRAY: {
      ArrayType *left_array  = (ArrayType *) left;
      ArrayType *right_array = (ArrayType *) right;
      return type_equal(left_array->base, right_array->base);
    }
    default:
      return 1;
    }
  }
}

TypeKind type_kind(const Type *type)
{
  return type->kind;
}

int type_is_std(const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_BOOLEAN:
  case TYPE_CHAR:
  case TYPE_INTEGER:
    return 1;
  default:
    return 0;
  }
}

static unsigned long type_to_string__impl(FILE *stream, const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_PROC: {
    unsigned long length = 0;
    ProcType     *proc   = (ProcType *) type;
    unsigned long i;
    length += fprintf(stream, "procedure");
    if (type_list_count(proc->param) > 0) {
      length += fprintf(stream, "(");
      for (i = 0; i < type_list_count(proc->param); ++i) {
        if (i > 0) {
          length += fprintf(stream, ", ");
        }
        length += type_to_string__impl(stream, type_list_at(proc->param, i));
      }
      length += fprintf(stream, ")");
    }
    return length;
  }

  case TYPE_ARRAY: {
    unsigned long length = 0;
    ArrayType    *array  = (ArrayType *) type;
    length += fprintf(stream, "array[%lu] of ", array->size);
    length += type_to_string__impl(stream, array->base);
    return length;
  }

  case TYPE_BOOLEAN:
    return fprintf(stream, "boolean");

  case TYPE_CHAR:
    return fprintf(stream, "char");

  case TYPE_STRING:
    return fprintf(stream, "string");

  case TYPE_INTEGER:
    return fprintf(stream, "integer");

  default:
    unreachable();
  }
}

char *type_to_string(const Type *type)
{
  FILE         *file   = tmpfile();
  unsigned long length = type_to_string__impl(file, type);
  char         *string = xmalloc(length + 1);
  rewind(file);
  fread(string, 1, length, file);
  string[length] = '\0';
  fclose(file);
  return string;
}

const Type *type_array_elem(const ArrayType *type)
{
  return type->base;
}

unsigned long type_array_size(const ArrayType *type)
{
  return type->size;
}

unsigned long type_proc_param_count(const ProcType *type)
{
  return type_list_count(type->param);
}

const Type *type_proc_param(const ProcType *type, unsigned long index)
{
  return type_list_at(type->param, index);
}

TypeListBuilder *type_list_builder_new(void)
{
  TypeListBuilder *builder = xmalloc(sizeof(TypeListBuilder));
  builder->types           = array_new(sizeof(Type *));
  return builder;
}

void type_list_builder_add(TypeListBuilder *builder, Type *type)
{
  array_push(builder->types, &type);
}

TypeList *type_list_builder_finish(TypeListBuilder *builder)
{
  TypeList *list = xmalloc(sizeof(TypeList));
  list->count    = array_count(builder->types);
  list->types    = (Type **) array_steal(builder->types);
  free(builder);
  return list;
}

TypeList *type_list_clone(const TypeList *list)
{
  unsigned long i;

  TypeList *clone = xmalloc(sizeof(TypeList));
  clone->count    = list->count;
  clone->types    = xmalloc(sizeof(Type *) * list->count);
  for (i = 0; i < list->count; ++i) {
    clone->types[i] = type_clone(list->types[i]);
  }
  return clone;
}

void type_list_free(TypeList *list)
{
  if (list) {
    unsigned long i;
    for (i = 0; i < list->count; ++i) {
      type_free(list->types[i]);
    }
    free(list->types);
    free(list);
  }
}

unsigned long type_list_count(const TypeList *list)
{
  return list->count;
}

const Type *type_list_at(const TypeList *list, unsigned long index)
{
  return list->types[index];
}

int type_list_equal(const TypeList *left, const TypeList *right)
{
  unsigned long i;
  if (left->count != right->count) {
    return 0;
  } else {
    for (i = 0; i < left->count; ++i) {
      if (!type_equal(left->types[i], right->types[i])) {
        return 0;
      }
    }
    return 1;
  }
}
