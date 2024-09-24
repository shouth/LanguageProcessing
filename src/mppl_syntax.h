/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_SYNTAX_H
#define MPPL_SYNTAX_H

#include "syntax_tree.h"
#include "utility.h"

/* mppl syntax kind */

typedef enum {
  MPPL_SYNTAX_ERROR,

  MPPL_SYNTAX_END_OF_FILE,

  MPPL_SYNTAX_IDENT_TOKEN,

  MPPL_SYNTAX_NUMBER_LIT,
  MPPL_SYNTAX_STRING_LIT,

  MPPL_SYNTAX_PLUS_TOKEN,
  MPPL_SYNTAX_MINUS_TOKEN,
  MPPL_SYNTAX_STAR_TOKEN,
  MPPL_SYNTAX_EQUAL_TOKEN,
  MPPL_SYNTAX_NOTEQ_TOKEN,
  MPPL_SYNTAX_LESS_TOKEN,
  MPPL_SYNTAX_LESSEQ_TOKEN,
  MPPL_SYNTAX_GREATER_TOKEN,
  MPPL_SYNTAX_GREATEREQ_TOKEN,
  MPPL_SYNTAX_LPAREN_TOKEN,
  MPPL_SYNTAX_RPAREN_TOKEN,
  MPPL_SYNTAX_LBRACKET_TOKEN,
  MPPL_SYNTAX_RBRACKET_TOKEN,
  MPPL_SYNTAX_ASSIGN_TOKEN,
  MPPL_SYNTAX_DOT_TOKEN,
  MPPL_SYNTAX_COMMA_TOKEN,
  MPPL_SYNTAX_COLON_TOKEN,
  MPPL_SYNTAX_SEMI_TOKEN,

  MPPL_SYNTAX_PROGRAM_KW,
  MPPL_SYNTAX_VAR_KW,
  MPPL_SYNTAX_ARRAY_KW,
  MPPL_SYNTAX_OF_KW,
  MPPL_SYNTAX_BEGIN_KW,
  MPPL_SYNTAX_END_KW,
  MPPL_SYNTAX_IF_KW,
  MPPL_SYNTAX_THEN_KW,
  MPPL_SYNTAX_ELSE_KW,
  MPPL_SYNTAX_PROCEDURE_KW,
  MPPL_SYNTAX_RETURN_KW,
  MPPL_SYNTAX_CALL_KW,
  MPPL_SYNTAX_WHILE_KW,
  MPPL_SYNTAX_DO_KW,
  MPPL_SYNTAX_NOT_KW,
  MPPL_SYNTAX_OR_KW,
  MPPL_SYNTAX_DIV_KW,
  MPPL_SYNTAX_AND_KW,
  MPPL_SYNTAX_CHAR_KW,
  MPPL_SYNTAX_INTEGER_KW,
  MPPL_SYNTAX_BOOLEAN_KW,
  MPPL_SYNTAX_READ_KW,
  MPPL_SYNTAX_WRITE_KW,
  MPPL_SYNTAX_READLN_KW,
  MPPL_SYNTAX_WRITELN_KW,
  MPPL_SYNTAX_TRUE_KW,
  MPPL_SYNTAX_FALSE_KW,
  MPPL_SYNTAX_BREAK_KW,

  MPPL_SYNTAX_SPACE_TRIVIA,
  MPPL_SYNTAX_BRACES_COMMENT_TRIVIA,
  MPPL_SYNTAX_C_COMMENT_TRIVIA,

  MPPL_SYNTAX_PROGRAM = SYNTAX_KIND_TREE,
  MPPL_SYNTAX_DECL_LIST,
  MPPL_SYNTAX_VAR_DECL_PART,
  MPPL_SYNTAX_VAR_DECL_LIST_ELEM,
  MPPL_SYNTAX_VAR_DECL_LIST,
  MPPL_SYNTAX_VAR_DECL,
  MPPL_SYNTAX_ARRAY_TYPE,
  MPPL_SYNTAX_PROC_DECL,
  MPPL_SYNTAX_FML_PARAM_LIST_ELEM,
  MPPL_SYNTAX_FML_PARAM_LIST,
  MPPL_SYNTAX_FML_PARAMS,
  MPPL_SYNTAX_FML_PARAM_SEC,
  MPPL_SYNTAX_STMT_LIST_ELEM,
  MPPL_SYNTAX_STMT_LIST,
  MPPL_SYNTAX_ASSIGN_STMT,
  MPPL_SYNTAX_IF_STMT,
  MPPL_SYNTAX_ELSE_CLAUSE,
  MPPL_SYNTAX_WHILE_STMT,
  MPPL_SYNTAX_BREAK_STMT,
  MPPL_SYNTAX_CALL_STMT,
  MPPL_SYNTAX_ACT_PARAMS,
  MPPL_SYNTAX_RETURN_STMT,
  MPPL_SYNTAX_INPUT_STMT,
  MPPL_SYNTAX_INPUTS,
  MPPL_SYNTAX_OUTPUT_STMT,
  MPPL_SYNTAX_OUTPUT_LIST_ELEM,
  MPPL_SYNTAX_OUTPUT_LIST,
  MPPL_SYNTAX_OUTPUTS,
  MPPL_SYNTAX_OUTPUT_VALUE,
  MPPL_SYNTAX_COMP_STMT,
  MPPL_SYNTAX_EXPR_LIST_ELEM,
  MPPL_SYNTAX_EXPR_LIST,
  MPPL_SYNTAX_ENTIRE_VAR,
  MPPL_SYNTAX_INDEXED_VAR,
  MPPL_SYNTAX_UNARY_EXPR,
  MPPL_SYNTAX_BINARY_EXPR,
  MPPL_SYNTAX_PAREN_EXPR,
  MPPL_SYNTAX_CAST_EXPR,
  MPPL_SYNTAX_IDENT_LIST_ELEM,
  MPPL_SYNTAX_IDENT_LIST,

  MPPL_SYNTAX_BOGUS
} MpplSyntaxKind;

