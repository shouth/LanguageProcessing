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
  MPPL_SYNTAX_OUTPUT_VALUE,
  MPPL_SYNTAX_COMP_STMT,
  MPPL_SYNTAX_EXPR_LIST_ELEM,
  MPPL_SYNTAX_EXPR_LIST,
  MPPL_SYNTAX_REF_IDENT,
  MPPL_SYNTAX_ENTIRE_VAR,
  MPPL_SYNTAX_INDEXED_VAR,
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
  MPPL_SYNTAX_BOGUS_IDENT
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
} MpplEofSyntaxKind;

typedef enum {
  MPPL_DECL_PART_SYNTAX_VAR,
  MPPL_DECL_PART_SYNTAX_PROC,
  MPPL_DECL_PART_SYNTAX_BOGUS
} MpplDeclPartSyntaxKind;

typedef enum {
  MPPL_TYPE_SYNTAX_INTEGER,
  MPPL_TYPE_SYNTAX_CHAR,
  MPPL_TYPE_SYNTAX_BOOLEAN,
  MPPL_TYPE_SYNTAX_ARRAY
} MpplTypeSyntaxKind;

typedef enum {
  MPPL_VAR_DECL_SYNTAX,
  MPPL_VAR_DECL_SYNTAX_BOGUS
} MpplVarDeclSyntaxKind;

typedef enum {
  MPPL_FML_PARAM_SEC_SYNTAX,
  MPPL_FML_PARAM_SEC_SYNTAX_BOGUS
} MpplFmlParamSecSyntaxKind;

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
} MpplStmtSyntaxKind;

typedef enum {
  MPPL_VAR_SYNTAX_ENTIRE,
  MPPL_VAR_SYNTAX_INDEXED
} MpplVarSyntaxKind;

typedef enum {
  MPPL_EXPR_SYNTAX_VAR,
  MPPL_EXPR_SYNTAX_UNARY,
  MPPL_EXPR_SYNTAX_BINARY,
  MPPL_EXPR_SYNTAX_PAREN,
  MPPL_EXPR_SYNTAX_CAST,
  MPPL_EXPR_SYNTAX_BOGUS
} MpplExprSyntaxKind;

typedef enum {
  MPPL_OUTPUT_SYNTAX_EXPR,
  MPPL_OUTPUT_SYNTAX_OUTPUT_VALUE
} MpplOutputSyntaxKind;

typedef enum {
  MPPL_OUTPUT_VALUE_SYNTAX,
  MPPL_OUTPUT_VALUE_SYNTAX_BOGUS
} MpplOutputValueSyntaxKind;

typedef enum {
  MPPL_IDENT_SYNTAX,
  MPPL_IDENT_SYNTAX_BOGUS
} MpplIdentSyntaxKind;

#define MpplSyntax     \
  struct {             \
    SyntaxTree syntax; \
  }

typedef MpplSyntax MpplRootSyntax;
typedef MpplSyntax MpplProgramSyntax;
typedef MpplSyntax MpplDeclPartListSyntax;
typedef MpplSyntax MpplVarDeclPartSyntax;
typedef MpplSyntax MpplVarDeclListElemSyntax;
typedef MpplSyntax MpplVarDeclListSyntax;
typedef MpplSyntax MpplVarDeclSyntax;
typedef MpplSyntax MpplArrayTypeSyntax;
typedef MpplSyntax MpplProcDeclPartSyntax;
typedef MpplSyntax MpplProcHeadingSyntax;
typedef MpplSyntax MpplProcBodySyntax;
typedef MpplSyntax MpplProcDeclSyntax;
typedef MpplSyntax MpplFmlParamListElemSyntax;
typedef MpplSyntax MpplFmlParamListSyntax;
typedef MpplSyntax MpplFmlParamsSyntax;
typedef MpplSyntax MpplFmlParamSecSyntax;
typedef MpplSyntax MpplStmtListElemSyntax;
typedef MpplSyntax MpplStmtListSyntax;
typedef MpplSyntax MpplAssignStmtSyntax;
typedef MpplSyntax MpplIfStmtSyntax;
typedef MpplSyntax MpplElseClauseSyntax;
typedef MpplSyntax MpplWhileStmtSyntax;
typedef MpplSyntax MpplBreakStmtSyntax;
typedef MpplSyntax MpplCallStmtSyntax;
typedef MpplSyntax MpplActParamsSyntax;
typedef MpplSyntax MpplReturnStmtSyntax;
typedef MpplSyntax MpplInputStmtSyntax;
typedef MpplSyntax MpplInputsSyntax;
typedef MpplSyntax MpplOutputStmtSyntax;
typedef MpplSyntax MpplOutputListElemSyntax;
typedef MpplSyntax MpplOutputListSyntax;
typedef MpplSyntax MpplOutputsSyntax;
typedef MpplSyntax MpplOutputValueSyntax;
typedef MpplSyntax MpplCompStmtSyntax;
typedef MpplSyntax MpplExprListElemSyntax;
typedef MpplSyntax MpplExprListSyntax;
typedef MpplSyntax MpplEntireVarSyntax;
typedef MpplSyntax MpplIndexedVarSyntax;
typedef MpplSyntax MpplUnaryExprSyntax;
typedef MpplSyntax MpplBinaryExprSyntax;
typedef MpplSyntax MpplParenExprSyntax;
typedef MpplSyntax MpplCastExprSyntax;
typedef MpplSyntax MpplIdentListElemSyntax;
typedef MpplSyntax MpplIdentListSyntax;

