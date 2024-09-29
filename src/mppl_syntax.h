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

typedef struct MpplSyntax           MpplSyntax;
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

typedef struct AnyMpplDecl     AnyMpplDecl;
typedef struct AnyMpplType     AnyMpplType;
typedef struct AnyMpplStmt     AnyMpplStmt;
typedef struct AnyMpplExpr     AnyMpplExpr;
typedef struct AnyMpplUnaryOp  AnyMpplUnaryOp;
typedef struct AnyMpplBinaryOp AnyMpplBinaryOp;

typedef struct MpplSyntaxFields           MpplSyntaxFields;
typedef struct MpplProgramFields          MpplProgramFields;
typedef struct MpplDeclListFields         MpplDeclListFields;
typedef struct MpplVarDeclPartFields      MpplVarDeclPartFields;
typedef struct MpplVarDeclListElemFields  MpplVarDeclListElemFields;
typedef struct MpplVarDeclListFields      MpplVarDeclListFields;
typedef struct MpplVarDeclFields          MpplVarDeclFields;
typedef struct MpplArrayTypeFields        MpplArrayTypeFields;
typedef struct MpplProcDeclFields         MpplProcDeclFields;
typedef struct MpplFmlParamListElemFields MpplFmlParamListElemFields;
typedef struct MpplFmlParamListFields     MpplFmlParamListFields;
typedef struct MpplFmlParamsFields        MpplFmlParamsFields;
typedef struct MpplFmlParamSecFields      MpplFmlParamSecFields;
typedef struct MpplStmtListElemFields     MpplStmtListElemFields;
typedef struct MpplStmtListFields         MpplStmtListFields;
typedef struct MpplAssignStmtFields       MpplAssignStmtFields;
typedef struct MpplIfStmtFields           MpplIfStmtFields;
typedef struct MpplElseClauseFields       MpplElseClauseFields;
typedef struct MpplWhileStmtFields        MpplWhileStmtFields;
typedef struct MpplBreakStmtFields        MpplBreakStmtFields;
typedef struct MpplCallStmtFields         MpplCallStmtFields;
typedef struct MpplActParamsFields        MpplActParamsFields;
typedef struct MpplReturnStmtFields       MpplReturnStmtFields;
typedef struct MpplInputStmtFields        MpplInputStmtFields;
typedef struct MpplInputsFields           MpplInputsFields;
typedef struct MpplOutputStmtFields       MpplOutputStmtFields;
typedef struct MpplOutputListElemFields   MpplOutputListElemFields;
typedef struct MpplOutputListFields       MpplOutputListFields;
typedef struct MpplOutputsFields          MpplOutputsFields;
typedef struct MpplOutputValueFields      MpplOutputValueFields;
typedef struct MpplCompStmtFields         MpplCompStmtFields;
typedef struct MpplExprListElemFields     MpplExprListElemFields;
typedef struct MpplExprListFields         MpplExprListFields;
typedef struct MpplEntireVarFields        MpplEntireVarFields;
typedef struct MpplIndexedVarFields       MpplIndexedVarFields;
typedef struct MpplUnaryExprFields        MpplUnaryExprFields;
typedef struct MpplBinaryExprFields       MpplBinaryExprFields;
typedef struct MpplParenExprFields        MpplParenExprFields;
typedef struct MpplCastExprFields         MpplCastExprFields;
typedef struct MpplIdentListElemFields    MpplIdentListElemFields;
typedef struct MpplIdentListFields        MpplIdentListFields;

typedef struct MpplSyntaxVisitor MpplSyntaxVisitor;

/* MpplErrorToken */

MpplErrorToken *mppl_error_token_alloc(SyntaxToken *token);
void            mppl_error_token_free(MpplErrorToken *error_token);

/* MpplEndOfFileToken */

MpplEndOfFileToken *mppl_end_of_file_token_alloc(SyntaxToken *token);
void                mppl_end_of_file_token_free(MpplEndOfFileToken *end_of_file_token);

/* MpplIdentToken */

MpplIdentToken *mppl_ident_token_alloc(SyntaxToken *token);
void            mppl_ident_token_free(MpplIdentToken *ident_token);

/* MpplNumberLit */

MpplNumberLit *mppl_number_lit_alloc(SyntaxToken *token);
void           mppl_number_lit_free(MpplNumberLit *number_lit);

/* MpplStringLit */

MpplStringLit *mppl_string_lit_alloc(SyntaxToken *token);
void           mppl_string_lit_free(MpplStringLit *string_lit);

/* MpplPlusToken */

MpplPlusToken *mppl_plus_token_alloc(SyntaxToken *token);
void           mppl_plus_token_free(MpplPlusToken *plus_token);

/* MpplMinusToken */

MpplMinusToken *mppl_minus_token_alloc(SyntaxToken *token);
void            mppl_minus_token_free(MpplMinusToken *minus_token);

/* MpplStarToken */

MpplStarToken *mppl_star_token_alloc(SyntaxToken *token);
void           mppl_star_token_free(MpplStarToken *star_token);

/* MpplEqualToken */

MpplEqualToken *mppl_equal_token_alloc(SyntaxToken *token);
void            mppl_equal_token_free(MpplEqualToken *equal_token);

/* MpplNotEqToken */

MpplNotEqToken *mppl_not_eq_token_alloc(SyntaxToken *token);
void            mppl_not_eq_token_free(MpplNotEqToken *not_eq_token);

/* MpplLessToken */

MpplLessToken *mppl_less_token_alloc(SyntaxToken *token);
void           mppl_less_token_free(MpplLessToken *less_token);

/* MpplLessEqToken */

MpplLessEqToken *mppl_less_eq_token_alloc(SyntaxToken *token);
void             mppl_less_eq_token_free(MpplLessEqToken *less_eq_token);

/* MpplGreaterToken */

MpplGreaterToken *mppl_greater_token_alloc(SyntaxToken *token);
void              mppl_greater_token_free(MpplGreaterToken *greater_token);

/* MpplGreaterEqToken */

MpplGreaterEqToken *mppl_greater_eq_token_alloc(SyntaxToken *token);
void                mppl_greater_eq_token_free(MpplGreaterEqToken *greater_eq_token);

/* MpplLParenToken */

MpplLParenToken *mppl_lparen_token_alloc(SyntaxToken *token);
void             mppl_lparen_token_free(MpplLParenToken *lparen_token);

/* MpplRParenToken */

MpplRParenToken *mppl_rparen_token_alloc(SyntaxToken *token);
void             mppl_rparen_token_free(MpplRParenToken *rparen_token);

/* MpplLBracketToken */

MpplLBracketToken *mppl_lbracket_token_alloc(SyntaxToken *token);
void               mppl_lbracket_token_free(MpplLBracketToken *lbracket_token);

/* MpplRBracketToken */