#define MPPL_BEGIN_PUNCT MPPL_SYNTAX_PLUS_TOKEN
#define MPPL_END_PUNCT   MPPL_SYNTAX_SEMI_TOKEN

#define MPPL_BEGIN_KEYWORD MPPL_SYNTAX_PROGRAM_KW
#define MPPL_END_KEYWORD   MPPL_SYNTAX_BREAK_KW

#define MPPL_BEGIN_TRIVIA MPPL_SYNTAX_SPACE_TRIVIA
#define MPPL_END_TRIVIA   MPPL_SYNTAX_C_COMMENT_TRIVIA

#define MPPL_BEGIN_TOKEN MPPL_SYNTAX_ERROR
#define MPPL_END_TOKEN   MPPL_SYNTAX_C_COMMENT_TRIVIA

#define MPPL_BEGIN_TREE MPPL_SYNTAX_PROGRAM
#define MPPL_END_TREE   MPPL_SYNTAX_IDENT_LIST

typedef BitSet(MPPL_END_TOKEN - MPPL_BEGIN_TOKEN + 1) MpplTokenKindSet;

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size);
const char    *mppl_syntax_kind_static_lexeme(MpplSyntaxKind kind);
const char    *mppl_syntax_kind_to_string(MpplSyntaxKind kind);

/* mppl syntax */

typedef enum {
  MPPL_DECL_VAR,
  MPPL_DECL_PROC
} MpplDeclKind;

typedef enum {
  MPPL_TYPE_INTEGER,
  MPPL_TYPE_CHAR,
  MPPL_TYPE_BOOLEAN,
  MPPL_TYPE_ARRAY
} MpplTypeKind;

typedef enum {
  MPPL_STMT_ASSIGN,
  MPPL_STMT_IF,
  MPPL_STMT_WHILE,
  MPPL_STMT_BREAK,
  MPPL_STMT_CALL,
  MPPL_STMT_RETURN,
  MPPL_STMT_INPUT,
  MPPL_STMT_OUTPUT,
  MPPL_STMT_COMP
} MpplStmtKind;

typedef enum {
  MPPL_EXPR_ENTIRE_VAR,
  MPPL_EXPR_INDEXED_VAR,
  MPPL_EXPR_UNARY,
  MPPL_EXPR_BINARY,
  MPPL_EXPR_PAREN,
  MPPL_EXPR_CAST
} MpplExprKind;

typedef enum {
  MPPL_UNARY_NOT,
  MPPL_UNARY_PLUS,
  MPPL_UNARY_MINUS
} MpplUnaryOpKind;