typedef MpplSyntax BogusMpplEofSyntax;
typedef MpplSyntax BogusMpplDeclPartSyntax;
typedef MpplSyntax BogusMpplVarDeclSyntax;
typedef MpplSyntax BogusMpplFmlParamSecSyntax;
typedef MpplSyntax BogusMpplStmtSyntax;
typedef MpplSyntax BogusMpplOutputValueSyntax;
typedef MpplSyntax BogusMpplExprSyntax;
typedef MpplSyntax BogusMpplIdentSyntax;

typedef union AnyMpplEofSyntax         AnyMpplEofSyntax;
typedef union AnyMpplDeclPartSyntax    AnyMpplDeclPartSyntax;
typedef union AnyMpplTypeSyntax        AnyMpplTypeSyntax;
typedef union AnyMpplVarDeclSyntax     AnyMpplVarDeclSyntax;
typedef union AnyMpplFmlParamSecSyntax AnyMpplFmlParamSecSyntax;
typedef union AnyMpplStmtSyntax        AnyMpplStmtSyntax;
typedef union AnyMpplOutputSyntax      AnyMpplOutputSyntax;
typedef union AnyMpplOutputValueSyntax AnyMpplOutputValueSyntax;
typedef union AnyMpplVarSyntax         AnyMpplVarSyntax;
typedef union AnyMpplExprSyntax        AnyMpplExprSyntax;
typedef union AnyMpplIdentSyntax       AnyMpplIdentSyntax;

typedef struct MpplRootSyntaxFields             MpplRootSyntaxFields;
typedef struct MpplProgramSyntaxFields          MpplProgramSyntaxFields;
typedef struct MpplDeclPartListSyntaxFields     MpplDeclPartListSyntaxFields;
typedef struct MpplVarDeclPartSyntaxFields      MpplVarDeclPartSyntaxFields;
typedef struct MpplVarDeclListElemSyntaxFields  MpplVarDeclListElemSyntaxFields;
typedef struct MpplVarDeclListSyntaxFields      MpplVarDeclListSyntaxFields;
typedef struct MpplVarDeclSyntaxFields          MpplVarDeclSyntaxFields;
typedef struct MpplArrayTypeSyntaxFields        MpplArrayTypeSyntaxFields;
typedef struct MpplProcDeclPartSyntaxFields     MpplProcDeclPartSyntaxFields;
typedef struct MpplProcHeadingSyntaxFields      MpplProcHeadingSyntaxFields;
typedef struct MpplProcBodySyntaxFields         MpplProcBodySyntaxFields;
typedef struct MpplProcDeclSyntaxFields         MpplProcDeclSyntaxFields;
typedef struct MpplFmlParamListElemSyntaxFields MpplFmlParamListElemSyntaxFields;
typedef struct MpplFmlParamListSyntaxFields     MpplFmlParamListSyntaxFields;
typedef struct MpplFmlParamsSyntaxFields        MpplFmlParamsSyntaxFields;
typedef struct MpplFmlParamSecSyntaxFields      MpplFmlParamSecSyntaxFields;
typedef struct MpplStmtListElemSyntaxFields     MpplStmtListElemSyntaxFields;
typedef struct MpplStmtListSyntaxFields         MpplStmtListSyntaxFields;
typedef struct MpplAssignStmtSyntaxFields       MpplAssignStmtSyntaxFields;
typedef struct MpplIfStmtSyntaxFields           MpplIfStmtSyntaxFields;
typedef struct MpplElseClauseSyntaxFields       MpplElseClauseSyntaxFields;
typedef struct MpplWhileStmtSyntaxFields        MpplWhileStmtSyntaxFields;
typedef struct MpplBreakStmtSyntaxFields        MpplBreakStmtSyntaxFields;
typedef struct MpplCallStmtSyntaxFields         MpplCallStmtSyntaxFields;
typedef struct MpplActParamsSyntaxFields        MpplActParamsSyntaxFields;
typedef struct MpplReturnStmtSyntaxFields       MpplReturnStmtSyntaxFields;
typedef struct MpplInputStmtSyntaxFields        MpplInputStmtSyntaxFields;
typedef struct MpplInputsSyntaxFields           MpplInputsSyntaxFields;
typedef struct MpplOutputStmtSyntaxFields       MpplOutputStmtSyntaxFields;
typedef struct MpplOutputListElemSyntaxFields   MpplOutputListElemSyntaxFields;
typedef struct MpplOutputListSyntaxFields       MpplOutputListSyntaxFields;
typedef struct MpplOutputsSyntaxFields          MpplOutputsSyntaxFields;
typedef struct MpplOutputValueSyntaxFields      MpplOutputValueSyntaxFields;
typedef struct MpplCompStmtSyntaxFields         MpplCompStmtSyntaxFields;
typedef struct MpplExprListElemSyntaxFields     MpplExprListElemSyntaxFields;
typedef struct MpplExprListSyntaxFields         MpplExprListSyntaxFields;
typedef struct MpplEntireVarSyntaxFields        MpplEntireVarSyntaxFields;
typedef struct MpplIndexedVarSyntaxFields       MpplIndexedVarSyntaxFields;
typedef struct MpplUnaryExprSyntaxFields        MpplUnaryExprSyntaxFields;
typedef struct MpplBinaryExprSyntaxFields       MpplBinaryExprSyntaxFields;
typedef struct MpplParenExprSyntaxFields        MpplParenExprSyntaxFields;
typedef struct MpplCastExprSyntaxFields         MpplCastExprSyntaxFields;
typedef struct MpplIdentListElemSyntaxFields    MpplIdentListElemSyntaxFields;
typedef struct MpplIdentListSyntaxFields        MpplIdentListSyntaxFields;

#undef MpplSyntax

union AnyMpplEofSyntax {
  SyntaxToken        eof_token;
  BogusMpplEofSyntax bogus;
};

union AnyMpplDeclPartSyntax {
  MpplVarDeclPartSyntax   var_decl_part;
  MpplProcDeclPartSyntax  proc_decl_part;
  BogusMpplDeclPartSyntax bogus;
};