MpplRBracketToken *mppl_rbracket_token_alloc(SyntaxToken *token);
void               mppl_rbracket_token_free(MpplRBracketToken *rbracket_token);

/* MpplAssignToken */

MpplAssignToken *mppl_assign_token_alloc(SyntaxToken *token);
void             mppl_assign_token_free(MpplAssignToken *assign_token);

/* MpplDotToken */

MpplDotToken *mppl_dot_token_alloc(SyntaxToken *token);
void          mppl_dot_token_free(MpplDotToken *dot_token);

/* MpplCommaToken */

MpplCommaToken *mppl_comma_token_alloc(SyntaxToken *token);
void            mppl_comma_token_free(MpplCommaToken *comma_token);

/* MpplColonToken */

MpplColonToken *mppl_colon_token_alloc(SyntaxToken *token);
void            mppl_colon_token_free(MpplColonToken *colon_token);

/* MpplSemiToken */

MpplSemiToken *mppl_semi_token_alloc(SyntaxToken *token);
void           mppl_semi_token_free(MpplSemiToken *semi_token);

/* MpplProgramKw */

MpplProgramKw *mppl_program_kw_alloc(SyntaxToken *token);
void           mppl_program_kw_free(MpplProgramKw *program_kw);

/* MpplVarKw */

MpplVarKw *mppl_var_kw_alloc(SyntaxToken *token);
void       mppl_var_kw_free(MpplVarKw *var_kw);

/* MpplArrayKw */

MpplArrayKw *mppl_array_kw_alloc(SyntaxToken *token);
void         mppl_array_kw_free(MpplArrayKw *array_kw);

/* MpplOfKw */

MpplOfKw *mppl_of_kw_alloc(SyntaxToken *token);
void      mppl_of_kw_free(MpplOfKw *of_kw);

/* MpplBeginKw */

MpplBeginKw *mppl_begin_kw_alloc(SyntaxToken *token);
void         mppl_begin_kw_free(MpplBeginKw *begin_kw);

/* MpplEndKw */

MpplEndKw *mppl_end_kw_alloc(SyntaxToken *token);
void       mppl_end_kw_free(MpplEndKw *end_kw);

/* MpplIfKw */

MpplIfKw *mppl_if_kw_alloc(SyntaxToken *token);
void      mppl_if_kw_free(MpplIfKw *if_kw);

/* MpplThenKw */

MpplThenKw *mppl_then_kw_alloc(SyntaxToken *token);
void        mppl_then_kw_free(MpplThenKw *then_kw);

/* MpplElseKw */

MpplElseKw *mppl_else_kw_alloc(SyntaxToken *token);
void        mppl_else_kw_free(MpplElseKw *else_kw);

/* MpplProcedureKw */

MpplProcedureKw *mppl_procedure_kw_alloc(SyntaxToken *token);
void             mppl_procedure_kw_free(MpplProcedureKw *procedure_kw);

/* MpplReturnKw */

MpplReturnKw *mppl_return_kw_alloc(SyntaxToken *token);
void          mppl_return_kw_free(MpplReturnKw *return_kw);

/* MpplCallKw */

MpplCallKw *mppl_call_kw_alloc(SyntaxToken *token);
void        mppl_call_kw_free(MpplCallKw *call_kw);

/* MpplWhileKw */

MpplWhileKw *mppl_while_kw_alloc(SyntaxToken *token);
void         mppl_while_kw_free(MpplWhileKw *while_kw);

/* MpplDoKw */

MpplDoKw *mppl_do_kw_alloc(SyntaxToken *token);
void      mppl_do_kw_free(MpplDoKw *do_kw);

/* MpplNotKw */

MpplNotKw *mppl_not_kw_alloc(SyntaxToken *token);
void       mppl_not_kw_free(MpplNotKw *not_kw);

/* MpplOrKw */

MpplOrKw *mppl_or_kw_alloc(SyntaxToken *token);
void      mppl_or_kw_free(MpplOrKw *or_kw);

/* MpplDivKw */

MpplDivKw *mppl_div_kw_alloc(SyntaxToken *token);
void       mppl_div_kw_free(MpplDivKw *div_kw);

/* MpplAndKw */

MpplAndKw *mppl_and_kw_alloc(SyntaxToken *token);
void       mppl_and_kw_free(MpplAndKw *and_kw);

/* MpplCharKw */

MpplCharKw *mppl_char_kw_alloc(SyntaxToken *token);
void        mppl_char_kw_free(MpplCharKw *char_kw);

/* MpplIntegerKw */

MpplIntegerKw *mppl_integer_kw_alloc(SyntaxToken *token);
void           mppl_integer_kw_free(MpplIntegerKw *integer_kw);

/* MpplBooleanKw */

MpplBooleanKw *mppl_boolean_kw_alloc(SyntaxToken *token);
void           mppl_boolean_kw_free(MpplBooleanKw *boolean_kw);

/* MpplReadKw */

MpplReadKw *mppl_read_kw_alloc(SyntaxToken *token);
void        mppl_read_kw_free(MpplReadKw *read_kw);

/* MpplWriteKw */

MpplWriteKw *mppl_write_kw_alloc(SyntaxToken *token);
void         mppl_write_kw_free(MpplWriteKw *write_kw);

/* MpplReadLnKw */

MpplReadLnKw *mppl_readln_kw_alloc(SyntaxToken *token);
void          mppl_readln_kw_free(MpplReadLnKw *readln_kw);

/* MpplWriteLnKw */

MpplWriteLnKw *mppl_writeln_kw_alloc(SyntaxToken *token);
void           mppl_writeln_kw_free(MpplWriteLnKw *writeln_kw);

/* MpplTrueKw */

MpplTrueKw *mppl_true_kw_alloc(SyntaxToken *token);
void        mppl_true_kw_free(MpplTrueKw *true_kw);

/* MpplFalseKw */

MpplFalseKw *mppl_false_kw_alloc(SyntaxToken *token);
void         mppl_false_kw_free(MpplFalseKw *false_kw);

/* MpplBreakKw */

MpplBreakKw *mppl_break_kw_alloc(SyntaxToken *token);
void         mppl_break_kw_free(MpplBreakKw *break_kw);

/* MpplSpaceTrivia */

MpplSpaceTrivia *mppl_space_trivia_alloc(SyntaxToken *token);
void             mppl_space_trivia_free(MpplSpaceTrivia *space_trivia);

/* MpplBracesCommentTrivia */

MpplBracesCommentTrivia *mppl_braces_comment_trivia_alloc(SyntaxToken *token);
void                     mppl_braces_comment_trivia_free(MpplBracesCommentTrivia *braces_comment_trivia);

/* MpplCCommentTrivia */

MpplCCommentTrivia *mppl_c_comment_trivia_alloc(SyntaxToken *token);
void                mppl_c_comment_trivia_free(MpplCCommentTrivia *c_comment_trivia);