typedef enum {
  MPPL_BINARY_PLUS,
  MPPL_BINARY_MINUS,
  MPPL_BINARY_STAR,
  MPPL_BINARY_DIV,
  MPPL_BINARY_EQUAL,
  MPPL_BINARY_NOTEQ,
  MPPL_BINARY_LESS,
  MPPL_BINARY_LESSEQ,
  MPPL_BINARY_GREATER,
  MPPL_BINARY_GREATEREQ,
  MPPL_BINARY_AND,
  MPPL_BINARY_OR
} MpplBinaryOpKind;

typedef struct MpplErrorToken          MpplErrorToken;
typedef struct MpplEndOfFileToken      MpplEndOfFileToken;
typedef struct MpplIdentToken          MpplIdentToken;
typedef struct MpplNumberLit           MpplNumberLit;
typedef struct MpplStringLit           MpplStringLit;
typedef struct MpplPlusToken           MpplPlusToken;
typedef struct MpplMinusToken          MpplMinusToken;
typedef struct MpplStarToken           MpplStarToken;
typedef struct MpplEqualToken          MpplEqualToken;
typedef struct MpplNotEqToken          MpplNotEqToken;
typedef struct MpplLessToken           MpplLessToken;
typedef struct MpplLessEqToken         MpplLessEqToken;
typedef struct MpplGreaterToken        MpplGreaterToken;
typedef struct MpplGreaterEqToken      MpplGreaterEqToken;
typedef struct MpplLParenToken         MpplLParenToken;
typedef struct MpplRParenToken         MpplRParenToken;
typedef struct MpplLBracketToken       MpplLBracketToken;
typedef struct MpplRBracketToken       MpplRBracketToken;
typedef struct MpplAssignToken         MpplAssignToken;
typedef struct MpplDotToken            MpplDotToken;
typedef struct MpplCommaToken          MpplCommaToken;
typedef struct MpplColonToken          MpplColonToken;
typedef struct MpplSemiToken           MpplSemiToken;
typedef struct MpplProgramKw           MpplProgramKw;
typedef struct MpplVarKw               MpplVarKw;
typedef struct MpplArrayKw             MpplArrayKw;
typedef struct MpplOfKw                MpplOfKw;
typedef struct MpplBeginKw             MpplBeginKw;
typedef struct MpplEndKw               MpplEndKw;
typedef struct MpplIfKw                MpplIfKw;
typedef struct MpplThenKw              MpplThenKw;
typedef struct MpplElseKw              MpplElseKw;
typedef struct MpplProcedureKw         MpplProcedureKw;
typedef struct MpplReturnKw            MpplReturnKw;
typedef struct MpplCallKw              MpplCallKw;
typedef struct MpplWhileKw             MpplWhileKw;
typedef struct MpplDoKw                MpplDoKw;
typedef struct MpplNotKw               MpplNotKw;
typedef struct MpplOrKw                MpplOrKw;
typedef struct MpplDivKw               MpplDivKw;
typedef struct MpplAndKw               MpplAndKw;
typedef struct MpplCharKw              MpplCharKw;
typedef struct MpplIntegerKw           MpplIntegerKw;
typedef struct MpplBooleanKw           MpplBooleanKw;
typedef struct MpplReadKw              MpplReadKw;
typedef struct MpplWriteKw             MpplWriteKw;
typedef struct MpplReadLnKw            MpplReadLnKw;
typedef struct MpplWriteLnKw           MpplWriteLnKw;
typedef struct MpplTrueKw              MpplTrueKw;
typedef struct MpplFalseKw             MpplFalseKw;
typedef struct MpplBreakKw             MpplBreakKw;
typedef struct MpplSpaceTrivia         MpplSpaceTrivia;
typedef struct MpplBracesCommentTrivia MpplBracesCommentTrivia;
typedef struct MpplCCommentTrivia      MpplCCommentTrivia;

