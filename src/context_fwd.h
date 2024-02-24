#ifndef CONTEXT_FWD_H
#define CONTEXT_FWD_H

typedef enum {
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_PROC
} TypeKind;

typedef enum {
  DEF_PROGRAM,
  DEF_PROC,
  DEF_VAR,
  DEF_PARAM,
  DEF_LOCAL
} DefKind;

typedef struct String    String;
typedef struct TypeList  TypeList;
typedef struct Type      Type;
typedef struct ArrayType ArrayType;
typedef struct ProcType  ProcType;
typedef struct Def       Def;
typedef struct Ctx       Ctx;

#endif