/* MpplSyntax */

struct MpplSyntaxFields {
  MpplProgram        *program;
  MpplEndOfFileToken *eof_token;
};

MpplSyntax      *mppl_syntax_alloc(SyntaxTree *tree);
void             mppl_syntax_free(MpplSyntax *syntax);
MpplSyntaxFields mppl_syntax_fields_alloc(MpplSyntax *syntax);
void             mppl_syntax_fields_free(MpplSyntaxFields *fields);
void             mppl_syntax_visit(MpplSyntaxVisitor *visitor, const MpplSyntax *syntax, const MpplSyntaxFields *fields);
void             mppl_syntax_walk(MpplSyntaxVisitor *visitor, const MpplSyntax *syntax);

/* MpplProgram */

struct MpplProgramFields {
  MpplProgramKw  *program_token;
  MpplIdentToken *name;
  MpplSemiToken  *semi_token;
  MpplDeclList   *decl_list;
  MpplCompStmt   *comp_stmt;
  MpplDotToken   *dot_token;
};

MpplProgram      *mppl_program_alloc(SyntaxTree *tree);
void              mppl_program_free(MpplProgram *program);
MpplProgramFields mppl_program_fields_alloc(MpplProgram *program);
void              mppl_program_fields_free(MpplProgramFields *fields);
void              mppl_program_visit(MpplSyntaxVisitor *visitor, const MpplProgram *program, const MpplProgramFields *fields);
void              mppl_program_walk(MpplSyntaxVisitor *visitor, const MpplProgram *program);

/* MpplDeclList */

struct MpplDeclListFields {
  Slice(AnyMpplDecl *) elems;
};

MpplDeclList      *mppl_decl_list_alloc(SyntaxTree *tree);
void               mppl_decl_list_free(MpplDeclList *decl_list);
MpplDeclListFields mppl_decl_list_fields_alloc(MpplDeclList *decl_list);
void               mppl_decl_list_fields_free(MpplDeclListFields *fields);
void               mppl_decl_list_visit(MpplSyntaxVisitor *visitor, const MpplDeclList *decl_list, const MpplDeclListFields *fields);
void               mppl_decl_list_walk(MpplSyntaxVisitor *visitor, const MpplDeclList *decl_list);

/* MpplVarDeclPart */

struct MpplVarDeclPartFields {
  MpplVarKw       *var_token;
  MpplVarDeclList *var_decl_list;
};

MpplVarDeclPart      *mppl_var_decl_part_alloc(SyntaxTree *tree);
void                  mppl_var_decl_part_free(MpplVarDeclPart *var_decl_part);
MpplVarDeclPartFields mppl_var_decl_part_fields_alloc(MpplVarDeclPart *var_decl_part);
void                  mppl_var_decl_part_fields_free(MpplVarDeclPartFields *fields);
void                  mppl_var_decl_part_visit(MpplSyntaxVisitor *visitor, const MpplVarDeclPart *var_decl_part, const MpplVarDeclPartFields *fields);
void                  mppl_var_decl_part_walk(MpplSyntaxVisitor *visitor, const MpplVarDeclPart *var_decl_part);

/* MpplVarDeclListElem */

struct MpplVarDeclListElemFields {
  MpplVarDecl   *var_decl;
  MpplSemiToken *semi_token;
};

MpplVarDeclListElem      *mppl_var_decl_list_elem_alloc(SyntaxTree *tree);
void                      mppl_var_decl_list_elem_free(MpplVarDeclListElem *var_decl_list_elem);
MpplVarDeclListElemFields mppl_var_decl_list_elem_fields_alloc(MpplVarDeclListElem *var_decl_list_elem);
void                      mppl_var_decl_list_elem_fields_free(MpplVarDeclListElemFields *fields);
void                      mppl_var_decl_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplVarDeclListElem *var_decl_list_elem, const MpplVarDeclListElemFields *fields);
void                      mppl_var_decl_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplVarDeclListElem *var_decl_list_elem);

/* MpplVarDeclList */

struct MpplVarDeclListFields {
  Slice(MpplVarDeclListElem *) elems;
};

MpplVarDeclList      *mppl_var_decl_list_alloc(SyntaxTree *tree);
void                  mppl_var_decl_list_free(MpplVarDeclList *var_decl_list);
MpplVarDeclListFields mppl_var_decl_list_fields_alloc(MpplVarDeclList *var_decl_list);
void                  mppl_var_decl_list_fields_free(MpplVarDeclListFields *fields);
void                  mppl_var_decl_list_visit(MpplSyntaxVisitor *visitor, const MpplVarDeclList *var_decl_list, const MpplVarDeclListFields *fields);
void                  mppl_var_decl_list_walk(MpplSyntaxVisitor *visitor, const MpplVarDeclList *var_decl_list);

/* MpplVarDecl */

struct MpplVarDeclFields {
  MpplIdentList  *ident_list;
  MpplColonToken *colon_token;
  AnyMpplType    *type;
};

MpplVarDecl      *mppl_var_decl_alloc(SyntaxTree *tree);
void              mppl_var_decl_free(MpplVarDecl *var_decl);
MpplVarDeclFields mppl_var_decl_fields_alloc(MpplVarDecl *var_decl);
void              mppl_var_decl_fields_free(MpplVarDeclFields *fields);
void              mppl_var_decl_visit(MpplSyntaxVisitor *visitor, const MpplVarDecl *var_decl, const MpplVarDeclFields *fields);
void              mppl_var_decl_walk(MpplSyntaxVisitor *visitor, const MpplVarDecl *var_decl);

/* MpplArrayType */

struct MpplArrayTypeFields {
  MpplArrayKw       *array_token;
  MpplLBracketToken *lbracket_token;
  MpplNumberLit     *number_lit;
  MpplRBracketToken *rbracket_token;
  MpplOfKw          *of_token;
  AnyMpplType       *type;
};

MpplArrayType      *mppl_array_type_alloc(SyntaxTree *tree);
void                mppl_array_type_free(MpplArrayType *array_type);
MpplArrayTypeFields mppl_array_type_fields_alloc(MpplArrayType *array_type);
void                mppl_array_type_fields_free(MpplArrayTypeFields *fields);
void                mppl_array_type_visit(MpplSyntaxVisitor *visitor, const MpplArrayType *array_type, const MpplArrayTypeFields *fields);
void                mppl_array_type_walk(MpplSyntaxVisitor *visitor, const MpplArrayType *array_type);

/* MpplProcDecl */

struct MpplProcDeclFields {
  MpplProcedureKw *proc_token;
  MpplIdentToken  *name;
  MpplFmlParams   *fml_params;
  MpplSemiToken   *first_semi_token;
  MpplVarDeclPart *var_decl_part;
  MpplCompStmt    *comp_stmt;
  MpplSemiToken   *second_semi_token;
};