typedef struct MpplProgram          MpplProgram;
typedef struct MpplDeclList         MpplDeclList;
typedef struct MpplVarDeclPart      MpplVarDeclPart;
typedef struct MpplVarDeclListElem  MpplVarDeclListElem;
typedef struct MpplVarDeclList      MpplVarDeclList;
typedef struct MpplVarDecl          MpplVarDecl;
typedef struct MpplArrayType        MpplArrayType;
typedef struct MpplProcDecl         MpplProcDecl;
typedef struct MpplFmlParamListElem MpplFmlParamListElem;
typedef struct MpplFmlParamList     MpplFmlParamList;
typedef struct MpplFmlParams        MpplFmlParams;
typedef struct MpplFmlParamSec      MpplFmlParamSec;
typedef struct MpplStmtListElem     MpplStmtListElem;
typedef struct MpplStmtList         MpplStmtList;
typedef struct MpplAssignStmt       MpplAssignStmt;
typedef struct MpplIfStmt           MpplIfStmt;
typedef struct MpplElseClause       MpplElseClause;
typedef struct MpplWhileStmt        MpplWhileStmt;
typedef struct MpplBreakStmt        MpplBreakStmt;
typedef struct MpplCallStmt         MpplCallStmt;
typedef struct MpplActParams        MpplActParams;
typedef struct MpplReturnStmt       MpplReturnStmt;
typedef struct MpplInputStmt        MpplInputStmt;
typedef struct MpplInputs           MpplInputs;
typedef struct MpplOutputStmt       MpplOutputStmt;
typedef struct MpplOutputListElem   MpplOutputListElem;
typedef struct MpplOutputList       MpplOutputList;
typedef struct MpplOutputs          MpplOutputs;
typedef struct MpplOutputValue      MpplOutputValue;
typedef struct MpplCompStmt         MpplCompStmt;
typedef struct MpplExprListElem     MpplExprListElem;
typedef struct MpplExprList         MpplExprList;
typedef struct MpplEntireVar        MpplEntireVar;
typedef struct MpplIndexedVar       MpplIndexedVar;
typedef struct MpplUnaryExpr        MpplUnaryExpr;
typedef struct MpplBinaryExpr       MpplBinaryExpr;
typedef struct MpplParenExpr        MpplParenExpr;
typedef struct MpplCastExpr         MpplCastExpr;
typedef struct MpplIdentListElem    MpplIdentListElem;
typedef struct MpplIdentList        MpplIdentList;
typedef struct MpplBogus            MpplBogus;

typedef struct AnyMpplDecl     AnyMpplDecl;
typedef struct AnyMpplType     AnyMpplType;
typedef struct AnyMpplStmt     AnyMpplStmt;
typedef struct AnyMpplExpr     AnyMpplExpr;
typedef struct AnyMpplUnaryOp  AnyMpplUnaryOp;
typedef struct AnyMpplBinaryOp AnyMpplBinaryOp;

struct MpplErrorToken {
  SyntaxToken token;
};

struct MpplEndOfFileToken {
  SyntaxToken token;
};

struct MpplIdentToken {
  SyntaxToken token;
};

struct MpplNumberLit {
  SyntaxToken token;
};

struct MpplStringLit {
  SyntaxToken token;
};

struct MpplPlusToken {
  SyntaxToken token;
};

struct MpplMinusToken {
  SyntaxToken token;
};

struct MpplStarToken {
  SyntaxToken token;
};

struct MpplEqualToken {
  SyntaxToken token;
};

struct MpplNotEqToken {
  SyntaxToken token;
};

struct MpplLessToken {
  SyntaxToken token;
};

struct MpplLessEqToken {
  SyntaxToken token;
};

struct MpplGreaterToken {
  SyntaxToken token;
};

struct MpplGreaterEqToken {
  SyntaxToken token;
};

struct MpplLParenToken {
  SyntaxToken token;
};

struct MpplRParenToken {
  SyntaxToken token;
};

struct MpplLBracketToken {
  SyntaxToken token;
};

struct MpplRBracketToken {
  SyntaxToken token;
};

struct MpplAssignToken {
  SyntaxToken token;
};

struct MpplDotToken {
  SyntaxToken token;
};

struct MpplCommaToken {
  SyntaxToken token;
};

struct MpplColonToken {
  SyntaxToken token;
};

struct MpplSemiToken {
  SyntaxToken token;
};

struct MpplProgramKw {
  SyntaxToken token;
};

struct MpplVarKw {
  SyntaxToken token;
};

struct MpplArrayKw {
  SyntaxToken token;
};

struct MpplOfKw {
  SyntaxToken token;
};

struct MpplBeginKw {
  SyntaxToken token;
};

struct MpplEndKw {
  SyntaxToken token;
};

struct MpplIfKw {
  SyntaxToken token;
};

struct MpplThenKw {
  SyntaxToken token;
};

struct MpplElseKw {
  SyntaxToken token;
};

