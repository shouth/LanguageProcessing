#ifndef RESOLUTION_H
#define RESOLUTION_H

#include "string.h"
#include "syntax_tree.h"

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
  const String *name;
  unsigned long offset;
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
const Def *res_get_def(const Res *res, const RawSyntaxNode *node);
const Def *res_get_ref(const Res *res, const RawSyntaxNode *node);
void       res_record_ref(Res *res, const RawSyntaxNode *node, const Def *def);

#endif