MpplProcDecl      *mppl_proc_decl_alloc(SyntaxTree *tree);
void               mppl_proc_decl_free(MpplProcDecl *proc_decl);
MpplProcDeclFields mppl_proc_decl_fields_alloc(MpplProcDecl *proc_decl);
void               mppl_proc_decl_fields_free(MpplProcDeclFields *fields);
void               mppl_proc_decl_visit(MpplSyntaxVisitor *visitor, const MpplProcDecl *proc_decl, const MpplProcDeclFields *fields);
void               mppl_proc_decl_walk(MpplSyntaxVisitor *visitor, const MpplProcDecl *proc_decl);

/* MpplFmlParamListElem */

struct MpplFmlParamListElemFields {
  MpplFmlParamSec *fml_param_sec;
  MpplSemiToken   *semi_token;
};

MpplFmlParamListElem      *mppl_fml_param_list_elem_alloc(SyntaxTree *tree);
void                       mppl_fml_param_list_elem_free(MpplFmlParamListElem *fml_param_list_elem);
MpplFmlParamListElemFields mppl_fml_param_list_elem_fields_alloc(MpplFmlParamListElem *fml_param_list_elem);
void                       mppl_fml_param_list_elem_fields_free(MpplFmlParamListElemFields *fields);
void                       mppl_fml_param_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplFmlParamListElem *fml_param_list_elem, const MpplFmlParamListElemFields *fields);
void                       mppl_fml_param_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplFmlParamListElem *fml_param_list_elem);

/* MpplFmlParamList */

struct MpplFmlParamListFields {
  Slice(MpplFmlParamListElem *) elems;
};

MpplFmlParamList      *mppl_fml_param_list_alloc(SyntaxTree *tree);
void                   mppl_fml_param_list_free(MpplFmlParamList *fml_param_list);
MpplFmlParamListFields mppl_fml_param_list_fields_alloc(MpplFmlParamList *fml_param_list);
void                   mppl_fml_param_list_fields_free(MpplFmlParamListFields *fields);
void                   mppl_fml_param_list_visit(MpplSyntaxVisitor *visitor, const MpplFmlParamList *fml_param_list, const MpplFmlParamListFields *fields);
void                   mppl_fml_param_list_walk(MpplSyntaxVisitor *visitor, const MpplFmlParamList *fml_param_list);

/* MpplFmlParams */

struct MpplFmlParamsFields {
  MpplLParenToken  *lparen_token;
  MpplFmlParamList *fml_param_list;
  MpplRParenToken  *rparen_token;
};

MpplFmlParams      *mppl_fml_params_alloc(SyntaxTree *tree);
void                mppl_fml_params_free(MpplFmlParams *fml_params);
MpplFmlParamsFields mppl_fml_params_fields_alloc(MpplFmlParams *fml_params);
void                mppl_fml_params_fields_free(MpplFmlParamsFields *fields);
void                mppl_fml_params_visit(MpplSyntaxVisitor *visitor, const MpplFmlParams *fml_params, const MpplFmlParamsFields *fields);
void                mppl_fml_params_walk(MpplSyntaxVisitor *visitor, const MpplFmlParams *fml_params);

/* MpplFmlParamSec */

struct MpplFmlParamSecFields {
  MpplIdentList  *ident_list;
  MpplColonToken *colon_token;
  AnyMpplType    *type;
};

MpplFmlParamSec      *mppl_fml_param_sec_alloc(SyntaxTree *tree);
void                  mppl_fml_param_sec_free(MpplFmlParamSec *fml_param_sec);
MpplFmlParamSecFields mppl_fml_param_sec_fields_alloc(MpplFmlParamSec *fml_param_sec);
void                  mppl_fml_param_sec_fields_free(MpplFmlParamSecFields *fields);
void                  mppl_fml_param_sec_visit(MpplSyntaxVisitor *visitor, const MpplFmlParamSec *fml_param_sec, const MpplFmlParamSecFields *fields);
void                  mppl_fml_param_sec_walk(MpplSyntaxVisitor *visitor, const MpplFmlParamSec *fml_param_sec);

/* MpplStmtListElem */

struct MpplStmtListElemFields {
  AnyMpplStmt   *stmt;
  MpplSemiToken *semi_token;
};

MpplStmtListElem      *mppl_stmt_list_elem_alloc(SyntaxTree *tree);
void                   mppl_stmt_list_elem_free(MpplStmtListElem *stmt_list_elem);
MpplStmtListElemFields mppl_stmt_list_elem_fields_alloc(MpplStmtListElem *stmt_list_elem);
void                   mppl_stmt_list_elem_fields_free(MpplStmtListElemFields *fields);
void                   mppl_stmt_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplStmtListElem *stmt_list_elem, const MpplStmtListElemFields *fields);
void                   mppl_stmt_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplStmtListElem *stmt_list_elem);

/* MpplStmtList */

struct MpplStmtListFields {
  Slice(MpplStmtListElem *) elems;
};

MpplStmtList      *mppl_stmt_list_alloc(SyntaxTree *tree);
void               mppl_stmt_list_free(MpplStmtList *stmt_list);
MpplStmtListFields mppl_stmt_list_fields_alloc(MpplStmtList *stmt_list);
void               mppl_stmt_list_fields_free(MpplStmtListFields *fields);
void               mppl_stmt_list_visit(MpplSyntaxVisitor *visitor, const MpplStmtList *stmt_list, const MpplStmtListFields *fields);
void               mppl_stmt_list_walk(MpplSyntaxVisitor *visitor, const MpplStmtList *stmt_list);

/* MpplAssignStmt */

struct MpplAssignStmtFields {
  AnyMpplExpr     *lhs;
  MpplAssignToken *assign_token;
  AnyMpplExpr     *rhs;
};

MpplAssignStmt      *mppl_assign_stmt_alloc(SyntaxTree *tree);
void                 mppl_assign_stmt_free(MpplAssignStmt *assign_stmt);
MpplAssignStmtFields mppl_assign_stmt_fields_alloc(MpplAssignStmt *assign_stmt);
void                 mppl_assign_stmt_fields_free(MpplAssignStmtFields *fields);
void                 mppl_assign_stmt_visit(MpplSyntaxVisitor *visitor, const MpplAssignStmt *assign_stmt, const MpplAssignStmtFields *fields);
void                 mppl_assign_stmt_walk(MpplSyntaxVisitor *visitor, const MpplAssignStmt *assign_stmt);

/* MpplIfStmt */

struct MpplIfStmtFields {
  MpplIfKw       *if_token;
  AnyMpplExpr    *cond;
  MpplThenKw     *then_token;
  AnyMpplStmt    *then_stmt;
  MpplElseClause *else_clause;
};

