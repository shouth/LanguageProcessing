#ifndef IR_H
#define IR_H

#include "ty_ctxt.h"
#include "util.h"

typedef enum {
  IR_BINOP_ADD,
  IR_BINOP_SUB,
  IR_BINOP_MUL,
  IR_BINOP_DIV,
  IR_BINOP_EQ,
  IR_BINOP_NE,
  IR_BINOP_LT,
  IR_BINOP_LE,
  IR_BINOP_GT,
  IR_BINOP_GE
} IrBinOpKind;

typedef enum {
  IR_UNOP_NEG,
  IR_UNOP_NOT
} IrUnOpKind;

typedef enum {
  IR_RVALUE_BINOP,
  IR_RVALUE_UNOP,
  IR_RVALUE_PLACE,
  IR_RVALUE_CAST
} IrRValueKind;

typedef enum {
  IR_OPERAND_PLACE,
  IR_OPERAND_CONST
} IrOperandKind;

typedef enum {
  IR_TERM_JUMP,
  IR_TERM_BRANCH,
  IR_TERM_RETURN
} IrTermKind;

typedef struct IrLocal IrLocal;

typedef struct IrPlace IrPlace;

typedef struct IrRValue      IrRValue;
typedef struct IrBinOpRValue IrBinOpRValue;
typedef struct IrUnOpRValue  IrUnOpRValue;
typedef struct IrPlaceRValue IrPlaceRValue;
typedef struct IrCastRValue  IrCastRValue;

typedef struct IrOperand      IrOperand;
typedef struct IrPlaceOperand IrPlaceOperand;
typedef struct IrConstOperand IrConstOperand;

typedef struct IrStmt IrStmt;

typedef struct IrTerm       IrTerm;
typedef struct IrCallTerm   IrCallTerm;
typedef struct IrJumpTerm   IrJumpTerm;
typedef struct IrBranchTerm IrBranchTerm;
typedef struct IrReturnTerm IrReturnTerm;

typedef unsigned long  IrBlockId;
typedef struct IrBlock IrBlock;
typedef struct IrItem  IrItem;
typedef struct Ir      Ir;

struct IrLocal {
  Ty *ty;
};

struct IrPlace {
  IrLocal   *local;
  IrOperand *offset;
};

struct IrRValue {
  IrRValueKind kind;
};

struct IrBinOpRValue {
  IrRValue    base;
  IrBinOpKind kind;
  IrOperand  *lhs;
  IrOperand  *rhs;
};

struct IrUnOpRValue {
  IrRValue   base;
  IrUnOpKind kind;
  IrOperand *operand;
};

struct IrPlaceRValue {
  IrRValue base;
  IrPlace *place;
};

struct IrCastRValue {
  IrRValue   base;
  Ty        *ty;
  IrOperand *operand;
};

struct IrOperand {
  IrOperandKind kind;
};

struct IrPlaceOperand {
  IrOperand base;
  IrPlace  *place;
};

struct IrConstOperand {
  IrOperand base;
  long      value;
};

struct IrStmt {
  IrPlace  *dest;
  IrRValue *src;
};

struct IrTerm {
  IrTermKind kind;
};

struct IrCallTerm {
  IrTerm   base;
  IrPlace *dest;
  Slice(IrOperand *) args;
  IrBlockId target;
  IrBlockId unwind;
};

struct IrJumpTerm {
  IrTerm    base;
  IrBlockId target;
};

struct IrBranchTerm {
  IrTerm     base;
  IrOperand *cond;
  IrBlockId  target_true;
  IrBlockId  target_false;
};

struct IrBlock {
  Slice(IrStmt *) stmts;
  IrTerm *term;
};

struct IrItem {
  Slice(IrLocal) locals;
  Slice(IrBlock) blocks;
};

struct Ir {
  Slice(IrLocal) globals;
  Slice(IrItem *) items;
};

#endif
