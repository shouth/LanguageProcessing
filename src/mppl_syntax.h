/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_SYNTAX_H
#define MPPL_SYNTAX_H

#include "syntax_tree.h"
#include "util.h"

/* mppl syntax kind */

typedef enum {
  MPPL_SYNTAX_ERROR = SYNTAX_TOKEN << 8,
  MPPL_SYNTAX_EOF_TOKEN,
  MPPL_SYNTAX_IDENT_TOKEN,
  MPPL_SYNTAX_INTEGER_LIT,
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

  MPPL_SYNTAX_EOF = SYNTAX_TREE << 8,
  MPPL_SYNTAX_PROGRAM,
  MPPL_SYNTAX_DECL_PART_LIST,
  MPPL_SYNTAX_BIND_IDENT_LIST_ELEM,
  MPPL_SYNTAX_BIND_IDENT_LIST,
  MPPL_SYNTAX_BIND_IDENT,
  MPPL_SYNTAX_VAR_DECL_PART,
  MPPL_SYNTAX_VAR_DECL_LIST_ELEM,
  MPPL_SYNTAX_VAR_DECL_LIST,
  MPPL_SYNTAX_VAR_DECL,
  MPPL_SYNTAX_INTEGER_TYPE,
  MPPL_SYNTAX_CHAR_TYPE,
  MPPL_SYNTAX_BOOLEAN_TYPE,
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
  MPPL_SYNTAX_OUTPUT_VALUE_FIELD_WIDTH,
  MPPL_SYNTAX_OUTPUT_VALUE,
  MPPL_SYNTAX_COMP_STMT,
  MPPL_SYNTAX_EXPR_LIST_ELEM,
  MPPL_SYNTAX_EXPR_LIST,
  MPPL_SYNTAX_REF_IDENT,
  MPPL_SYNTAX_INTEGER_LIT_EXPR,
  MPPL_SYNTAX_BOOLEAN_LIT_EXPR,
  MPPL_SYNTAX_STRING_LIT_EXPR,
  MPPL_SYNTAX_ENTIRE_VAR_EXPR,
  MPPL_SYNTAX_INDEXED_VAR_EXPR,
  MPPL_SYNTAX_UNARY_EXPR,
  MPPL_SYNTAX_BINARY_EXPR,
  MPPL_SYNTAX_PAREN_EXPR,
  MPPL_SYNTAX_CAST_EXPR,

  MPPL_SYNTAX_BOGUS_EOF,
  MPPL_SYNTAX_BOGUS_DECL_PART,
  MPPL_SYNTAX_BOGUS_VAR_DECL,
  MPPL_SYNTAX_BOGUS_FML_PARAM_SEC,
  MPPL_SYNTAX_BOGUS_STMT,
  MPPL_SYNTAX_BOGUS_OUTPUT_VALUE,
  MPPL_SYNTAX_BOGUS_EXPR,
  MPPL_SYNTAX_BOGUS_BIND_IDENT
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
  MPPL_EOF_SYNTAX,
  MPPL_EOF_SYNTAX_BOGUS
} MpplEofKind;

typedef enum {
  MPPL_DECL_PART_SYNTAX_VAR,
  MPPL_DECL_PART_SYNTAX_PROC,
  MPPL_DECL_PART_SYNTAX_BOGUS
} MpplDeclPartKind;

typedef enum {
  MPPL_TYPE_SYNTAX_INTEGER,
  MPPL_TYPE_SYNTAX_CHAR,
  MPPL_TYPE_SYNTAX_BOOLEAN,
  MPPL_TYPE_SYNTAX_ARRAY
} MpplTypeKind;

typedef enum {
  MPPL_VAR_DECL_SYNTAX,
  MPPL_VAR_DECL_SYNTAX_BOGUS
} MpplVarDeclKind;

typedef enum {
  MPPL_FML_PARAM_SEC_SYNTAX,
  MPPL_FML_PARAM_SEC_SYNTAX_BOGUS
} MpplFmlParamSecKind;