union AnyMpplTypeSyntax {
  SyntaxToken         integer_type;
  SyntaxToken         char_type;
  SyntaxToken         boolean_type;
  MpplArrayTypeSyntax array_type;
};

union AnyMpplVarDeclSyntax {
  MpplVarDeclSyntax      var_decl;
  BogusMpplVarDeclSyntax bogus;
};

union AnyMpplFmlParamSecSyntax {
  MpplFmlParamSecSyntax      fml_param_sec;
  BogusMpplFmlParamSecSyntax bogus;
};

union AnyMpplStmtSyntax {
  MpplAssignStmtSyntax assign_stmt;
  MpplIfStmtSyntax     if_stmt;
  MpplWhileStmtSyntax  while_stmt;
  MpplBreakStmtSyntax  break_stmt;
  MpplCallStmtSyntax   call_stmt;
  MpplReturnStmtSyntax return_stmt;
  MpplInputStmtSyntax  input_stmt;
  MpplOutputStmtSyntax output_stmt;
  MpplCompStmtSyntax   comp_stmt;
  BogusMpplStmtSyntax  bogus;
};

union AnyMpplVarSyntax {
  MpplEntireVarSyntax  entire_var;
  MpplIndexedVarSyntax indexed_var;
};

union AnyMpplExprSyntax {
  AnyMpplVarSyntax     var_expr;
  MpplUnaryExprSyntax  unary_expr;
  MpplBinaryExprSyntax binary_expr;
  MpplParenExprSyntax  paren_expr;
  MpplCastExprSyntax   cast_expr;
  BogusMpplExprSyntax  bogus;
};

union AnyMpplOutputValueSyntax {
  MpplOutputValueSyntax      output_value;
  BogusMpplOutputValueSyntax bogus;
};

union AnyMpplOutputSyntax {
  AnyMpplExprSyntax        expr;
  AnyMpplOutputValueSyntax output_value;
};

union AnyMpplIdentSyntax {
  SyntaxToken          ident;
  BogusMpplIdentSyntax bogus_ident;
};

struct MpplRootSyntaxFields {
  MpplProgramSyntax *program;
  AnyMpplEofSyntax  *eof;
};

struct MpplProgramSyntaxFields {
  SyntaxToken            *program_kw;
  SyntaxToken            *name;
  SyntaxToken            *semi_token;
  MpplDeclPartListSyntax *decl_part_list;
  MpplCompStmtSyntax     *comp_stmt;
  SyntaxToken            *dot_token;
};

struct MpplDeclPartListSyntaxFields {
  Slice(AnyMpplDeclPartSyntax *) decl_parts;
};

struct MpplVarDeclPartSyntaxFields {
  SyntaxToken           *var_kw;
  MpplVarDeclListSyntax *var_decl_list;
};

struct MpplVarDeclListElemSyntaxFields {
  MpplVarDeclSyntax *var_decl;
  SyntaxToken       *semi_token;
};

struct MpplVarDeclListSyntaxFields {
  Slice(MpplVarDeclListElemSyntax *) var_decl_list_elems;
};

struct MpplVarDeclSyntaxFields {
  MpplIdentListSyntax *ident_list;
  SyntaxToken         *colon_token;
  AnyMpplTypeSyntax   *type;
};

struct MpplArrayTypeSyntaxFields {
  SyntaxToken       *array_kw;
  SyntaxToken       *lbracket_token;
  SyntaxToken       *number_lit;
  SyntaxToken       *rbracket_token;
  SyntaxToken       *of_kw;
  AnyMpplTypeSyntax *type;
};

struct MpplProcDeclPartSyntaxFields {
  MpplProcDeclSyntax *proc_decl;
  SyntaxToken        *semi_token;
};

struct MpplProcHeadingSyntaxFields {
  SyntaxToken         *procedure_kw;
  SyntaxToken         *name;
  MpplFmlParamsSyntax *fml_params;
};

struct MpplProcBodySyntaxFields {
  MpplVarDeclPartSyntax *var_decl_part;
  MpplCompStmtSyntax    *comp_stmt;
};

struct MpplProcDeclSyntaxFields {
  MpplProcHeadingSyntax *proc_heading;
  SyntaxToken           *semi_token;
  MpplProcBodySyntax    *proc_body;
};

struct MpplFmlParamListElemSyntaxFields {
  MpplFmlParamSecSyntax *fml_param_sec;
  SyntaxToken           *semi_token;
};

struct MpplFmlParamListSyntaxFields {
  Slice(MpplFmlParamListElemSyntax *) fml_param_list_elems;
};

struct MpplFmlParamsSyntaxFields {
  SyntaxToken            *lparen_token;
  MpplFmlParamListSyntax *fml_param_list;
  SyntaxToken            *rparen_token;
};

struct MpplFmlParamSecSyntaxFields {
  MpplIdentListSyntax *ident_list;
  SyntaxToken         *colon_token;
  AnyMpplTypeSyntax   *type;
};

struct MpplStmtListElemSyntaxFields {
  AnyMpplStmtSyntax *stmt;
  SyntaxToken       *semi_token;
};

struct MpplStmtListSyntaxFields {
  Slice(MpplStmtListElemSyntax *) stmt_list_elems;
};

struct MpplAssignStmtSyntaxFields {
  AnyMpplExprSyntax *lhs;
  SyntaxToken       *assign_token;
  AnyMpplExprSyntax *rhs;
};

struct MpplIfStmtSyntaxFields {
  SyntaxToken          *if_kw;
  AnyMpplExprSyntax    *cond;
  SyntaxToken          *then_kw;
  AnyMpplStmtSyntax    *then_stmt;
  MpplElseClauseSyntax *else_clause;
};

