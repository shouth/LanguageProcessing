#include <stdio.h>

#include "type.h"
#include "utility.h"

struct Type {
  TypeKind kind;
};

struct ArrayType {
  TypeKind      kind;
  Type         *elem;
  unsigned long size;
};

struct ProcType {
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
  ProcType *type    = xmalloc(sizeof(ProcType));
  type->kind        = TYPE_PROC;
  type->param       = param;
  type->param_count = param_count;
  return (Type *) type;
}

Type *type_new_array(Type *elem, unsigned long size)
{
  ArrayType *type = xmalloc(sizeof(ArrayType));
  type->kind      = TYPE_ARRAY;
  type->elem      = elem;
  type->size      = size;
  return (Type *) type;
}

Type *type_clone(const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_PROC: {
    ProcType     *proc = (ProcType *) type;
    Type        **param;
    unsigned long i;
    param = xmalloc(sizeof(Type *) * proc->param_count);
    for (i = 0; i < proc->param_count; ++i) {
      param[i] = type_clone(proc->param[i]);
    }
    return type_new_proc(param, proc->param_count);
  }
  case TYPE_ARRAY: {
    ArrayType *array = (ArrayType *) type;
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
      ProcType     *proc = (ProcType *) type;
      unsigned long i;
      for (i = 0; i < proc->param_count; ++i) {
        type_free(proc->param[i]);
      }
      free(proc->param);
      free(proc);
      break;
    }
    case TYPE_ARRAY: {
      ArrayType *array = (ArrayType *) type;
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
      ProcType *left_proc  = (ProcType *) left;
      ProcType *right_proc = (ProcType *) right;
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
      ArrayType *left_array  = (ArrayType *) left;
      ArrayType *right_array = (ArrayType *) right;
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
    ProcType     *proc   = (ProcType *) type;
    unsigned long i;
    length += fprintf(stream, "procedure");
    if (proc->param_count > 0) {
      length += fprintf(stream, "(");
      for (i = 0; i < proc->param_count; ++i) {
        if (i > 0) {
          length += fprintf(stream, ", ");
        }
        length += type_to_string__impl(stream, proc->param[i]);
      }
      length += fprintf(stream, ")");
    }
    return length;
  }

  case TYPE_ARRAY: {
    unsigned long length = 0;
    ArrayType    *array  = (ArrayType *) type;
    length += fprintf(stream, "array[%lu] of ", array->size);
    length += type_to_string__impl(stream, array->elem);
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
  return type->elem;
}

unsigned long type_array_size(const ArrayType *type)
{
  return type->size;
}

unsigned long type_proc_param_count(const ProcType *type)
{
  return type->param_count;
}

const Type *type_proc_param(const ProcType *type, unsigned long index)
{
  return type->param[index];
}