struct MpplProcedureKw {
  SyntaxToken token;
};

struct MpplReturnKw {
  SyntaxToken token;
};

struct MpplCallKw {
  SyntaxToken token;
};

struct MpplWhileKw {
  SyntaxToken token;
};

struct MpplDoKw {
  SyntaxToken token;
};

struct MpplNotKw {
  SyntaxToken token;
};

struct MpplOrKw {
  SyntaxToken token;
};

struct MpplDivKw {
  SyntaxToken token;
};

struct MpplAndKw {
  SyntaxToken token;
};

struct MpplCharKw {
  SyntaxToken token;
};

struct MpplIntegerKw {
  SyntaxToken token;
};

struct MpplBooleanKw {
  SyntaxToken token;
};

struct MpplReadKw {
  SyntaxToken token;
};

struct MpplWriteKw {
  SyntaxToken token;
};

struct MpplReadLnKw {
  SyntaxToken token;
};

struct MpplWriteLnKw {
  SyntaxToken token;
};

struct MpplTrueKw {
  SyntaxToken token;
};

struct MpplFalseKw {
  SyntaxToken token;
};

struct MpplBreakKw {
  SyntaxToken token;
};

struct MpplProgram {
  SyntaxTree      tree;
  MpplProgramKw  *program_token;
  MpplIdentToken *name;
  MpplSemiToken  *semi_token;
  MpplDeclList   *decl_list;
  MpplCompStmt   *comp_stmt;
  MpplDotToken   *dot_token;
};

struct MpplDeclList {
  SyntaxTree tree;
  Slice(AnyMpplDecl *) elems;
};

struct MpplVarDeclPart {
  SyntaxTree       tree;
  MpplVarKw       *var_token;
  MpplVarDeclList *var_decl_list;
};

struct MpplVarDeclListElem {
  SyntaxTree     tree;
  MpplVarDecl   *var_decl;
  MpplSemiToken *semi_token;
};

struct MpplVarDeclList {
  SyntaxTree tree;
  Slice(MpplVarDeclListElem *) elems;
};

struct MpplVarDecl {
  SyntaxTree      tree;
  MpplIdentList  *ident_list;
  MpplColonToken *colon_token;
  AnyMpplType    *type;
};

struct MpplArrayType {
  SyntaxTree         tree;
  MpplArrayKw       *array_token;
  MpplLBracketToken *lbracket_token;
  MpplNumberLit     *number_lit;
  MpplRBracketToken *rbracket_token;
  MpplOfKw          *of_token;
  AnyMpplType       *type;
};

struct MpplProcDecl {
  SyntaxTree       tree;
  MpplProcedureKw *proc_token;
  MpplIdentToken  *name;
  MpplFmlParams   *fml_params;
  MpplSemiToken   *first_semi_token;
  MpplVarDeclPart *var_decl_part;
  MpplCompStmt    *comp_stmt;
  MpplSemiToken   *second_semi_token;
};

struct MpplFmlParamListElem {
  SyntaxTree       tree;
  MpplFmlParamSec *fml_param_sec;
  MpplSemiToken   *semi_token;
};

struct MpplFmlParamList {
  SyntaxTree tree;
  Slice(MpplFmlParamListElem *) elems;
};

struct MpplFmlParams {
  SyntaxTree        tree;
  MpplLParenToken  *lparen_token;
  MpplFmlParamList *fml_param_list;
  MpplRParenToken  *rparen_token;
};

struct MpplFmlParamSec {
  SyntaxTree      tree;
  MpplIdentList  *ident_list;
  MpplColonToken *colon_token;
  AnyMpplType    *type;
};

struct MpplStmtListElem {
  SyntaxTree     tree;
  AnyMpplStmt   *stmt;
  MpplSemiToken *semi_token;
};

struct MpplStmtList {
  SyntaxTree tree;
  Slice(MpplStmtListElem *) elems;
};

struct MpplAssignStmt {
  SyntaxTree       tree;
  AnyMpplExpr     *lhs;
  MpplAssignToken *assign_token;
  AnyMpplExpr     *rhs;
};

struct MpplIfStmt {
  SyntaxTree      tree;
  MpplIfKw       *if_token;
  AnyMpplExpr    *cond;
  MpplThenKw     *then_token;
  AnyMpplStmt    *then_stmt;
  MpplElseClause *else_clause;
};

