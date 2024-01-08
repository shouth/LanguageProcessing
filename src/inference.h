#ifndef INFERENCE_H
#define INFERENCE_H

#include "resolution.h"
#include "token_tree.h"
#include "type.h"

typedef struct Infer Infer;

Infer *infer_new(void);
void   infer_free(Infer *infer);
Type  *infer_get_def_type(const Infer *infer, const Def *def);
Type  *infer_get_expr_type(const Infer *infer, const TokenNode *node);
void   infer_record_def_type(Infer *infer, const Def *def, Type *type);
void   infer_record_expr_type(Infer *infer, const TokenNode *node, Type *type);

#endif