typedef enum {
  MPPL_STMT_SYNTAX_ASSIGN,
  MPPL_STMT_SYNTAX_IF,
  MPPL_STMT_SYNTAX_WHILE,
  MPPL_STMT_SYNTAX_BREAK,
  MPPL_STMT_SYNTAX_CALL,
  MPPL_STMT_SYNTAX_RETURN,
  MPPL_STMT_SYNTAX_INPUT,
  MPPL_STMT_SYNTAX_OUTPUT,
  MPPL_STMT_SYNTAX_COMP,
  MPPL_STMT_SYNTAX_BOGUS
} MpplStmtKind;

typedef enum {
  MPPL_EXPR_SYNTAX_INTEGER_LIT,
  MPPL_EXPR_SYNTAX_BOOLEAN_LIT,
  MPPL_EXPR_SYNTAX_STRING_LIT,
  MPPL_EXPR_SYNTAX_ENTIRE_VAR,
  MPPL_EXPR_SYNTAX_INDEXED_VAR,
  MPPL_EXPR_SYNTAX_UNARY,
  MPPL_EXPR_SYNTAX_BINARY,
  MPPL_EXPR_SYNTAX_PAREN,
  MPPL_EXPR_SYNTAX_CAST,
  MPPL_EXPR_SYNTAX_BOGUS
} MpplExprKind;

typedef enum {
  MPPL_OUTPUT_VALUE_SYNTAX,
  MPPL_OUTPUT_VALUE_SYNTAX_BOGUS
} MpplOutputValueKind;

typedef enum {
  MPPL_BIND_IDENT_SYNTAX,
  MPPL_BIND_IDENT_SYNTAX_BOGUS
} MpplBindIdentKind;

#define MpplSyntax     \
  struct {             \
    SyntaxTree syntax; \
  }

typedef MpplSyntax MpplRoot;
typedef MpplSyntax MpplEof;
typedef MpplSyntax MpplProgram;
typedef MpplSyntax MpplDeclPartList;
typedef MpplSyntax MpplBindIdentListElem;
typedef MpplSyntax MpplBindIdentList;
typedef MpplSyntax MpplBindIdent;
typedef MpplSyntax MpplVarDeclPart;
typedef MpplSyntax MpplVarDeclListElem;
typedef MpplSyntax MpplVarDeclList;
typedef MpplSyntax MpplVarDecl;
typedef MpplSyntax MpplIntegerType;
typedef MpplSyntax MpplCharType;
typedef MpplSyntax MpplBooleanType;
typedef MpplSyntax MpplArrayType;
typedef MpplSyntax MpplProcDeclPart;
typedef MpplSyntax MpplProcHeading;
typedef MpplSyntax MpplProcBody;
typedef MpplSyntax MpplProcDecl;
typedef MpplSyntax MpplFmlParamListElem;
typedef MpplSyntax MpplFmlParamList;
typedef MpplSyntax MpplFmlParams;
typedef MpplSyntax MpplFmlParamSec;
typedef MpplSyntax MpplStmtListElem;
typedef MpplSyntax MpplStmtList;
typedef MpplSyntax MpplAssignStmt;
typedef MpplSyntax MpplIfStmt;
typedef MpplSyntax MpplElseClause;
typedef MpplSyntax MpplWhileStmt;
typedef MpplSyntax MpplBreakStmt;
typedef MpplSyntax MpplCallStmt;
typedef MpplSyntax MpplActParams;
typedef MpplSyntax MpplReturnStmt;
typedef MpplSyntax MpplInputStmt;
typedef MpplSyntax MpplInputs;
typedef MpplSyntax MpplOutputStmt;
typedef MpplSyntax MpplOutputListElem;
typedef MpplSyntax MpplOutputList;
typedef MpplSyntax MpplOutputs;
typedef MpplSyntax MpplOutputValueFieldWidth;
typedef MpplSyntax MpplOutputValue;
typedef MpplSyntax MpplCompStmt;
typedef MpplSyntax MpplExprListElem;
typedef MpplSyntax MpplExprList;
typedef MpplSyntax MpplRefIdent;
typedef MpplSyntax MpplIntegerLitExpr;
typedef MpplSyntax MpplBooleanLitExpr;
typedef MpplSyntax MpplStringLitExpr;
typedef MpplSyntax MpplEntireVarExpr;
typedef MpplSyntax MpplIndexedVarExpr;
typedef MpplSyntax MpplUnaryExpr;
typedef MpplSyntax MpplBinaryExpr;
typedef MpplSyntax MpplParenExpr;
typedef MpplSyntax MpplCastExpr;