struct MpplElseClauseSyntaxFields {
  SyntaxToken        *else_kw;
  MpplCompStmtSyntax *else_stmt;
};

struct MpplWhileStmtSyntaxFields {
  SyntaxToken       *while_kw;
  AnyMpplExprSyntax *cond;
  SyntaxToken       *do_kw;
  AnyMpplStmtSyntax *stmt;
};

struct MpplBreakStmtSyntaxFields {
  SyntaxToken *break_kw;
};

struct MpplCallStmtSyntaxFields {
  SyntaxToken         *call_kw;
  SyntaxToken         *name;
  MpplActParamsSyntax *act_params;
};

struct MpplActParamsSyntaxFields {
  SyntaxToken        *lparen_token;
  MpplExprListSyntax *expr_list;
  SyntaxToken        *rparen_token;
};

struct MpplReturnStmtSyntaxFields {
  SyntaxToken *return_kw;
};

struct MpplInputStmtSyntaxFields {
  SyntaxToken      *read_op;
  MpplInputsSyntax *inputs;
};

struct MpplInputsSyntaxFields {
  SyntaxToken        *lparen_token;
  MpplExprListSyntax *expr_list;
  SyntaxToken        *rparen_token;
};

struct MpplOutputStmtSyntaxFields {
  SyntaxToken       *write_op;
  MpplOutputsSyntax *outputs;
};

struct MpplOutputListElemSyntaxFields {
  AnyMpplOutputSyntax *output;
  SyntaxToken         *comma_token;
};

struct MpplOutputListSyntaxFields {
  Slice(MpplOutputListElemSyntax *) output_list_elems;
};

struct MpplOutputsSyntaxFields {
  SyntaxToken          *lparen_token;
  MpplOutputListSyntax *output_list;
  SyntaxToken          *rparen_token;
};

struct MpplOutputValueSyntaxFields {
  AnyMpplExprSyntax *expr;
  SyntaxToken       *colon_token;
  SyntaxToken       *number_lit;
};

struct MpplCompStmtSyntaxFields {
  SyntaxToken        *begin_kw;
  MpplStmtListSyntax *stmt_list;
  SyntaxToken        *end_kw;
};

struct MpplExprListElemSyntaxFields {
  AnyMpplExprSyntax *expr;
  SyntaxToken       *comma_token;
};

struct MpplExprListSyntaxFields {
  Slice(MpplExprListElemSyntax *) expr_list_elems;
};

struct MpplEntireVarSyntaxFields {
  SyntaxToken *name;
};

struct MpplIndexedVarSyntaxFields {
  SyntaxToken       *name;
  SyntaxToken       *lbracket_token;
  AnyMpplExprSyntax *index;
  SyntaxToken       *rbracket_token;
};

struct MpplUnaryExprSyntaxFields {
  SyntaxToken       *op;
  AnyMpplExprSyntax *expr;
};

struct MpplBinaryExprSyntaxFields {
  AnyMpplExprSyntax *lhs;
  SyntaxToken       *op;
  AnyMpplExprSyntax *rhs;
};

struct MpplParenExprSyntaxFields {
  SyntaxToken       *lparen_token;
  AnyMpplExprSyntax *expr;
  SyntaxToken       *rparen_token;
};

struct MpplCastExprSyntaxFields {
  AnyMpplTypeSyntax *type;
  SyntaxToken       *lparen_token;
  AnyMpplExprSyntax *expr;
  SyntaxToken       *rparen_token;
};

struct MpplIdentListElemSyntaxFields {
  SyntaxToken *ident;
  SyntaxToken *comma_token;
};

struct MpplIdentListSyntaxFields {
  Slice(MpplIdentListElemSyntax *) ident_list_elems;
};

