#ifndef SEMANTIC_MODEL_H
#define SEMANTIC_MODEL_H

#include "mppl_syntax.h"
#include "util.h"

/* mppl semantics */

typedef struct MpplRef       MpplRef;
typedef struct MpplDef       MpplDef;
typedef struct MpplSemantics MpplSemantics;

struct MpplRef {
  unsigned long declared_at;
  unsigned long used_at;
};

struct MpplDef {
  MpplIdentToken *binding;
  Slice(unsigned long) refs;
};

struct MpplSemantics {
  Slice(MpplDef) defs;
  HashMap(unsigned long, const MpplDef *) ref;
};

MpplSemantics *mppl_semantics_new(const MpplSyntax *syntax, const MpplRef *refs, unsigned long refs_count);
void           mppl_semantics_free(MpplSemantics *semantics);

#endif /* SEMANTIC_MODEL_H */
