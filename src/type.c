#include <stdio.h>

#include "type.h"
#include "utility.h"

struct Type {
  TypeKind kind;
};

struct TypeArray {
  TypeKind      kind;
  Type         *elem;
  unsigned long size;
};

struct TypeProc {
  TypeKind      kind;
  Type        **param;
  unsigned long param_count;
};

Type *type_new(TypeKind kind)
{
  Type *type = xmalloc(sizeof(Type));
  type->kind = kind;
  return type;
}

Type *type_new_proc(Type **param, unsigned long param_count)
{
  TypeProc *type    = xmalloc(sizeof(TypeProc));
  type->kind        = TYPE_PROC;
  type->param       = param;
  type->param_count = param_count;
  return (Type *) type;
}

Type *type_new_array(Type *elem, unsigned long size)
{
  TypeArray *type = xmalloc(sizeof(TypeArray));
  type->kind      = TYPE_ARRAY;
  type->elem      = elem;
  type->size      = size;
  return (Type *) type;
}

Type *type_clone(const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_PROC: {
    TypeProc     *proc = (TypeProc *) type;
    Type        **param;
    unsigned long i;
    param = xmalloc(sizeof(Type *) * proc->param_count);
    for (i = 0; i < proc->param_count; ++i) {
      param[i] = type_clone(proc->param[i]);
    }
    return type_new_proc(param, proc->param_count);
  }
  case TYPE_ARRAY: {
    TypeArray *array = (TypeArray *) type;
    return type_new_array(type_clone(array->elem), array->size);
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
      TypeProc     *proc = (TypeProc *) type;
      unsigned long i;
      for (i = 0; i < proc->param_count; ++i) {
        type_free(proc->param[i]);
      }
      free(proc->param);
      free(proc);
      break;
    }
    case TYPE_ARRAY: {
      TypeArray *array = (TypeArray *) type;
      type_free(array->elem);
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
      TypeProc *left_proc  = (TypeProc *) left;
      TypeProc *right_proc = (TypeProc *) right;
      if (left_proc->param_count != right_proc->param_count) {
        return 0;
      } else {
        unsigned long i;
        for (i = 0; i < left_proc->param_count; ++i) {
          if (!type_equal(left_proc->param[i], right_proc->param[i])) {
            return 0;
          }
        }
        return 1;
      }
    }
    case TYPE_ARRAY: {
      TypeArray *left_array  = (TypeArray *) left;
      TypeArray *right_array = (TypeArray *) right;
      return type_equal(left_array->elem, right_array->elem);
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
    TypeProc     *proc   = (TypeProc *) type;
    unsigned long i;
    length += fprintf(stream, "procedure(");
    for (i = 0; i < proc->param_count; ++i) {
      if (i > 0) {
        length += fprintf(stream, ", ");
      }
      length += type_to_string__impl(stream, proc->param[i]);
    }
    length += fprintf(stream, ")");
    return length;
  }

  case TYPE_ARRAY: {
    unsigned long length = 0;
    TypeArray    *array  = (TypeArray *) type;
    length += fprintf(stream, "array[%lu] of ", array->size);
    length += type_to_string__impl(stream, array->elem);
    return length;
  }

  case TYPE_BOOLEAN:
    return fprintf(stream, "boolean");

  case TYPE_CHAR:
    return fprintf(stream, "char");

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

const Type *type_array_elem(const TypeArray *type)
{
  return type->elem;
}

unsigned long type_array_size(const TypeArray *type)
{
  return type->size;
}

unsigned long type_proc_param_count(const TypeProc *type)
{
  return type->param_count;
}

const Type *type_proc_param(const TypeProc *type, unsigned long index)
{
  return type->param[index];
}