MpplRootSyntax             *mppl_root_syntax_alloc(const SyntaxTree *syntax);
MpplProgramSyntax          *mppl_program_syntax_alloc(const SyntaxTree *syntax);
MpplDeclPartListSyntax     *mppl_decl_part_list_syntax_alloc(const SyntaxTree *syntax);
MpplVarDeclPartSyntax      *mppl_var_decl_part_syntax_alloc(const SyntaxTree *syntax);
MpplVarDeclListElemSyntax  *mppl_var_decl_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplVarDeclListSyntax      *mppl_var_decl_list_syntax_alloc(const SyntaxTree *syntax);
MpplVarDeclSyntax          *mppl_var_decl_syntax_alloc(const SyntaxTree *syntax);
MpplArrayTypeSyntax        *mppl_array_type_syntax_alloc(const SyntaxTree *syntax);
MpplProcDeclPartSyntax     *mppl_proc_decl_part_syntax_alloc(const SyntaxTree *syntax);
MpplProcHeadingSyntax      *mppl_proc_heading_syntax_alloc(const SyntaxTree *syntax);
MpplProcBodySyntax         *mppl_proc_body_syntax_alloc(const SyntaxTree *syntax);
MpplProcDeclSyntax         *mppl_proc_decl_syntax_alloc(const SyntaxTree *syntax);
MpplFmlParamListElemSyntax *mppl_fml_param_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplFmlParamListSyntax     *mppl_fml_param_list_syntax_alloc(const SyntaxTree *syntax);
MpplFmlParamsSyntax        *mppl_fml_params_syntax_alloc(const SyntaxTree *syntax);
MpplFmlParamSecSyntax      *mppl_fml_param_sec_syntax_alloc(const SyntaxTree *syntax);
MpplStmtListElemSyntax     *mppl_stmt_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplStmtListSyntax         *mppl_stmt_list_syntax_alloc(const SyntaxTree *syntax);
MpplAssignStmtSyntax       *mppl_assign_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplIfStmtSyntax           *mppl_if_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplElseClauseSyntax       *mppl_else_clause_syntax_alloc(const SyntaxTree *syntax);
MpplWhileStmtSyntax        *mppl_while_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplBreakStmtSyntax        *mppl_break_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplCallStmtSyntax         *mppl_call_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplActParamsSyntax        *mppl_act_params_syntax_alloc(const SyntaxTree *syntax);
MpplReturnStmtSyntax       *mppl_return_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplInputStmtSyntax        *mppl_input_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplInputsSyntax           *mppl_inputs_syntax_alloc(const SyntaxTree *syntax);
MpplOutputStmtSyntax       *mppl_output_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplOutputListElemSyntax   *mppl_output_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplOutputListSyntax       *mppl_output_list_syntax_alloc(const SyntaxTree *syntax);
MpplOutputsSyntax          *mppl_outputs_syntax_alloc(const SyntaxTree *syntax);
MpplOutputValueSyntax      *mppl_output_value_syntax_alloc(const SyntaxTree *syntax);
MpplCompStmtSyntax         *mppl_comp_stmt_syntax_alloc(const SyntaxTree *syntax);
MpplExprListElemSyntax     *mppl_expr_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplExprListSyntax         *mppl_expr_list_syntax_alloc(const SyntaxTree *syntax);
MpplEntireVarSyntax        *mppl_entire_var_syntax_alloc(const SyntaxTree *syntax);
MpplIndexedVarSyntax       *mppl_indexed_var_syntax_alloc(const SyntaxTree *syntax);
MpplUnaryExprSyntax        *mppl_unary_expr_syntax_alloc(const SyntaxTree *syntax);
MpplBinaryExprSyntax       *mppl_binary_expr_syntax_alloc(const SyntaxTree *syntax);
MpplParenExprSyntax        *mppl_paren_expr_syntax_alloc(const SyntaxTree *syntax);
MpplCastExprSyntax         *mppl_cast_expr_syntax_alloc(const SyntaxTree *syntax);
MpplIdentListElemSyntax    *mppl_ident_list_elem_syntax_alloc(const SyntaxTree *syntax);
MpplIdentListSyntax        *mppl_ident_list_syntax_alloc(const SyntaxTree *syntax);

void mppl_root_syntax_free(MpplRootSyntax *root);
void mppl_program_syntax_free(MpplProgramSyntax *program);
void mppl_decl_part_list_syntax_free(MpplDeclPartListSyntax *decl_part_list);
void mppl_var_decl_part_syntax_free(MpplVarDeclPartSyntax *var_decl_part);
void mppl_var_decl_list_elem_syntax_free(MpplVarDeclListElemSyntax *var_decl_list_elem);
void mppl_var_decl_list_syntax_free(MpplVarDeclListSyntax *var_decl_list);
void mppl_var_decl_syntax_free(MpplVarDeclSyntax *var_decl);
void mppl_array_type_syntax_free(MpplArrayTypeSyntax *array_type);
void mppl_proc_decl_part_syntax_free(MpplProcDeclPartSyntax *proc_decl_part);
void mppl_proc_heading_syntax_free(MpplProcHeadingSyntax *proc_heading);
void mppl_proc_body_syntax_free(MpplProcBodySyntax *proc_body);
void mppl_proc_decl_syntax_free(MpplProcDeclSyntax *proc_decl);
void mppl_fml_param_list_elem_syntax_free(MpplFmlParamListElemSyntax *fml_param_list_elem);
void mppl_fml_param_list_syntax_free(MpplFmlParamListSyntax *fml_param_list);
void mppl_fml_params_syntax_free(MpplFmlParamsSyntax *fml_params);
void mppl_fml_param_sec_syntax_free(MpplFmlParamSecSyntax *fml_param_sec);
void mppl_stmt_list_elem_syntax_free(MpplStmtListElemSyntax *stmt_list_elem);
void mppl_stmt_list_syntax_free(MpplStmtListSyntax *stmt_list);
void mppl_assign_stmt_syntax_free(MpplAssignStmtSyntax *assign_stmt);
void mppl_if_stmt_syntax_free(MpplIfStmtSyntax *if_stmt);
void mppl_else_clause_syntax_free(MpplElseClauseSyntax *else_clause);
void mppl_while_stmt_syntax_free(MpplWhileStmtSyntax *while_stmt);
void mppl_break_stmt_syntax_free(MpplBreakStmtSyntax *break_stmt);
void mppl_call_stmt_syntax_free(MpplCallStmtSyntax *call_stmt);
void mppl_act_params_syntax_free(MpplActParamsSyntax *act_params);
void mppl_return_stmt_syntax_free(MpplReturnStmtSyntax *return_stmt);
void mppl_input_stmt_syntax_free(MpplInputStmtSyntax *input_stmt);
void mppl_inputs_syntax_free(MpplInputsSyntax *inputs);
void mppl_output_stmt_syntax_free(MpplOutputStmtSyntax *output_stmt);
void mppl_output_list_elem_syntax_free(MpplOutputListElemSyntax *output_list_elem);
void mppl_output_list_syntax_free(MpplOutputListSyntax *output_list);
void mppl_outputs_syntax_free(MpplOutputsSyntax *outputs);
void mppl_output_value_syntax_free(MpplOutputValueSyntax *output_value);
void mppl_comp_stmt_syntax_free(MpplCompStmtSyntax *comp_stmt);
void mppl_expr_list_elem_syntax_free(MpplExprListElemSyntax *expr_list_elem);
void mppl_expr_list_syntax_free(MpplExprListSyntax *expr_list);
void mppl_entire_var_syntax_free(MpplEntireVarSyntax *entire_var);
void mppl_indexed_var_syntax_free(MpplIndexedVarSyntax *indexed_var);
void mppl_unary_expr_syntax_free(MpplUnaryExprSyntax *unary_expr);
void mppl_binary_expr_syntax_free(MpplBinaryExprSyntax *binary_expr);
void mppl_paren_expr_syntax_free(MpplParenExprSyntax *paren_expr);
void mppl_cast_expr_syntax_free(MpplCastExprSyntax *cast_expr);
void mppl_ident_list_elem_syntax_free(MpplIdentListElemSyntax *ident_list_elem);
void mppl_ident_list_syntax_free(MpplIdentListSyntax *ident_list);

