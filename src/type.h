#ifndef TYPE_H
#define TYPE_H

typedef struct Type      Type;
typedef struct TypeArray TypeArray;
typedef struct TypeProc  TypeProc;

typedef enum {
  TYPE_PROC,
  TYPE_ARRAY,
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_INTEGER
} TypeKind;

Type         *type_new(TypeKind kind);
Type         *type_new_proc(Type **param, unsigned long param_count);
Type         *type_new_array(Type *elem, unsigned long size);
Type         *type_clone(const Type *type);
void          type_free(Type *type);
int           type_equal(const Type *left, const Type *right);
TypeKind      type_kind(const Type *type);
int           type_is_std(const Type *type);
char         *type_to_string(const Type *type);
const Type   *type_array_elem(const TypeArray *type);
unsigned long type_array_size(const TypeArray *type);
unsigned long type_proc_param_count(const TypeProc *type);
const Type   *type_proc_param(const TypeProc *type, unsigned long index);

#endif
