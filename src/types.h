#ifndef TYPES_H
#define TYPES_H

/**********     symbol     **********/

typedef struct symbol__s symbol_t;

struct symbol__s {
  const char *ptr;
  long        len;
};

/**********     type     **********/

typedef enum {
  TYPE_INTEGER,
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_PROGRAM,
  TYPE_PROCEDURE
} type_kind_t;

typedef struct substs__s         substs_t;
typedef struct type__s           type_t;
typedef struct type__procedure_s type_procedure_t;
typedef struct type__array_s     type_array_t;

struct substs__s {
  const type_t **types;
  long           count;
};

struct type__procedure_s {
  const substs_t *params;
};

struct type__array_s {
  const substs_t *base;
  long            size;
};

struct type__s {
  union {
    type_procedure_t procedure;
    type_array_t     array;
  } type;

  type_kind_t kind;
};

/**********     definition     **********/

typedef enum {
  DEF_PROGRAM,
  DEF_PROCEDURE,
  DEF_VAR,
  DEF_PARAM
} def_kind_t;

typedef struct def__s def_t;

struct def__s {
  const void     *ast;
  const symbol_t *name;
  def_kind_t      kind;
  const type_t   *type;
  def_t          *inner;
  def_t          *next;
};

#endif