BogusMpplEofSyntax         *bogus_mppl_eof_syntax_alloc(const SyntaxTree *syntax);
BogusMpplDeclPartSyntax    *bogus_mppl_decl_part_syntax_alloc(const SyntaxTree *syntax);
BogusMpplVarDeclSyntax     *bogus_mppl_var_decl_syntax_alloc(const SyntaxTree *syntax);
BogusMpplFmlParamSecSyntax *bogus_mppl_fml_param_sec_syntax_alloc(const SyntaxTree *syntax);
BogusMpplStmtSyntax        *bogus_mppl_stmt_syntax_alloc(const SyntaxTree *syntax);
BogusMpplOutputValueSyntax *bogus_mppl_output_value_syntax_alloc(const SyntaxTree *syntax);
BogusMpplExprSyntax        *bogus_mppl_expr_syntax_alloc(const SyntaxTree *syntax);
BogusMpplIdentSyntax       *bogus_mppl_ident_syntax_alloc(const SyntaxTree *syntax);
BogusMpplEofSyntax         *bogus_mppl_eof_syntax_alloc(const SyntaxTree *syntax);

void bogus_mppl_eof_syntax_free(BogusMpplEofSyntax *bogus_eof);
void bogus_mppl_decl_part_syntax_free(BogusMpplDeclPartSyntax *bogus_decl_part);
void bogus_mppl_var_decl_syntax_free(BogusMpplVarDeclSyntax *bogus_var_decl);
void bogus_mppl_fml_param_sec_syntax_free(BogusMpplFmlParamSecSyntax *bogus_fml_param_sec);
void bogus_mppl_stmt_syntax_free(BogusMpplStmtSyntax *bogus_stmt);
void bogus_mppl_output_value_syntax_free(BogusMpplOutputValueSyntax *bogus_output_value);
void bogus_mppl_expr_syntax_free(BogusMpplExprSyntax *bogus_expr);
void bogus_mppl_ident_syntax_free(BogusMpplIdentSyntax *bogus_ident);
void bogus_mppl_eof_syntax_free(BogusMpplEofSyntax *bogus_eof);

AnyMpplEofSyntax         *any_mppl_eof_syntax_alloc(const SyntaxTree *syntax);
AnyMpplDeclPartSyntax    *any_mppl_decl_part_syntax_alloc(const SyntaxTree *syntax);
AnyMpplTypeSyntax        *any_mppl_type_syntax_alloc(const SyntaxTree *syntax);
AnyMpplVarDeclSyntax     *any_mppl_var_decl_syntax_alloc(const SyntaxTree *syntax);
AnyMpplFmlParamSecSyntax *any_mppl_fml_param_sec_syntax_alloc(const SyntaxTree *syntax);
AnyMpplStmtSyntax        *any_mppl_stmt_syntax_alloc(const SyntaxTree *syntax);
AnyMpplOutputSyntax      *any_mppl_output_syntax_alloc(const SyntaxTree *syntax);
AnyMpplOutputValueSyntax *any_mppl_output_value_syntax_alloc(const SyntaxTree *syntax);
AnyMpplVarSyntax         *any_mppl_var_syntax_alloc(const SyntaxTree *syntax);
AnyMpplExprSyntax        *any_mppl_expr_syntax_alloc(const SyntaxTree *syntax);
AnyMpplIdentSyntax       *any_mppl_ident_syntax_alloc(const SyntaxTree *syntax);

void any_mppl_eof_syntax_free(AnyMpplEofSyntax *any_eof);
void any_mppl_decl_part_syntax_free(AnyMpplDeclPartSyntax *any_decl_part);
void any_mppl_type_syntax_free(AnyMpplTypeSyntax *any_type);
void any_mppl_var_decl_syntax_free(AnyMpplVarDeclSyntax *any_var_decl);
void any_mppl_fml_param_sec_syntax_free(AnyMpplFmlParamSecSyntax *any_fml_param_sec);
void any_mppl_stmt_syntax_free(AnyMpplStmtSyntax *any_stmt);
void any_mppl_output_syntax_free(AnyMpplOutputSyntax *any_output);
void any_mppl_output_value_syntax_free(AnyMpplOutputValueSyntax *any_output_value);
void any_mppl_var_syntax_free(AnyMpplVarSyntax *any_var);
void any_mppl_expr_syntax_free(AnyMpplExprSyntax *any_expr);
void any_mppl_ident_syntax_free(AnyMpplIdentSyntax *any_ident);

MpplEofSyntaxKind         mppl_eof_syntax_kind(const AnyMpplEofSyntax *eof);
MpplDeclPartSyntaxKind    mppl_decl_part_syntax_kind(const AnyMpplDeclPartSyntax *decl_part);
MpplTypeSyntaxKind        mppl_type_syntax_kind(const AnyMpplTypeSyntax *type);
MpplVarDeclSyntaxKind     mppl_var_decl_syntax_kind(const AnyMpplVarDeclSyntax *var_decl);
MpplFmlParamSecSyntaxKind mppl_fml_param_sec_syntax_kind(const AnyMpplFmlParamSecSyntax *fml_param_sec);
MpplStmtSyntaxKind        mppl_stmt_syntax_kind(const AnyMpplStmtSyntax *stmt);
MpplOutputSyntaxKind      mppl_output_syntax_kind(const AnyMpplOutputSyntax *output);
MpplOutputValueSyntaxKind mppl_output_value_syntax_kind(const AnyMpplOutputValueSyntax *output_value);
MpplVarSyntaxKind         mppl_var_syntax_kind(const AnyMpplVarSyntax *var);
MpplExprSyntaxKind        mppl_expr_syntax_kind(const AnyMpplExprSyntax *expr);
MpplIdentSyntaxKind       mppl_ident_syntax_kind(const AnyMpplIdentSyntax *ident);