struct MpplElseClause {
  SyntaxTree   tree;
  MpplElseKw  *else_token;
  AnyMpplStmt *stmt;
};

struct MpplWhileStmt {
  SyntaxTree   tree;
  MpplWhileKw *while_token;
  AnyMpplExpr *cond;
  MpplDoKw    *do_token;
  AnyMpplStmt *stmt;
};

struct MpplBreakStmt {
  SyntaxTree   tree;
  MpplBreakKw *break_token;
};

struct MpplCallStmt {
  SyntaxTree      tree;
  MpplCallKw     *call_token;
  MpplIdentToken *name;
  MpplActParams  *act_params;
};

struct MpplActParams {
  SyntaxTree       tree;
  MpplLParenToken *lparen_token;
  MpplExprList    *expr_list;
  MpplRParenToken *rparen_token;
};

struct MpplReturnStmt {
  SyntaxTree    tree;
  MpplReturnKw *return_token;
};

struct MpplInputStmt {
  SyntaxTree  tree;
  MpplReadKw *read_token;
  MpplInputs *inputs;
};

struct MpplInputs {
  SyntaxTree       tree;
  MpplLParenToken *lparen_token;
  MpplExprList    *expr_list;
  MpplRParenToken *rparen_token;
};

struct MpplOutputStmt {
  SyntaxTree   tree;
  MpplWriteKw *write_token;
  MpplOutputs *outputs;
};

struct MpplOutputListElem {
  SyntaxTree       tree;
  MpplOutputValue *output_value;
  MpplCommaToken  *comma_token;
};

struct MpplOutputList {
  SyntaxTree tree;
  Slice(MpplOutputListElem *) elems;
};

struct MpplOutputs {
  SyntaxTree       tree;
  MpplLParenToken *lparen_token;
  MpplOutputList  *output_list;
  MpplRParenToken *rparen_token;
};

struct MpplOutputValue {
  SyntaxTree      tree;
  AnyMpplExpr    *expr;
  MpplColonToken *colon_token;
  MpplNumberLit  *number_lit;
};

struct MpplCompStmt {
  SyntaxTree    tree;
  MpplBeginKw  *begin_token;
  MpplStmtList *stmt_list;
  MpplEndKw    *end_token;
};

struct MpplExprListElem {
  SyntaxTree      tree;
  AnyMpplExpr    *expr;
  MpplCommaToken *comma_token;
};

struct MpplExprList {
  SyntaxTree tree;
  Slice(MpplExprListElem *) elems;
};

struct MpplEntireVar {
  SyntaxTree      tree;
  MpplIdentToken *name;
};

struct MpplIndexedVar {
  SyntaxTree         tree;
  MpplIdentToken    *name;
  MpplLBracketToken *lbracket_token;
  AnyMpplExpr       *index;
  MpplRBracketToken *rbracket_token;
};

struct MpplUnaryExpr {
  SyntaxTree      tree;
  AnyMpplUnaryOp *op;
  AnyMpplExpr    *expr;
};

struct MpplBinaryExpr {
  SyntaxTree       tree;
  AnyMpplExpr     *lhs;
  AnyMpplBinaryOp *op;
  AnyMpplExpr     *rhs;
};

struct MpplParenExpr {
  SyntaxTree       tree;
  MpplLParenToken *lparen_token;
  AnyMpplExpr     *expr;
  MpplRParenToken *rparen_token;
};

struct MpplCastExpr {
  SyntaxTree       tree;
  AnyMpplType     *type;
  MpplLParenToken *lparen_token;
  AnyMpplExpr     *expr;
  MpplRParenToken *rparen_token;
};

struct MpplIdentListElem {
  SyntaxTree      tree;
  MpplIdentToken *ident;
  MpplCommaToken *comma_token;
};

struct MpplIdentList {
  SyntaxTree tree;
  Slice(MpplIdentListElem *) elems;
};

struct MpplBogus {
  SyntaxTree tree;
  Slice(MpplErrorToken *) elems;
};

struct AnyMpplDecl {
  SyntaxNode node;
};

struct AnyMpplType {
  SyntaxNode node;
};

struct AnyMpplStmt {
  SyntaxNode node;
};

struct AnyMpplExpr {
  SyntaxNode node;
};

struct AnyMpplUnaryOp {
  SyntaxNode node;
};

struct AnyMpplBinaryOp {
  SyntaxNode node;
};

#endif