typedef MpplSyntax BogusMpplEof;
typedef MpplSyntax BogusMpplDeclPart;
typedef MpplSyntax BogusMpplVarDecl;
typedef MpplSyntax BogusMpplFmlParamSec;
typedef MpplSyntax BogusMpplStmt;
typedef MpplSyntax BogusMpplOutputValue;
typedef MpplSyntax BogusMpplExpr;
typedef MpplSyntax BogusMpplBindIdent;

typedef union AnyMpplEof         AnyMpplEof;
typedef union AnyMpplDeclPart    AnyMpplDeclPart;
typedef union AnyMpplType        AnyMpplType;
typedef union AnyMpplVarDecl     AnyMpplVarDecl;
typedef union AnyMpplFmlParamSec AnyMpplFmlParamSec;
typedef union AnyMpplStmt        AnyMpplStmt;
typedef union AnyMpplOutputValue AnyMpplOutputValue;
typedef union AnyMpplExpr        AnyMpplExpr;
typedef union AnyMpplBindIdent   AnyMpplBindIdent;

typedef struct MpplRootFields                  MpplRootFields;
typedef struct MpplEofFields                   MpplEofFields;
typedef struct MpplProgramFields               MpplProgramFields;
typedef struct MpplBindIdentListElemFields     MpplBindIdentListElemFields;
typedef struct MpplBindIdentFields             MpplBindIdentFields;
typedef struct MpplVarDeclPartFields           MpplVarDeclPartFields;
typedef struct MpplVarDeclListElemFields       MpplVarDeclListElemFields;
typedef struct MpplVarDeclFields               MpplVarDeclFields;
typedef struct MpplIntegerTypeFields           MpplIntegerTypeFields;
typedef struct MpplCharTypeFields              MpplCharTypeFields;
typedef struct MpplBooleanTypeFields           MpplBooleanTypeFields;
typedef struct MpplArrayTypeFields             MpplArrayTypeFields;
typedef struct MpplProcDeclPartFields          MpplProcDeclPartFields;
typedef struct MpplProcHeadingFields           MpplProcHeadingFields;
typedef struct MpplProcBodyFields              MpplProcBodyFields;
typedef struct MpplProcDeclFields              MpplProcDeclFields;
typedef struct MpplFmlParamListElemFields      MpplFmlParamListElemFields;
typedef struct MpplFmlParamsFields             MpplFmlParamsFields;
typedef struct MpplFmlParamSecFields           MpplFmlParamSecFields;
typedef struct MpplStmtListElemFields          MpplStmtListElemFields;
typedef struct MpplAssignStmtFields            MpplAssignStmtFields;
typedef struct MpplIfStmtFields                MpplIfStmtFields;
typedef struct MpplElseClauseFields            MpplElseClauseFields;
typedef struct MpplWhileStmtFields             MpplWhileStmtFields;
typedef struct MpplBreakStmtFields             MpplBreakStmtFields;
typedef struct MpplCallStmtFields              MpplCallStmtFields;
typedef struct MpplActParamsFields             MpplActParamsFields;
typedef struct MpplReturnStmtFields            MpplReturnStmtFields;
typedef struct MpplInputStmtFields             MpplInputStmtFields;
typedef struct MpplInputsFields                MpplInputsFields;
typedef struct MpplOutputStmtFields            MpplOutputStmtFields;
typedef struct MpplOutputListElemFields        MpplOutputListElemFields;
typedef struct MpplOutputsFields               MpplOutputsFields;
typedef struct MpplOutputValueFieldWidthFields MpplOutputValueFieldWidthFields;
typedef struct MpplOutputValueFields           MpplOutputValueFields;
typedef struct MpplCompStmtFields              MpplCompStmtFields;
typedef struct MpplExprListElemFields          MpplExprListElemFields;
typedef struct MpplRefIdentFields              MpplRefIdentFields;
typedef struct MpplIntegerLitExprFields        MpplIntegerLitExprFields;
typedef struct MpplBooleanLitExprFields        MpplBooleanLitExprFields;
typedef struct MpplStringLitExprFields         MpplStringLitExprFields;
typedef struct MpplEntireVarExprFields         MpplEntireVarExprFields;
typedef struct MpplIndexedVarExprFields        MpplIndexedVarExprFields;
typedef struct MpplUnaryExprFields             MpplUnaryExprFields;
typedef struct MpplBinaryExprFields            MpplBinaryExprFields;
typedef struct MpplParenExprFields             MpplParenExprFields;
typedef struct MpplCastExprFields              MpplCastExprFields;