MpplRootSyntaxFields             mppl_root_syntax_fields_alloc(const MpplRootSyntax *root);
MpplProgramSyntaxFields          mppl_program_syntax_fields_alloc(const MpplProgramSyntax *program);
MpplDeclPartListSyntaxFields     mppl_decl_part_list_syntax_fields_alloc(const MpplDeclPartListSyntax *decl_part_list);
MpplVarDeclPartSyntaxFields      mppl_var_decl_part_syntax_fields_alloc(const MpplVarDeclPartSyntax *var_decl_part);
MpplVarDeclListElemSyntaxFields  mppl_var_decl_list_elem_syntax_fields_alloc(const MpplVarDeclListElemSyntax *var_decl_list_elem);
MpplVarDeclListSyntaxFields      mppl_var_decl_list_syntax_fields_alloc(const MpplVarDeclListSyntax *var_decl_list);
MpplVarDeclSyntaxFields          mppl_var_decl_syntax_fields_alloc(const MpplVarDeclSyntax *var_decl);
MpplArrayTypeSyntaxFields        mppl_array_type_syntax_fields_alloc(const MpplArrayTypeSyntax *array_type);
MpplProcDeclPartSyntaxFields     mppl_proc_decl_part_syntax_fields_alloc(const MpplProcDeclPartSyntax *proc_decl_part);
MpplProcHeadingSyntaxFields      mppl_proc_heading_syntax_fields_alloc(const MpplProcHeadingSyntax *proc_heading);
MpplProcBodySyntaxFields         mppl_proc_body_syntax_fields_alloc(const MpplProcBodySyntax *proc_body);
MpplProcDeclSyntaxFields         mppl_proc_decl_syntax_fields_alloc(const MpplProcDeclSyntax *proc_decl);
MpplFmlParamListElemSyntaxFields mppl_fml_param_list_elem_syntax_fields_alloc(const MpplFmlParamListElemSyntax *fml_param_list_elem);
MpplFmlParamListSyntaxFields     mppl_fml_param_list_syntax_fields_alloc(const MpplFmlParamListSyntax *fml_param_list);
MpplFmlParamsSyntaxFields        mppl_fml_params_syntax_fields_alloc(const MpplFmlParamsSyntax *fml_params);
MpplFmlParamSecSyntaxFields      mppl_fml_param_sec_syntax_fields_alloc(const MpplFmlParamSecSyntax *fml_param_sec);
MpplStmtListElemSyntaxFields     mppl_stmt_list_elem_syntax_fields_alloc(const MpplStmtListElemSyntax *stmt_list_elem);
MpplStmtListSyntaxFields         mppl_stmt_list_syntax_fields_alloc(const MpplStmtListSyntax *stmt_list);
MpplAssignStmtSyntaxFields       mppl_assign_stmt_syntax_fields_alloc(const MpplAssignStmtSyntax *assign_stmt);
MpplIfStmtSyntaxFields           mppl_if_stmt_syntax_fields_alloc(const MpplIfStmtSyntax *if_stmt);
MpplElseClauseSyntaxFields       mppl_else_clause_syntax_fields_alloc(const MpplElseClauseSyntax *else_clause);
MpplWhileStmtSyntaxFields        mppl_while_stmt_syntax_fields_alloc(const MpplWhileStmtSyntax *while_stmt);
MpplBreakStmtSyntaxFields        mppl_break_stmt_syntax_fields_alloc(const MpplBreakStmtSyntax *break_stmt);
MpplCallStmtSyntaxFields         mppl_call_stmt_syntax_fields_alloc(const MpplCallStmtSyntax *call_stmt);
MpplActParamsSyntaxFields        mppl_act_params_syntax_fields_alloc(const MpplActParamsSyntax *act_params);
MpplReturnStmtSyntaxFields       mppl_return_stmt_syntax_fields_alloc(const MpplReturnStmtSyntax *return_stmt);
MpplInputStmtSyntaxFields        mppl_input_stmt_syntax_fields_alloc(const MpplInputStmtSyntax *input_stmt);
MpplInputsSyntaxFields           mppl_inputs_syntax_fields_alloc(const MpplInputsSyntax *inputs);
MpplOutputStmtSyntaxFields       mppl_output_stmt_syntax_fields_alloc(const MpplOutputStmtSyntax *output_stmt);
MpplOutputListElemSyntaxFields   mppl_output_list_elem_syntax_fields_alloc(const MpplOutputListElemSyntax *output_list_elem);
MpplOutputListSyntaxFields       mppl_output_list_syntax_fields_alloc(const MpplOutputListSyntax *output_list);
MpplOutputsSyntaxFields          mppl_outputs_syntax_fields_alloc(const MpplOutputsSyntax *outputs);
MpplOutputValueSyntaxFields      mppl_output_value_syntax_fields_alloc(const MpplOutputValueSyntax *output_value);
MpplCompStmtSyntaxFields         mppl_comp_stmt_syntax_fields_alloc(const MpplCompStmtSyntax *comp_stmt);
MpplExprListElemSyntaxFields     mppl_expr_list_elem_syntax_fields_alloc(const MpplExprListElemSyntax *expr_list_elem);
MpplExprListSyntaxFields         mppl_expr_list_syntax_fields_alloc(const MpplExprListSyntax *expr_list);
MpplEntireVarSyntaxFields        mppl_entire_var_syntax_fields_alloc(const MpplEntireVarSyntax *entire_var);
MpplIndexedVarSyntaxFields       mppl_indexed_var_syntax_fields_alloc(const MpplIndexedVarSyntax *indexed_var);
MpplUnaryExprSyntaxFields        mppl_unary_expr_syntax_fields_alloc(const MpplUnaryExprSyntax *unary_expr);
MpplBinaryExprSyntaxFields       mppl_binary_expr_syntax_fields_alloc(const MpplBinaryExprSyntax *binary_expr);
MpplParenExprSyntaxFields        mppl_paren_expr_syntax_fields_alloc(const MpplParenExprSyntax *paren_expr);
MpplCastExprSyntaxFields         mppl_cast_expr_syntax_fields_alloc(const MpplCastExprSyntax *cast_expr);
MpplIdentListElemSyntaxFields    mppl_ident_list_elem_syntax_fields_alloc(const MpplIdentListElemSyntax *ident_list_elem);
MpplIdentListSyntaxFields        mppl_ident_list_syntax_fields_alloc(const MpplIdentListSyntax *ident_list);