MpplIfStmt      *mppl_if_stmt_alloc(SyntaxTree *tree);
void             mppl_if_stmt_free(MpplIfStmt *if_stmt);
MpplIfStmtFields mppl_if_stmt_fields_alloc(MpplIfStmt *if_stmt);
void             mppl_if_stmt_fields_free(MpplIfStmtFields *fields);
void             mppl_if_stmt_visit(MpplSyntaxVisitor *visitor, const MpplIfStmt *if_stmt, const MpplIfStmtFields *fields);
void             mppl_if_stmt_walk(MpplSyntaxVisitor *visitor, const MpplIfStmt *if_stmt);

/* MpplElseClause */

struct MpplElseClauseFields {
  MpplElseKw  *else_token;
  AnyMpplStmt *stmt;
};

MpplElseClause      *mppl_else_clause_alloc(SyntaxTree *tree);
void                 mppl_else_clause_free(MpplElseClause *else_clause);
MpplElseClauseFields mppl_else_clause_fields_alloc(MpplElseClause *else_clause);
void                 mppl_else_clause_fields_free(MpplElseClauseFields *fields);
void                 mppl_else_clause_visit(MpplSyntaxVisitor *visitor, const MpplElseClause *else_clause, const MpplElseClauseFields *fields);
void                 mppl_else_clause_walk(MpplSyntaxVisitor *visitor, const MpplElseClause *else_clause);

/* MpplWhileStmt */

struct MpplWhileStmtFields {
  MpplWhileKw *while_token;
  AnyMpplExpr *cond;
  MpplDoKw    *do_token;
  AnyMpplStmt *stmt;
};

MpplWhileStmt      *mppl_while_stmt_alloc(SyntaxTree *tree);
void                mppl_while_stmt_free(MpplWhileStmt *while_stmt);
MpplWhileStmtFields mppl_while_stmt_fields_alloc(MpplWhileStmt *while_stmt);
void                mppl_while_stmt_fields_free(MpplWhileStmtFields *fields);
void                mppl_while_stmt_visit(MpplSyntaxVisitor *visitor, const MpplWhileStmt *while_stmt, const MpplWhileStmtFields *fields);
void                mppl_while_stmt_walk(MpplSyntaxVisitor *visitor, const MpplWhileStmt *while_stmt);

/* MpplBreakStmt */

struct MpplBreakStmtFields {
  MpplBreakKw *break_token;
};

MpplBreakStmt      *mppl_break_stmt_alloc(SyntaxTree *tree);
void                mppl_break_stmt_free(MpplBreakStmt *break_stmt);
MpplBreakStmtFields mppl_break_stmt_fields_alloc(MpplBreakStmt *break_stmt);
void                mppl_break_stmt_fields_free(MpplBreakStmtFields *fields);
void                mppl_break_stmt_visit(MpplSyntaxVisitor *visitor, const MpplBreakStmt *break_stmt, const MpplBreakStmtFields *fields);
void                mppl_break_stmt_walk(MpplSyntaxVisitor *visitor, const MpplBreakStmt *break_stmt);

/* MpplCallStmt */

struct MpplCallStmtFields {
  MpplCallKw     *call_token;
  MpplIdentToken *name;
  MpplActParams  *act_params;
};

MpplCallStmt      *mppl_call_stmt_alloc(SyntaxTree *tree);
void               mppl_call_stmt_free(MpplCallStmt *call_stmt);
MpplCallStmtFields mppl_call_stmt_fields_alloc(MpplCallStmt *call_stmt);
void               mppl_call_stmt_fields_free(MpplCallStmtFields *fields);
void               mppl_call_stmt_visit(MpplSyntaxVisitor *visitor, const MpplCallStmt *call_stmt, const MpplCallStmtFields *fields);
void               mppl_call_stmt_walk(MpplSyntaxVisitor *visitor, const MpplCallStmt *call_stmt);

/* MpplActParams */

struct MpplActParamsFields {
  MpplLParenToken *lparen_token;
  MpplExprList    *expr_list;
  MpplRParenToken *rparen_token;
};

MpplActParams      *mppl_act_params_alloc(SyntaxTree *tree);
void                mppl_act_params_free(MpplActParams *act_params);
MpplActParamsFields mppl_act_params_fields_alloc(MpplActParams *act_params);
void                mppl_act_params_fields_free(MpplActParamsFields *fields);
void                mppl_act_params_visit(MpplSyntaxVisitor *visitor, const MpplActParams *act_params, const MpplActParamsFields *fields);
void                mppl_act_params_walk(MpplSyntaxVisitor *visitor, const MpplActParams *act_params);

/* MpplReturnStmt */

struct MpplReturnStmtFields {
  MpplReturnKw *return_token;
};

MpplReturnStmt      *mppl_return_stmt_alloc(SyntaxTree *tree);
void                 mppl_return_stmt_free(MpplReturnStmt *return_stmt);
MpplReturnStmtFields mppl_return_stmt_fields_alloc(MpplReturnStmt *return_stmt);
void                 mppl_return_stmt_fields_free(MpplReturnStmtFields *fields);
void                 mppl_return_stmt_visit(MpplSyntaxVisitor *visitor, const MpplReturnStmt *return_stmt, const MpplReturnStmtFields *fields);
void                 mppl_return_stmt_walk(MpplSyntaxVisitor *visitor, const MpplReturnStmt *return_stmt);

/* MpplInputStmt */

struct MpplInputStmtFields {
  MpplReadKw *read_token;
  MpplInputs *inputs;
};

MpplInputStmt      *mppl_input_stmt_alloc(SyntaxTree *tree);
void                mppl_input_stmt_free(MpplInputStmt *input_stmt);
MpplInputStmtFields mppl_input_stmt_fields_alloc(MpplInputStmt *input_stmt);
void                mppl_input_stmt_fields_free(MpplInputStmtFields *fields);
void                mppl_input_stmt_visit(MpplSyntaxVisitor *visitor, const MpplInputStmt *input_stmt, const MpplInputStmtFields *fields);
void                mppl_input_stmt_walk(MpplSyntaxVisitor *visitor, const MpplInputStmt *input_stmt);

/* MpplInputs */

struct MpplInputsFields {
  MpplLParenToken *lparen_token;
  MpplExprList    *expr_list;
  MpplRParenToken *rparen_token;
};

MpplInputs      *mppl_inputs_alloc(SyntaxTree *tree);
void             mppl_inputs_free(MpplInputs *inputs);
MpplInputsFields mppl_inputs_fields_alloc(MpplInputs *inputs);
void             mppl_inputs_fields_free(MpplInputsFields *fields);
void             mppl_inputs_visit(MpplSyntaxVisitor *visitor, const MpplInputs *inputs, const MpplInputsFields *fields);
void             mppl_inputs_walk(MpplSyntaxVisitor *visitor, const MpplInputs *inputs);