typedef Slice(AnyMpplDeclPart *) MpplDeclPartListFields;
typedef Slice(MpplBindIdentListElem *) MpplBindIdentListFields;
typedef Slice(MpplVarDeclListElem *) MpplVarDeclListFields;
typedef Slice(MpplFmlParamListElem *) MpplFmlParamListFields;
typedef Slice(MpplStmtListElem *) MpplStmtListFields;
typedef Slice(MpplOutputListElem *) MpplOutputListFields;
typedef Slice(MpplExprListElem *) MpplExprListFields;

#undef MpplSyntax

union AnyMpplEof {
  MpplEof      eof;
  BogusMpplEof bogus;
};

union AnyMpplDeclPart {
  MpplVarDeclPart   var_decl_part;
  MpplProcDeclPart  proc_decl_part;
  BogusMpplDeclPart bogus;
};

union AnyMpplType {
  MpplIntegerType integer_type;
  MpplCharType    char_type;
  MpplBooleanType boolean_type;
  MpplArrayType   array_type;
};

union AnyMpplVarDecl {
  MpplVarDecl      var_decl;
  BogusMpplVarDecl bogus;
};

union AnyMpplFmlParamSec {
  MpplFmlParamSec      fml_param_sec;
  BogusMpplFmlParamSec bogus;
};

union AnyMpplStmt {
  MpplAssignStmt assign_stmt;
  MpplIfStmt     if_stmt;
  MpplWhileStmt  while_stmt;
  MpplBreakStmt  break_stmt;
  MpplCallStmt   call_stmt;
  MpplReturnStmt return_stmt;
  MpplInputStmt  input_stmt;
  MpplOutputStmt output_stmt;
  MpplCompStmt   comp_stmt;
  BogusMpplStmt  bogus;
};

union AnyMpplExpr {
  MpplIntegerLitExpr integer_lit_expr;
  MpplBooleanLitExpr boolean_lit_expr;
  MpplStringLitExpr  string_lit_expr;
  MpplEntireVarExpr  entire_var_expr;
  MpplIndexedVarExpr indexed_var_expr;
  MpplUnaryExpr      unary_expr;
  MpplBinaryExpr     binary_expr;
  MpplParenExpr      paren_expr;
  MpplCastExpr       cast_expr;
  BogusMpplExpr      bogus;
};

union AnyMpplOutputValue {
  MpplOutputValue      output_value;
  BogusMpplOutputValue bogus;
};

union AnyMpplOutput {
  AnyMpplExpr        expr;
  AnyMpplOutputValue output_value;
};

union AnyMpplBindIdent {
  MpplBindIdent      ident;
  BogusMpplBindIdent bogus_ident;
};

struct MpplRootFields {
  MpplProgram *program;
  AnyMpplEof  *eof;
};

struct MpplEofFields {
  SyntaxToken *eof_token;
};

struct MpplProgramFields {
  SyntaxToken      *program_kw;
  MpplBindIdent    *name;
  SyntaxToken      *semi_token;
  MpplDeclPartList *decl_part_list;
  MpplCompStmt     *comp_stmt;
  SyntaxToken      *dot_token;
};

struct MpplBindIdentListElemFields {
  AnyMpplBindIdent *bind_ident;
  SyntaxToken      *comma_token;
};

struct MpplBindIdentFields {
  SyntaxToken *ident;
};

struct MpplVarDeclPartFields {
  SyntaxToken     *var_kw;
  MpplVarDeclList *var_decl_list;
};

struct MpplVarDeclListElemFields {
  MpplVarDecl *var_decl;
  SyntaxToken *semi_token;
};

struct MpplVarDeclFields {
  MpplBindIdentList *ident_list;
  SyntaxToken       *colon_token;
  AnyMpplType       *type;
};

struct MpplIntegerTypeFields {
  SyntaxToken *integer_kw;
};

