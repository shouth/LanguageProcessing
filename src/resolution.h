#ifndef RESOLUTION_H
#define RESOLUTION_H

#include "syntax_tree.h"
#include "token_tree.h"

typedef struct Binding Binding;
typedef struct Def     Def;
typedef struct Res     Res;

typedef enum {
  DEF_PROGRAM,
  DEF_PROC,
  DEF_VAR,
  DEF_PARAM
} DefKind;

struct Binding {
  const char   *name;
  unsigned long offset;
  unsigned long length;
};

struct Def {
  DefKind           kind;
  Binding           binding;
  const SyntaxTree *id;
  const SyntaxTree *body;
  unsigned long     offset;
};

Res       *res_new(void);
void       res_free(Res *res);
const Def *res_create_def(Res *res, DefKind kind, Binding *binding, const SyntaxTree *id, const SyntaxTree *body, unsigned long offset);
const Def *res_get_def(const Res *res, const TokenNode *node);
const Def *res_get_ref(const Res *res, const TokenNode *node);
void       res_record_ref(Res *res, const TokenNode *node, const Def *def);

#endif
