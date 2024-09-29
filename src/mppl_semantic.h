#ifndef SEMANTIC_MODEL_H
#define SEMANTIC_MODEL_H

#include "mppl_syntax.h"
#include "util.h"

/* mppl semantics */

typedef enum {
  MPPL_SEMANTIC_DEFINE,
  MPPL_SEMANTIC_USE,
  MPPL_SEMANTIC_NOT_FOUND
} MpplSemanticEventKind;

typedef struct MpplSemanticEvent MpplSemanticEvent;
typedef struct MpplBinding       MpplBinding;
typedef struct MpplSemantics     MpplSemantics;

struct MpplSemanticEvent {
  MpplSemanticEventKind kind;
  unsigned long         declared_at;
  unsigned long         used_at;
};

struct MpplBinding {
  SyntaxToken *binding;
  Slice(unsigned long) refs;
};

struct MpplSemantics {
  Slice(MpplBinding) bindings;
  HashMap(unsigned long, const MpplBinding *) ref;
};

MpplSemantics *mppl_semantics_new(const MpplSyntax *syntax, const MpplSemanticEvent *events, unsigned long event_count);
void           mppl_semantics_free(MpplSemantics *semantics);

#endif /* SEMANTIC_MODEL_H */