struct MpplCharTypeFields {
  SyntaxToken *char_kw;
};

struct MpplBooleanTypeFields {
  SyntaxToken *boolean_kw;
};

struct MpplArrayTypeFields {
  SyntaxToken *array_kw;
  SyntaxToken *lbracket_token;
  SyntaxToken *number_lit;
  SyntaxToken *rbracket_token;
  SyntaxToken *of_kw;
  AnyMpplType *type;
};

struct MpplProcDeclPartFields {
  MpplProcDecl *proc_decl;
  SyntaxToken  *semi_token;
};

struct MpplProcHeadingFields {
  SyntaxToken   *procedure_kw;
  MpplBindIdent *name;
  MpplFmlParams *fml_params;
};

struct MpplProcBodyFields {
  MpplVarDeclPart *var_decl_part;
  MpplCompStmt    *comp_stmt;
};

struct MpplProcDeclFields {
  MpplProcHeading *proc_heading;
  SyntaxToken     *semi_token;
  MpplProcBody    *proc_body;
};

struct MpplFmlParamListElemFields {
  MpplFmlParamSec *fml_param_sec;
  SyntaxToken     *semi_token;
};

struct MpplFmlParamsFields {
  SyntaxToken      *lparen_token;
  MpplFmlParamList *fml_param_list;
  SyntaxToken      *rparen_token;
};

struct MpplFmlParamSecFields {
  MpplBindIdentList *ident_list;
  SyntaxToken       *colon_token;
  AnyMpplType       *type;
};

struct MpplStmtListElemFields {
  AnyMpplStmt *stmt;
  SyntaxToken *semi_token;
};

struct MpplAssignStmtFields {
  AnyMpplExpr *lhs;
  SyntaxToken *assign_token;
  AnyMpplExpr *rhs;
};

struct MpplIfStmtFields {
  SyntaxToken    *if_kw;
  AnyMpplExpr    *cond;
  SyntaxToken    *then_kw;
  AnyMpplStmt    *then_stmt;
  MpplElseClause *else_clause;
};

struct MpplElseClauseFields {
  SyntaxToken *else_kw;
  AnyMpplStmt *else_stmt;
};

struct MpplWhileStmtFields {
  SyntaxToken *while_kw;
  AnyMpplExpr *cond;
  SyntaxToken *do_kw;
  AnyMpplStmt *stmt;
};

struct MpplBreakStmtFields {
  SyntaxToken *break_kw;
};

struct MpplCallStmtFields {
  SyntaxToken   *call_kw;
  MpplRefIdent  *name;
  MpplActParams *act_params;
};

struct MpplActParamsFields {
  SyntaxToken  *lparen_token;
  MpplExprList *expr_list;
  SyntaxToken  *rparen_token;
};

struct MpplReturnStmtFields {
  SyntaxToken *return_kw;
};

struct MpplInputStmtFields {
  SyntaxToken *read_op_token;
  MpplInputs  *inputs;
};

struct MpplInputsFields {
  SyntaxToken  *lparen_token;
  MpplExprList *expr_list;
  SyntaxToken  *rparen_token;
};

struct MpplOutputStmtFields {
  SyntaxToken *write_op_token;
  MpplOutputs *outputs;
};

struct MpplOutputListElemFields {
  AnyMpplOutputValue *output_value;
  SyntaxToken        *comma_token;
};

struct MpplOutputsFields {
  SyntaxToken    *lparen_token;
  MpplOutputList *output_list;
  SyntaxToken    *rparen_token;
};

struct MpplOutputValueFieldWidthFields {
  SyntaxToken *colon_token;
  SyntaxToken *field_width;
};

struct MpplOutputValueFields {
  AnyMpplExpr               *expr;
  MpplOutputValueFieldWidth *field_width;
};

struct MpplCompStmtFields {
  SyntaxToken  *begin_kw;
  MpplStmtList *stmt_list;
  SyntaxToken  *end_kw;
};

struct MpplExprListElemFields {
  AnyMpplExpr *expr;
  SyntaxToken *comma_token;
};

struct MpplRefIdentFields {
  SyntaxToken *ident;
};

struct MpplIntegerLitExprFields {
  SyntaxToken *integer_lit;
};