/* MpplOutputStmt */

struct MpplOutputStmtFields {
  MpplWriteKw *write_token;
  MpplOutputs *outputs;
};

MpplOutputStmt      *mppl_output_stmt_alloc(SyntaxTree *tree);
void                 mppl_output_stmt_free(MpplOutputStmt *output_stmt);
MpplOutputStmtFields mppl_output_stmt_fields_alloc(MpplOutputStmt *output_stmt);
void                 mppl_output_stmt_fields_free(MpplOutputStmtFields *fields);
void                 mppl_output_stmt_visit(MpplSyntaxVisitor *visitor, const MpplOutputStmt *output_stmt, const MpplOutputStmtFields *fields);
void                 mppl_output_stmt_walk(MpplSyntaxVisitor *visitor, const MpplOutputStmt *output_stmt);

/* MpplOutputListElem */

struct MpplOutputListElemFields {
  MpplOutputValue *output_value;
  MpplCommaToken  *comma_token;
};

MpplOutputListElem      *mppl_output_list_elem_alloc(SyntaxTree *tree);
void                     mppl_output_list_elem_free(MpplOutputListElem *output_list_elem);
MpplOutputListElemFields mppl_output_list_elem_fields_alloc(MpplOutputListElem *output_list_elem);
void                     mppl_output_list_elem_fields_free(MpplOutputListElemFields *fields);
void                     mppl_output_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplOutputListElem *output_list_elem, const MpplOutputListElemFields *fields);
void                     mppl_output_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplOutputListElem *output_list_elem);

/* MpplOutputList */

struct MpplOutputListFields {
  Slice(MpplOutputListElem *) elems;
};

MpplOutputList      *mppl_output_list_alloc(SyntaxTree *tree);
void                 mppl_output_list_free(MpplOutputList *output_list);
MpplOutputListFields mppl_output_list_fields_alloc(MpplOutputList *output_list);
void                 mppl_output_list_fields_free(MpplOutputListFields *fields);
void                 mppl_output_list_visit(MpplSyntaxVisitor *visitor, const MpplOutputList *output_list, const MpplOutputListFields *fields);
void                 mppl_output_list_walk(MpplSyntaxVisitor *visitor, const MpplOutputList *output_list);

/* MpplOutputs */

struct MpplOutputsFields {
  MpplLParenToken *lparen_token;
  MpplOutputList  *output_list;
  MpplRParenToken *rparen_token;
};

MpplOutputs      *mppl_outputs_alloc(SyntaxTree *tree);
void              mppl_outputs_free(MpplOutputs *outputs);
MpplOutputsFields mppl_outputs_fields_alloc(MpplOutputs *outputs);
void              mppl_outputs_fields_free(MpplOutputsFields *fields);
void              mppl_outputs_visit(MpplSyntaxVisitor *visitor, const MpplOutputs *outputs, const MpplOutputsFields *fields);
void              mppl_outputs_walk(MpplSyntaxVisitor *visitor, const MpplOutputs *outputs);

/* MpplOutputValue */

struct MpplOutputValueFields {
  AnyMpplExpr    *expr;
  MpplColonToken *colon_token;
  MpplNumberLit  *number_lit;
};

MpplOutputValue      *mppl_output_value_alloc(SyntaxTree *tree);
void                  mppl_output_value_free(MpplOutputValue *output_value);
MpplOutputValueFields mppl_output_value_fields_alloc(MpplOutputValue *output_value);
void                  mppl_output_value_fields_free(MpplOutputValueFields *fields);
void                  mppl_output_value_visit(MpplSyntaxVisitor *visitor, const MpplOutputValue *output_value, const MpplOutputValueFields *fields);
void                  mppl_output_value_walk(MpplSyntaxVisitor *visitor, const MpplOutputValue *output_value);

/* MpplCompStmt */

struct MpplCompStmtFields {
  MpplBeginKw  *begin_token;
  MpplStmtList *stmt_list;
  MpplEndKw    *end_token;
};

MpplCompStmt      *mppl_comp_stmt_alloc(SyntaxTree *tree);
void               mppl_comp_stmt_free(MpplCompStmt *comp_stmt);
MpplCompStmtFields mppl_comp_stmt_fields_alloc(MpplCompStmt *comp_stmt);
void               mppl_comp_stmt_fields_free(MpplCompStmtFields *fields);
void               mppl_comp_stmt_visit(MpplSyntaxVisitor *visitor, const MpplCompStmt *comp_stmt, const MpplCompStmtFields *fields);
void               mppl_comp_stmt_walk(MpplSyntaxVisitor *visitor, const MpplCompStmt *comp_stmt);

/* MpplExprListElem */

struct MpplExprListElemFields {
  AnyMpplExpr    *expr;
  MpplCommaToken *comma_token;
};

MpplExprListElem      *mppl_expr_list_elem_alloc(SyntaxTree *tree);
void                   mppl_expr_list_elem_free(MpplExprListElem *expr_list_elem);
MpplExprListElemFields mppl_expr_list_elem_fields_alloc(MpplExprListElem *expr_list_elem);
void                   mppl_expr_list_elem_fields_free(MpplExprListElemFields *fields);
void                   mppl_expr_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplExprListElem *expr_list_elem, const MpplExprListElemFields *fields);
void                   mppl_expr_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplExprListElem *expr_list_elem);

/* MpplExprList */

struct MpplExprListFields {
  Slice(MpplExprListElem *) elems;
};

MpplExprList      *mppl_expr_list_alloc(SyntaxTree *tree);
void               mppl_expr_list_free(MpplExprList *expr_list);
MpplExprListFields mppl_expr_list_fields_alloc(MpplExprList *expr_list);
void               mppl_expr_list_fields_free(MpplExprListFields *fields);
void               mppl_expr_list_visit(MpplSyntaxVisitor *visitor, const MpplExprList *expr_list, const MpplExprListFields *fields);
void               mppl_expr_list_walk(MpplSyntaxVisitor *visitor, const MpplExprList *expr_list);

/* MpplEntireVar */

struct MpplEntireVarFields {
  MpplIdentToken *name;
};

MpplEntireVar      *mppl_entire_var_alloc(SyntaxTree *tree);
void                mppl_entire_var_free(MpplEntireVar *entire_var);
MpplEntireVarFields mppl_entire_var_fields_alloc(MpplEntireVar *entire_var);
void                mppl_entire_var_fields_free(MpplEntireVarFields *fields);
void                mppl_entire_var_visit(MpplSyntaxVisitor *visitor, const MpplEntireVar *entire_var, const MpplEntireVarFields *fields);
void                mppl_entire_var_walk(MpplSyntaxVisitor *visitor, const MpplEntireVar *entire_var);

/* MpplIndexedVar */

