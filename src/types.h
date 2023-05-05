#ifndef TYPES_H
#define TYPES_H

typedef struct symbol__s   symbol_t;
typedef struct location__s location_t;
typedef struct region__s   region_t;
typedef struct type__s     type_t;
typedef struct subst__s    subst_t;
typedef struct def__s      def_t;

/**********     symbol     **********/

struct symbol__s {
  const char *ptr;
  long        len;
};

/**********     location     **********/

struct location__s {
  long line;
  long col;
};

/**********     region     **********/

struct region__s {
  long pos;
  long len;
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

typedef struct type__procedure_s type_procedure_t;
typedef struct type__array_s     type_array_t;

struct type__procedure_s {
  subst_t *params;
};

struct type__array_s {
  subst_t *base;
  long     size;
};

struct type__s {
  union {
    type_procedure_t procedure;
    type_array_t     array;
  } type;

  type_kind_t kind;
};

/**********     substitution     **********/

struct subst__s {
  const type_t *type;
  subst_t      *next;
};

/**********     definition     **********/

typedef enum {
  DEF_PROGRAM,
  DEF_PROCEDURE,
  DEF_VAR,
  DEF_PARAM
} def_kind_t;

struct def__s {
  const void     *ast;
  const symbol_t *name;
  region_t        region;
  def_kind_t      kind;
  def_t          *inner;
  def_t          *next;
};

#endif