struct MpplBooleanLitExprFields {
  SyntaxToken *boolean_lit;
};

struct MpplStringLitExprFields {
  SyntaxToken *string_lit;
};

struct MpplEntireVarExprFields {
  MpplRefIdent *name;
};

struct MpplIndexedVarExprFields {
  MpplRefIdent *name;
  SyntaxToken  *lbracket_token;
  AnyMpplExpr  *index;
  SyntaxToken  *rbracket_token;
};

struct MpplUnaryExprFields {
  SyntaxToken *op_token;
  AnyMpplExpr *expr;
};

struct MpplBinaryExprFields {
  AnyMpplExpr *lhs;
  SyntaxToken *op_token;
  AnyMpplExpr *rhs;
};

struct MpplParenExprFields {
  SyntaxToken *lparen_token;
  AnyMpplExpr *expr;
  SyntaxToken *rparen_token;
};

struct MpplCastExprFields {
  AnyMpplType *type;
  SyntaxToken *lparen_token;
  AnyMpplExpr *expr;
  SyntaxToken *rparen_token;
};

MpplRootFields                  mppl_root_fields_alloc(const MpplRoot *root);
MpplProgramFields               mppl_program_fields_alloc(const MpplProgram *program);
MpplEofFields                   mppl_eof_fields_alloc(const MpplEof *eof);
MpplDeclPartListFields          mppl_decl_part_list_fields_alloc(const MpplDeclPartList *decl_part_list);
MpplBindIdentListElemFields     mppl_bind_ident_list_elem_fields_alloc(const MpplBindIdentListElem *bind_ident_list_elem);
MpplBindIdentListFields         mppl_bind_ident_list_fields_alloc(const MpplBindIdentList *bind_ident_list);
MpplBindIdentFields             mppl_bind_ident_fields_alloc(const MpplBindIdent *bind_ident);
MpplVarDeclPartFields           mppl_var_decl_part_fields_alloc(const MpplVarDeclPart *var_decl_part);
MpplVarDeclListElemFields       mppl_var_decl_list_elem_fields_alloc(const MpplVarDeclListElem *var_decl_list_elem);
MpplVarDeclListFields           mppl_var_decl_list_fields_alloc(const MpplVarDeclList *var_decl_list);
MpplVarDeclFields               mppl_var_decl_fields_alloc(const MpplVarDecl *var_decl);
MpplIntegerTypeFields           mppl_integer_type_fields_alloc(const MpplArrayType *integer_type);
MpplCharTypeFields              mppl_char_type_fields_alloc(const MpplArrayType *char_type);
MpplBooleanTypeFields           mppl_boolean_type_fields_alloc(const MpplArrayType *boolean_type);
MpplArrayTypeFields             mppl_array_type_fields_alloc(const MpplArrayType *array_type);
MpplProcDeclPartFields          mppl_proc_decl_part_fields_alloc(const MpplProcDeclPart *proc_decl_part);
MpplProcHeadingFields           mppl_proc_heading_fields_alloc(const MpplProcHeading *proc_heading);
MpplProcBodyFields              mppl_proc_body_fields_alloc(const MpplProcBody *proc_body);
MpplProcDeclFields              mppl_proc_decl_fields_alloc(const MpplProcDecl *proc_decl);
MpplFmlParamListElemFields      mppl_fml_param_list_elem_fields_alloc(const MpplFmlParamListElem *fml_param_list_elem);
MpplFmlParamListFields          mppl_fml_param_list_fields_alloc(const MpplFmlParamList *fml_param_list);
MpplFmlParamsFields             mppl_fml_params_fields_alloc(const MpplFmlParams *fml_params);
MpplFmlParamSecFields           mppl_fml_param_sec_fields_alloc(const MpplFmlParamSec *fml_param_sec);
MpplStmtListElemFields          mppl_stmt_list_elem_fields_alloc(const MpplStmtListElem *stmt_list_elem);
MpplStmtListFields              mppl_stmt_list_fields_alloc(const MpplStmtList *stmt_list);
MpplAssignStmtFields            mppl_assign_stmt_fields_alloc(const MpplAssignStmt *assign_stmt);
MpplIfStmtFields                mppl_if_stmt_fields_alloc(const MpplIfStmt *if_stmt);
MpplElseClauseFields            mppl_else_clause_fields_alloc(const MpplElseClause *else_clause);
MpplWhileStmtFields             mppl_while_stmt_fields_alloc(const MpplWhileStmt *while_stmt);
MpplBreakStmtFields             mppl_break_stmt_fields_alloc(const MpplBreakStmt *break_stmt);
MpplCallStmtFields              mppl_call_stmt_fields_alloc(const MpplCallStmt *call_stmt);
MpplActParamsFields             mppl_act_params_fields_alloc(const MpplActParams *act_params);
MpplReturnStmtFields            mppl_return_stmt_fields_alloc(const MpplReturnStmt *return_stmt);
MpplInputStmtFields             mppl_input_stmt_fields_alloc(const MpplInputStmt *input_stmt);
MpplInputsFields                mppl_inputs_fields_alloc(const MpplInputs *inputs);
MpplOutputStmtFields            mppl_output_stmt_fields_alloc(const MpplOutputStmt *output_stmt);
MpplOutputListElemFields        mppl_output_list_elem_fields_alloc(const MpplOutputListElem *output_list_elem);
MpplOutputListFields            mppl_output_list_fields_alloc(const MpplOutputList *output_list);
MpplOutputsFields               mppl_outputs_fields_alloc(const MpplOutputs *outputs);
MpplOutputValueFieldWidthFields mppl_output_value_field_width_fields_alloc(const MpplOutputValueFieldWidth *output_value_field_width);
MpplOutputValueFields           mppl_output_value_fields_alloc(const MpplOutputValue *output_value);
MpplCompStmtFields              mppl_comp_stmt_fields_alloc(const MpplCompStmt *comp_stmt);
MpplExprListElemFields          mppl_expr_list_elem_fields_alloc(const MpplExprListElem *expr_list_elem);
MpplExprListFields              mppl_expr_list_fields_alloc(const MpplExprList *expr_list);
MpplRefIdentFields              mppl_ref_ident_fields_alloc(const MpplRefIdent *ref_ident);
MpplIntegerLitExprFields        mppl_integer_lit_expr_fields_alloc(const MpplIntegerLitExpr *integer_lit_expr);
MpplBooleanLitExprFields        mppl_boolean_lit_expr_fields_alloc(const MpplBooleanLitExpr *boolean_lit_expr);
MpplStringLitExprFields         mppl_string_lit_expr_fields_alloc(const MpplStringLitExpr *string_lit_expr);
MpplEntireVarExprFields         mppl_entire_var_expr_fields_alloc(const MpplEntireVarExpr *entire_var);
MpplIndexedVarExprFields        mppl_indexed_var_expr_fields_alloc(const MpplIndexedVarExpr *indexed_var);
MpplUnaryExprFields             mppl_unary_expr_fields_alloc(const MpplUnaryExpr *unary_expr);
MpplBinaryExprFields            mppl_binary_expr_fields_alloc(const MpplBinaryExpr *binary_expr);
MpplParenExprFields             mppl_paren_expr_fields_alloc(const MpplParenExpr *paren_expr);
MpplCastExprFields              mppl_cast_expr_fields_alloc(const MpplCastExpr *cast_expr);

