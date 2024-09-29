/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_SYNTAX_H
#define MPPL_SYNTAX_H

#include "syntax_tree.h"
#include "util.h"

/* mppl syntax kind */

typedef enum {
  MPPL_SYNTAX_ERROR = SYNTAX_TOKEN << 8,

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

  MPPL_SYNTAX_PROGRAM = SYNTAX_TREE << 8,
  MPPL_SYNTAX_DECL_PART_LIST,
  MPPL_SYNTAX_VAR_DECL_PART,
  MPPL_SYNTAX_VAR_DECL_LIST_ELEM,
  MPPL_SYNTAX_VAR_DECL_LIST,
  MPPL_SYNTAX_VAR_DECL,
  MPPL_SYNTAX_ARRAY_TYPE,
  MPPL_SYNTAX_PROC_DECL_PART,
  MPPL_SYNTAX_PROC_HEADING,
  MPPL_SYNTAX_PROC_BODY,
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
  MPPL_EXPR_alloc
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

typedef struct AnyMpplDecl     AnyMpplDecl;
typedef struct AnyMpplType     AnyMpplType;
typedef struct AnyMpplStmt     AnyMpplStmt;
typedef struct AnyMpplExpr     AnyMpplExpr;
typedef struct AnyMpplUnaryOp  AnyMpplUnaryOp;
typedef struct AnyMpplBinaryOp AnyMpplBinaryOp;

typedef struct MpplSyntax           MpplSyntax;
typedef struct MpplProgram          MpplProgram;
typedef struct MpplDeclPartList     MpplDeclPartList;
typedef struct MpplVarDeclPart      MpplVarDeclPart;
typedef struct MpplVarDeclListElem  MpplVarDeclListElem;
typedef struct MpplVarDeclList      MpplVarDeclList;
typedef struct MpplVarDecl          MpplVarDecl;
typedef struct MpplArrayType        MpplArrayType;
typedef struct MpplProcDeclPart     MpplProcDeclPart;
typedef struct MpplProcHeading      MpplProcHeading;
typedef struct MpplProcBody         MpplProcBody;
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

#endif /* MPPL_SYNTAX_H */