struct MpplIndexedVarFields {
  MpplIdentToken    *name;
  MpplLBracketToken *lbracket_token;
  AnyMpplExpr       *index;
  MpplRBracketToken *rbracket_token;
};

MpplIndexedVar      *mppl_indexed_var_alloc(SyntaxTree *tree);
void                 mppl_indexed_var_free(MpplIndexedVar *indexed_var);
MpplIndexedVarFields mppl_indexed_var_fields_alloc(MpplIndexedVar *indexed_var);
void                 mppl_indexed_var_fields_free(MpplIndexedVarFields *fields);
void                 mppl_indexed_var_visit(MpplSyntaxVisitor *visitor, const MpplIndexedVar *indexed_var, const MpplIndexedVarFields *fields);
void                 mppl_indexed_var_walk(MpplSyntaxVisitor *visitor, const MpplIndexedVar *indexed_var);

/* MpplUnaryExpr */

struct MpplUnaryExprFields {
  AnyMpplUnaryOp *op;
  AnyMpplExpr    *expr;
};

MpplUnaryExpr      *mppl_unary_expr_alloc(SyntaxTree *tree);
void                mppl_unary_expr_free(MpplUnaryExpr *unary_expr);
MpplUnaryExprFields mppl_unary_expr_fields_alloc(MpplUnaryExpr *unary_expr);
void                mppl_unary_expr_fields_free(MpplUnaryExprFields *fields);
void                mppl_unary_expr_visit(MpplSyntaxVisitor *visitor, const MpplUnaryExpr *unary_expr, const MpplUnaryExprFields *fields);
void                mppl_unary_expr_walk(MpplSyntaxVisitor *visitor, const MpplUnaryExpr *unary_expr);

/* MpplBinaryExpr */

struct MpplBinaryExprFields {
  AnyMpplExpr     *lhs;
  AnyMpplBinaryOp *op;
  AnyMpplExpr     *rhs;
};

MpplBinaryExpr      *mppl_binary_expr_alloc(SyntaxTree *tree);
void                 mppl_binary_expr_free(MpplBinaryExpr *binary_expr);
MpplBinaryExprFields mppl_binary_expr_fields_alloc(MpplBinaryExpr *binary_expr);
void                 mppl_binary_expr_fields_free(MpplBinaryExprFields *fields);
void                 mppl_binary_expr_visit(MpplSyntaxVisitor *visitor, const MpplBinaryExpr *binary_expr, const MpplBinaryExprFields *fields);
void                 mppl_binary_expr_walk(MpplSyntaxVisitor *visitor, const MpplBinaryExpr *binary_expr);

/* MpplParenExpr */

struct MpplParenExprFields {
  MpplLParenToken *lparen_token;
  AnyMpplExpr     *expr;
  MpplRParenToken *rparen_token;
};

MpplParenExpr      *mppl_paren_expr_alloc(SyntaxTree *tree);
void                mppl_paren_expr_free(MpplParenExpr *paren_expr);
MpplParenExprFields mppl_paren_expr_fields_alloc(MpplParenExpr *paren_expr);
void                mppl_paren_expr_fields_free(MpplParenExprFields *fields);
void                mppl_paren_expr_visit(MpplSyntaxVisitor *visitor, const MpplParenExpr *paren_expr, const MpplParenExprFields *fields);
void                mppl_paren_expr_walk(MpplSyntaxVisitor *visitor, const MpplParenExpr *paren_expr);

/* MpplCastExpr */

struct MpplCastExprFields {
  AnyMpplType     *type;
  MpplLParenToken *lparen_token;
  AnyMpplExpr     *expr;
  MpplRParenToken *rparen_token;
};

MpplCastExpr      *mppl_cast_expr_alloc(SyntaxTree *tree);
void               mppl_cast_expr_free(MpplCastExpr *cast_expr);
MpplCastExprFields mppl_cast_expr_fields_alloc(MpplCastExpr *cast_expr);
void               mppl_cast_expr_fields_free(MpplCastExprFields *fields);
void               mppl_cast_expr_visit(MpplSyntaxVisitor *visitor, const MpplCastExpr *cast_expr, const MpplCastExprFields *fields);
void               mppl_cast_expr_walk(MpplSyntaxVisitor *visitor, const MpplCastExpr *cast_expr);

/* MpplIdentListElem */

struct MpplIdentListElemFields {
  MpplIdentToken *ident;
  MpplCommaToken *comma_token;
};

MpplIdentListElem      *mppl_ident_list_elem_alloc(SyntaxTree *tree);
void                    mppl_ident_list_elem_free(MpplIdentListElem *ident_list_elem);
MpplIdentListElemFields mppl_ident_list_elem_fields_alloc(MpplIdentListElem *ident_list_elem);
void                    mppl_ident_list_elem_fields_free(MpplIdentListElemFields *fields);
void                    mppl_ident_list_elem_visit(MpplSyntaxVisitor *visitor, const MpplIdentListElem *ident_list_elem, const MpplIdentListElemFields *fields);
void                    mppl_ident_list_elem_walk(MpplSyntaxVisitor *visitor, const MpplIdentListElem *ident_list_elem);

/* MpplIdentList */

struct MpplIdentListFields {
  Slice(MpplIdentListElem *) elems;
};

MpplIdentList      *mppl_ident_list_alloc(SyntaxTree *tree);
void                mppl_ident_list_free(MpplIdentList *ident_list);
MpplIdentListFields mppl_ident_list_fields_alloc(MpplIdentList *ident_list);
void                mppl_ident_list_fields_free(MpplIdentListFields *fields);
void                mppl_ident_list_visit(MpplSyntaxVisitor *visitor, const MpplIdentList *ident_list, const MpplIdentListFields *fields);
void                mppl_ident_list_walk(MpplSyntaxVisitor *visitor, const MpplIdentList *ident_list);

/* AnyMpplDecl */

MpplDeclKind any_mppl_decl_kind(const AnyMpplDecl *decl);
void         any_mppl_decl_walk(MpplSyntaxVisitor *visitor, const AnyMpplDecl *decl);

/* AnyMpplType */

MpplTypeKind any_mppl_type_kind(const AnyMpplType *type);
void         any_mppl_type_walk(MpplSyntaxVisitor *visitor, const AnyMpplType *type);

/* AnyMpplStmt */

MpplStmtKind any_mppl_stmt_kind(const AnyMpplStmt *stmt);
void         any_mppl_stmt_walk(MpplSyntaxVisitor *visitor, const AnyMpplStmt *stmt);

/* AnyMpplExpr */

MpplExprKind any_mppl_expr_kind(const AnyMpplExpr *expr);
void         any_mppl_expr_walk(MpplSyntaxVisitor *visitor, const AnyMpplExpr *expr);

/* AnyMpplUnaryOp */

MpplUnaryOpKind any_mppl_unary_op_kind(const AnyMpplUnaryOp *op);