void mppl_root_syntax_fields_free(MpplRootSyntaxFields *fields);
void mppl_program_syntax_fields_free(MpplProgramSyntaxFields *fields);
void mppl_decl_part_list_syntax_fields_free(MpplDeclPartListSyntaxFields *fields);
void mppl_var_decl_part_syntax_fields_free(MpplVarDeclPartSyntaxFields *fields);
void mppl_var_decl_list_elem_syntax_fields_free(MpplVarDeclListElemSyntaxFields *fields);
void mppl_var_decl_list_syntax_fields_free(MpplVarDeclListSyntaxFields *fields);
void mppl_var_decl_syntax_fields_free(MpplVarDeclSyntaxFields *fields);
void mppl_array_type_syntax_fields_free(MpplArrayTypeSyntaxFields *fields);
void mppl_proc_decl_part_syntax_fields_free(MpplProcDeclPartSyntaxFields *fields);
void mppl_proc_heading_syntax_fields_free(MpplProcHeadingSyntaxFields *fields);
void mppl_proc_body_syntax_fields_free(MpplProcBodySyntaxFields *fields);
void mppl_proc_decl_syntax_fields_free(MpplProcDeclSyntaxFields *fields);
void mppl_fml_param_list_elem_syntax_fields_free(MpplFmlParamListElemSyntaxFields *fields);
void mppl_fml_param_list_syntax_fields_free(MpplFmlParamListSyntaxFields *fields);
void mppl_fml_params_syntax_fields_free(MpplFmlParamsSyntaxFields *fields);
void mppl_fml_param_sec_syntax_fields_free(MpplFmlParamSecSyntaxFields *fields);
void mppl_stmt_list_elem_syntax_fields_free(MpplStmtListElemSyntaxFields *fields);
void mppl_stmt_list_syntax_fields_free(MpplStmtListSyntaxFields *fields);
void mppl_assign_stmt_syntax_fields_free(MpplAssignStmtSyntaxFields *fields);
void mppl_if_stmt_syntax_fields_free(MpplIfStmtSyntaxFields *fields);
void mppl_else_clause_syntax_fields_free(MpplElseClauseSyntaxFields *fields);
void mppl_while_stmt_syntax_fields_free(MpplWhileStmtSyntaxFields *fields);
void mppl_break_stmt_syntax_fields_free(MpplBreakStmtSyntaxFields *fields);
void mppl_call_stmt_syntax_fields_free(MpplCallStmtSyntaxFields *fields);
void mppl_act_params_syntax_fields_free(MpplActParamsSyntaxFields *fields);
void mppl_return_stmt_syntax_fields_free(MpplReturnStmtSyntaxFields *fields);
void mppl_input_stmt_syntax_fields_free(MpplInputStmtSyntaxFields *fields);
void mppl_inputs_syntax_fields_free(MpplInputsSyntaxFields *fields);
void mppl_output_stmt_syntax_fields_free(MpplOutputStmtSyntaxFields *fields);
void mppl_output_list_elem_syntax_fields_free(MpplOutputListElemSyntaxFields *fields);
void mppl_output_list_syntax_fields_free(MpplOutputListSyntaxFields *fields);
void mppl_outputs_syntax_fields_free(MpplOutputsSyntaxFields *fields);
void mppl_output_value_syntax_fields_free(MpplOutputValueSyntaxFields *fields);
void mppl_comp_stmt_syntax_fields_free(MpplCompStmtSyntaxFields *fields);
void mppl_expr_list_elem_syntax_fields_free(MpplExprListElemSyntaxFields *fields);
void mppl_expr_list_syntax_fields_free(MpplExprListSyntaxFields *fields);
void mppl_entire_var_syntax_fields_free(MpplEntireVarSyntaxFields *fields);
void mppl_indexed_var_syntax_fields_free(MpplIndexedVarSyntaxFields *fields);
void mppl_unary_expr_syntax_fields_free(MpplUnaryExprSyntaxFields *fields);
void mppl_binary_expr_syntax_fields_free(MpplBinaryExprSyntaxFields *fields);
void mppl_paren_expr_syntax_fields_free(MpplParenExprSyntaxFields *fields);
void mppl_cast_expr_syntax_fields_free(MpplCastExprSyntaxFields *fields);
void mppl_ident_list_elem_syntax_fields_free(MpplIdentListElemSyntaxFields *fields);
void mppl_ident_list_syntax_fields_free(MpplIdentListSyntaxFields *fields);

#endif /* MPPL_SYNTAX_H */
