#ifndef TYPE_H
#define TYPE_H

typedef struct Type      Type;
typedef struct ArrayType ArrayType;
typedef struct ProcType  ProcType;

typedef enum {
  TYPE_PROC,
  TYPE_ARRAY,
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_STRING,
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
const Type   *type_array_elem(const ArrayType *type);
unsigned long type_array_size(const ArrayType *type);
unsigned long type_proc_param_count(const ProcType *type);
const Type   *type_proc_param(const ProcType *type, unsigned long index);

#endif