/* AnyMpplBinaryOp */

MpplBinaryOpKind any_mppl_binary_op_kind(const AnyMpplBinaryOp *op);

/* MpplSyntaxVisitor */

struct MpplSyntaxVisitor {
  void *data;
  void (*visit_syntax)(MpplSyntaxVisitor *visitor, const MpplSyntax *syntax, const MpplSyntaxFields *fields);
  void (*visit_program)(MpplSyntaxVisitor *visitor, const MpplProgram *program, const MpplProgramFields *fields);
  void (*visit_decl_list)(MpplSyntaxVisitor *visitor, const MpplDeclList *decl_list, const MpplDeclListFields *fields);
  void (*visit_var_decl_part)(MpplSyntaxVisitor *visitor, const MpplVarDeclPart *var_decl_part, const MpplVarDeclPartFields *fields);
  void (*visit_var_decl_list_elem)(MpplSyntaxVisitor *visitor, const MpplVarDeclListElem *var_decl_list_elem, const MpplVarDeclListElemFields *fields);
  void (*visit_var_decl_list)(MpplSyntaxVisitor *visitor, const MpplVarDeclList *var_decl_list, const MpplVarDeclListFields *fields);
  void (*visit_var_decl)(MpplSyntaxVisitor *visitor, const MpplVarDecl *var_decl, const MpplVarDeclFields *fields);
  void (*visit_array_type)(MpplSyntaxVisitor *visitor, const MpplArrayType *array_type, const MpplArrayTypeFields *fields);
  void (*visit_proc_decl)(MpplSyntaxVisitor *visitor, const MpplProcDecl *proc_decl, const MpplProcDeclFields *fields);
  void (*visit_fml_param_list_elem)(MpplSyntaxVisitor *visitor, const MpplFmlParamListElem *fml_param_list_elem, const MpplFmlParamListElemFields *fields);
  void (*visit_fml_param_list)(MpplSyntaxVisitor *visitor, const MpplFmlParamList *fml_param_list, const MpplFmlParamListFields *fields);
  void (*visit_fml_params)(MpplSyntaxVisitor *visitor, const MpplFmlParams *fml_params, const MpplFmlParamsFields *fields);
  void (*visit_fml_param_sec)(MpplSyntaxVisitor *visitor, const MpplFmlParamSec *fml_param_sec, const MpplFmlParamSecFields *fields);
  void (*visit_stmt_list_elem)(MpplSyntaxVisitor *visitor, const MpplStmtListElem *stmt_list_elem, const MpplStmtListElemFields *fields);
  void (*visit_stmt_list)(MpplSyntaxVisitor *visitor, const MpplStmtList *stmt_list, const MpplStmtListFields *fields);
  void (*visit_assign_stmt)(MpplSyntaxVisitor *visitor, const MpplAssignStmt *assign_stmt, const MpplAssignStmtFields *fields);
  void (*visit_if_stmt)(MpplSyntaxVisitor *visitor, const MpplIfStmt *if_stmt, const MpplIfStmtFields *fields);
  void (*visit_else_clause)(MpplSyntaxVisitor *visitor, const MpplElseClause *else_clause, const MpplElseClauseFields *fields);
  void (*visit_while_stmt)(MpplSyntaxVisitor *visitor, const MpplWhileStmt *while_stmt, const MpplWhileStmtFields *fields);
  void (*visit_break_stmt)(MpplSyntaxVisitor *visitor, const MpplBreakStmt *break_stmt, const MpplBreakStmtFields *fields);
  void (*visit_call_stmt)(MpplSyntaxVisitor *visitor, const MpplCallStmt *call_stmt, const MpplCallStmtFields *fields);
  void (*visit_act_params)(MpplSyntaxVisitor *visitor, const MpplActParams *act_params, const MpplActParamsFields *fields);
  void (*visit_return_stmt)(MpplSyntaxVisitor *visitor, const MpplReturnStmt *return_stmt, const MpplReturnStmtFields *fields);
  void (*visit_input_stmt)(MpplSyntaxVisitor *visitor, const MpplInputStmt *input_stmt, const MpplInputStmtFields *fields);
  void (*visit_inputs)(MpplSyntaxVisitor *visitor, const MpplInputs *inputs, const MpplInputsFields *fields);
  void (*visit_output_stmt)(MpplSyntaxVisitor *visitor, const MpplOutputStmt *output_stmt, const MpplOutputStmtFields *fields);
  void (*visit_output_list_elem)(MpplSyntaxVisitor *visitor, const MpplOutputListElem *output_list_elem, const MpplOutputListElemFields *fields);
  void (*visit_output_list)(MpplSyntaxVisitor *visitor, const MpplOutputList *output_list, const MpplOutputListFields *fields);
  void (*visit_outputs)(MpplSyntaxVisitor *visitor, const MpplOutputs *outputs, const MpplOutputsFields *fields);
  void (*visit_output_value)(MpplSyntaxVisitor *visitor, const MpplOutputValue *output_value, const MpplOutputValueFields *fields);
  void (*visit_comp_stmt)(MpplSyntaxVisitor *visitor, const MpplCompStmt *comp_stmt, const MpplCompStmtFields *fields);
  void (*visit_expr_list_elem)(MpplSyntaxVisitor *visitor, const MpplExprListElem *expr_list_elem, const MpplExprListElemFields *fields);
  void (*visit_expr_list)(MpplSyntaxVisitor *visitor, const MpplExprList *expr_list, const MpplExprListFields *fields);
  void (*visit_entire_var)(MpplSyntaxVisitor *visitor, const MpplEntireVar *entire_var, const MpplEntireVarFields *fields);
  void (*visit_indexed_var)(MpplSyntaxVisitor *visitor, const MpplIndexedVar *indexed_var, const MpplIndexedVarFields *fields);
  void (*visit_unary_expr)(MpplSyntaxVisitor *visitor, const MpplUnaryExpr *unary_expr, const MpplUnaryExprFields *fields);
  void (*visit_binary_expr)(MpplSyntaxVisitor *visitor, const MpplBinaryExpr *binary_expr, const MpplBinaryExprFields *fields);
  void (*visit_paren_expr)(MpplSyntaxVisitor *visitor, const MpplParenExpr *paren_expr, const MpplParenExprFields *fields);
  void (*visit_cast_expr)(MpplSyntaxVisitor *visitor, const MpplCastExpr *cast_expr, const MpplCastExprFields *fields);
  void (*visit_ident_list_elem)(MpplSyntaxVisitor *visitor, const MpplIdentListElem *ident_list_elem, const MpplIdentListElemFields *fields);
  void (*visit_ident_list)(MpplSyntaxVisitor *visitor, const MpplIdentList *ident_list, const MpplIdentListFields *fields);
};

#endif /* MPPL_SYNTAX_H */