void mppl_root_fields_free(MpplRootFields *fields);
void mppl_eof_fields_free(MpplEofFields *fields);
void mppl_program_fields_free(MpplProgramFields *fields);
void mppl_decl_part_list_fields_free(MpplDeclPartListFields *fields);
void mppl_bind_ident_list_elem_fields_free(MpplBindIdentListElemFields *fields);
void mppl_bind_ident_list_fields_free(MpplBindIdentListFields *fields);
void mppl_bind_ident_fields_free(MpplBindIdentFields *fields);
void mppl_var_decl_part_fields_free(MpplVarDeclPartFields *fields);
void mppl_var_decl_list_elem_fields_free(MpplVarDeclListElemFields *fields);
void mppl_var_decl_list_fields_free(MpplVarDeclListFields *fields);
void mppl_var_decl_fields_free(MpplVarDeclFields *fields);
void mppl_integer_type_fields_free(MpplIntegerTypeFields *fields);
void mppl_char_type_fields_free(MpplCharTypeFields *fields);
void mppl_boolean_type_fields_free(MpplBooleanTypeFields *fields);
void mppl_array_type_fields_free(MpplArrayTypeFields *fields);
void mppl_proc_decl_part_fields_free(MpplProcDeclPartFields *fields);
void mppl_proc_heading_fields_free(MpplProcHeadingFields *fields);
void mppl_proc_body_fields_free(MpplProcBodyFields *fields);
void mppl_proc_decl_fields_free(MpplProcDeclFields *fields);
void mppl_fml_param_list_elem_fields_free(MpplFmlParamListElemFields *fields);
void mppl_fml_param_list_fields_free(MpplFmlParamListFields *fields);
void mppl_fml_params_fields_free(MpplFmlParamsFields *fields);
void mppl_fml_param_sec_fields_free(MpplFmlParamSecFields *fields);
void mppl_stmt_list_elem_fields_free(MpplStmtListElemFields *fields);
void mppl_stmt_list_fields_free(MpplStmtListFields *fields);
void mppl_assign_stmt_fields_free(MpplAssignStmtFields *fields);
void mppl_if_stmt_fields_free(MpplIfStmtFields *fields);
void mppl_else_clause_fields_free(MpplElseClauseFields *fields);
void mppl_while_stmt_fields_free(MpplWhileStmtFields *fields);
void mppl_break_stmt_fields_free(MpplBreakStmtFields *fields);
void mppl_call_stmt_fields_free(MpplCallStmtFields *fields);
void mppl_act_params_fields_free(MpplActParamsFields *fields);
void mppl_return_stmt_fields_free(MpplReturnStmtFields *fields);
void mppl_input_stmt_fields_free(MpplInputStmtFields *fields);
void mppl_inputs_fields_free(MpplInputsFields *fields);
void mppl_output_stmt_fields_free(MpplOutputStmtFields *fields);
void mppl_output_list_elem_fields_free(MpplOutputListElemFields *fields);
void mppl_output_list_fields_free(MpplOutputListFields *fields);
void mppl_outputs_fields_free(MpplOutputsFields *fields);
void mppl_output_value_field_width_fields_free(MpplOutputValueFieldWidthFields *fields);
void mppl_output_value_fields_free(MpplOutputValueFields *fields);
void mppl_comp_stmt_fields_free(MpplCompStmtFields *fields);
void mppl_expr_list_elem_fields_free(MpplExprListElemFields *fields);
void mppl_expr_list_fields_free(MpplExprListFields *fields);
void mppl_ref_ident_fields_free(MpplRefIdentFields *fields);
void mppl_integer_lit_expr_fields_free(MpplIntegerLitExprFields *fields);
void mppl_boolean_lit_expr_fields_free(MpplBooleanLitExprFields *fields);
void mppl_string_lit_expr_fields_free(MpplStringLitExprFields *fields);
void mppl_entire_var_expr_fields_free(MpplEntireVarExprFields *fields);
void mppl_indexed_var_expr_fields_free(MpplIndexedVarExprFields *fields);
void mppl_unary_expr_fields_free(MpplUnaryExprFields *fields);
void mppl_binary_expr_fields_free(MpplBinaryExprFields *fields);
void mppl_paren_expr_fields_free(MpplParenExprFields *fields);
void mppl_cast_expr_fields_free(MpplCastExprFields *fields);

MpplEofKind         mppl_eof_kind(const AnyMpplEof *eof);
MpplDeclPartKind    mppl_decl_part_kind(const AnyMpplDeclPart *decl_part);
MpplTypeKind        mppl_type_kind(const AnyMpplType *type);
MpplVarDeclKind     mppl_var_decl_kind(const AnyMpplVarDecl *var_decl);
MpplFmlParamSecKind mppl_fml_param_sec_kind(const AnyMpplFmlParamSec *fml_param_sec);
MpplStmtKind        mppl_stmt_kind(const AnyMpplStmt *stmt);
MpplOutputValueKind mppl_output_value_kind(const AnyMpplOutputValue *output_value);
MpplExprKind        mppl_expr_kind(const AnyMpplExpr *expr);
MpplBindIdentKind   mppl_bind_ident_kind(const AnyMpplBindIdent *ident);

#endif /* MPPL_SYNTAX_H */
