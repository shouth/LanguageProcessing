#ifndef SYNTAX_H
#define SYNTAX_H

typedef struct MpplProgram MpplProgram;

typedef enum {
  MPPL_DECL_PART_VAR,
  MPPL_DECL_PART_PROC
} MpplDeclPartKind;

typedef struct AnyMpplDeclPart   AnyMpplDeclPart;
typedef struct MpplVarDeclPart   MpplVarDeclPart;
typedef struct MpplVarDecl       MpplVarDecl;
typedef struct MpplProcedureDecl MpplProcDecl;

typedef struct MpplFmlParamList MpplFmlParamList;
typedef struct MpplFmlParamSec  MpplFmlParamSec;

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

typedef struct AnyMpplStmt      AnyMpplStmt;
typedef struct MpplAssignStmt   MpplAssignStmt;
typedef struct MpplIfStmt       MpplIfStmt;
typedef struct MpplWhileStmt    MpplWhileStmt;
typedef struct MpplBreakStmt    MpplBreakStmt;
typedef struct MpplCallStmt     MpplCallStmt;
typedef struct MpplReturnStmt   MpplReturnStmt;
typedef struct MpplInputStmt    MpplInputStmt;
typedef struct MpplOutputStmt   MpplOutputStmt;
typedef struct MpplCompStmt     MpplCompStmt;
typedef struct MpplActParamList MpplActParamList;

typedef enum {
  MPPL_EXPR_BINARY,
  MPPL_EXPR_PAREN,
  MPPL_EXPR_NOT,
  MPPL_EXPR_CAST,
  MPPL_EXPR_VAR,
  MPPL_EXPR_LIT
} MpplExprKind;

typedef struct AnyMpplExpr    AnyMpplExpr;
typedef struct MpplBinaryExpr MpplBinaryExpr;
typedef struct MpplParenExpr  MpplParenExpr;
typedef struct MpplNotExpr    MpplNotExpr;
typedef struct MpplCastExpr   MpplCastExpr;

typedef enum {
  MPPL_VAR_ENTIRE,
  MPPL_VAR_INDEXED
} MpplVarKind;

typedef struct AnyMpplVar     AnyMpplVar;
typedef struct MpplEntireVar  MpplEntireVar;
typedef struct MpplIndexedVar MpplIndexedVar;

typedef enum {
  MPPL_TYPE_STD,
  MPPL_TYPE_ARRAY
} MpplTypeKind;

typedef struct AnyMpplType   AnyMpplType;
typedef struct MpplArrayType MpplArrayType;

typedef enum {
  MPPL_STD_TYPE_INTEGER,
  MPPL_STD_TYPE_BOOLEAN,
  MPPL_STD_TYPE_CHAR
} MpplStdTypeKind;

typedef struct AnyMpplStdType     AnyMpplStdType;
typedef struct MpplStdTypeInteger MpplStdTypeInteger;
typedef struct MpplStdTypeBoolean MpplStdTypeBoolean;
typedef struct MpplStdTypeChar    MpplStdTypeChar;

typedef struct MpplInputList   MpplInputList;
typedef struct MpplOutputList  MpplOutputList;
typedef struct MpplOutputValue MpplOutputValue;

typedef enum {
  MPPL_LIT_NUMBER,
  MPPL_LIT_BOOLEAN,
  MPPL_LIT_STRING
} MpplLitKind;

typedef struct AnyMpplLit     AnyMpplLit;
typedef struct MpplLitNumber  MpplLitNumber;
typedef struct MpplLitBoolean MpplLitBoolean;
typedef struct MpplLitString  MpplLitString;

typedef struct MpplToken MpplToken;

MpplToken       *mppl_program__program_token(const MpplProgram *program);
MpplToken       *mppl_program__name(const MpplProgram *program);
MpplToken       *mppl_program__semi_token(const MpplProgram *program);
unsigned long    mppl_program__decl_part_count(const MpplProgram *program);
AnyMpplDeclPart *mppl_program__decl_part(const MpplProgram *program, unsigned long index);
MpplCompStmt    *mppl_program__comp_stmt(const MpplProgram *program);
MpplToken       *mppl_program__dot_token(const MpplProgram *program);
MpplToken       *mppl_program__eof_token(const MpplProgram *program);

MpplDeclPartKind mppl_decl_part__kind(const AnyMpplDeclPart *part);

MpplToken    *mppl_var_decl_part__var_token(const MpplVarDeclPart *part);
unsigned long mppl_var_decl_part__var_decl_count(const MpplVarDeclPart *part);
MpplVarDecl  *mppl_var_decl_part__var_decl(const MpplVarDeclPart *part, unsigned long index);
MpplToken    *mppl_var_decl_part__semi_token(const MpplVarDeclPart *part, unsigned long index);

unsigned long mppl_var_decl__name_count(const MpplVarDecl *decl);
MpplToken    *mppl_var_decl__name(const MpplVarDecl *decl, unsigned long index);
MpplToken    *mppl_var_decl__comma_token(const MpplVarDecl *decl, unsigned long index);
MpplToken    *mppl_var_decl__colon_token(const MpplVarDecl *decl);
AnyMpplType  *mppl_var_decl__type(const MpplVarDecl *decl);

MpplToken        *mppl_proc_decl__procedure_token(const MpplProcDecl *decl);
MpplToken        *mppl_proc_decl__name(const MpplProcDecl *decl);
MpplFmlParamList *mppl_proc_decl__fml_param_list(const MpplProcDecl *decl);
MpplToken        *mppl_proc_decl__semi_token_0(const MpplProcDecl *decl);
MpplVarDeclPart  *mppl_proc_decl__var_decl_part(const MpplProcDecl *decl);
MpplCompStmt     *mppl_proc_decl__comp_stmt(const MpplProcDecl *decl);
MpplToken        *mppl_proc_decl__semi_token_1(const MpplProcDecl *decl);

MpplToken       *mppl_fml_param_list__lparen_token(const MpplFmlParamList *list);
unsigned long    mppl_fml_param_list__fml_param_sec_count(const MpplFmlParamList *list);
MpplFmlParamSec *mppl_fml_param_list__fml_param_sec(const MpplFmlParamList *list, unsigned long index);
MpplToken       *mppl_fml_param_list__semi_token(const MpplFmlParamList *list, unsigned long index);
MpplToken       *mppl_fml_param_list__rparen_token(const MpplFmlParamList *list);

unsigned long mppl_fml_param_sec__name_count(const MpplFmlParamSec *sec);
MpplToken    *mppl_fml_param_sec__name(const MpplFmlParamSec *sec, unsigned long index);
MpplToken    *mppl_fml_param_sec__comma_token(const MpplFmlParamSec *sec, unsigned long index);
MpplToken    *mppl_fml_param_sec__colon_token(const MpplFmlParamSec *sec);
AnyMpplType  *mppl_fml_param_sec__type(const MpplFmlParamSec *sec);

MpplStmtKind mppl_stmt__kind(const AnyMpplStmt *stmt);

AnyMpplVar  *mppl_assign_stmt__var(const MpplAssignStmt *stmt);
MpplToken   *mppl_assign_stmt__assign_token(const MpplAssignStmt *stmt);
AnyMpplExpr *mppl_assign_stmt__expr(const MpplAssignStmt *stmt);

MpplToken   *mppl_if_stmt__if_token(const MpplIfStmt *stmt);
AnyMpplExpr *mppl_if_stmt__expr(const MpplIfStmt *stmt);
MpplToken   *mppl_if_stmt__then_token(const MpplIfStmt *stmt);
AnyMpplStmt *mppl_if_stmt__then_stmt(const MpplIfStmt *stmt);
MpplToken   *mppl_if_stmt__else_token(const MpplIfStmt *stmt);
AnyMpplStmt *mppl_if_stmt__else_stmt(const MpplIfStmt *stmt);

MpplToken   *mppl_while_stmt__while_token(const MpplWhileStmt *stmt);
AnyMpplExpr *mppl_while_stmt__expr(const MpplWhileStmt *stmt);
MpplToken   *mppl_while_stmt__do_token(const MpplWhileStmt *stmt);
AnyMpplStmt *mppl_while_stmt__stmt(const MpplWhileStmt *stmt);

MpplToken *mppl_break_stmt__break_token(const MpplBreakStmt *stmt);

MpplToken        *mppl_call_stmt__call_token(const MpplCallStmt *stmt);
MpplToken        *mppl_call_stmt__name(const MpplCallStmt *stmt);
MpplActParamList *mppl_call_stmt__act_param_list(const MpplCallStmt *stmt);

MpplToken *mppl_return_stmt__return_token(const MpplReturnStmt *stmt);

MpplToken     *mppl_input_stmt__read_token(const MpplInputStmt *stmt);
MpplInputList *mppl_input_stmt__input_list(const MpplInputStmt *stmt);

MpplToken      *mppl_output_stmt__write_token(const MpplOutputStmt *stmt);
MpplOutputList *mppl_output_stmt__output_list(const MpplOutputStmt *stmt);

MpplToken    *mppl_comp_stmt__begin_token(const MpplCompStmt *stmt);
unsigned long mppl_comp_stmt__stmt_count(const MpplCompStmt *stmt);
AnyMpplStmt  *mppl_comp_stmt__stmt(const MpplCompStmt *stmt, unsigned long index);
MpplToken    *mppl_comp_stmt__semi_token(const MpplCompStmt *stmt, unsigned long index);
MpplToken    *mppl_comp_stmt__end_token(const MpplCompStmt *stmt);

MpplToken    *mppl_act_param_list__lparen_token(const MpplActParamList *list);
unsigned long mppl_act_param_list__expr_count(const MpplActParamList *list);
AnyMpplExpr  *mppl_act_param_list__expr(const MpplActParamList *list, unsigned long index);
MpplToken    *mppl_act_param_list__comma_token(const MpplActParamList *list, unsigned long index);
MpplToken    *mppl_act_param_list__rparen_token(const MpplActParamList *list);

MpplToken    *mppl_input_list__lparen_token(const MpplInputList *list);
unsigned long mppl_input_list__var_count(const MpplInputList *list);
AnyMpplVar   *mppl_input_list__var(const MpplInputList *list, unsigned long index);
MpplToken    *mppl_input_list__comma_token(const MpplInputList *list, unsigned long index);
MpplToken    *mppl_input_list__rparen_token(const MpplInputList *list);

MpplExprKind mppl_expr__kind(const AnyMpplExpr *expr);

AnyMpplExpr *mppl_binary_expr__lhs(const MpplBinaryExpr *expr);
MpplToken   *mppl_binary_expr__op_token(const MpplBinaryExpr *expr);
AnyMpplExpr *mppl_binary_expr__rhs(const MpplBinaryExpr *expr);

MpplToken   *mppl_paren_expr__lparen_token(const MpplParenExpr *expr);
AnyMpplExpr *mppl_paren_expr__expr(const MpplParenExpr *expr);
MpplToken   *mppl_paren_expr__rparen_token(const MpplParenExpr *expr);

MpplToken   *mppl_not_expr__not_token(const MpplNotExpr *expr);
AnyMpplExpr *mppl_not_expr__expr(const MpplNotExpr *expr);

AnyMpplType *mppl_cast_expr__type(const MpplCastExpr *expr);
MpplToken   *mppl_cast_expr__lparen_token(const MpplCastExpr *expr);
AnyMpplExpr *mppl_cast_expr__expr(const MpplCastExpr *expr);
MpplToken   *mppl_cast_expr__rparen_token(const MpplCastExpr *expr);

MpplVarKind mppl_var__kind(const AnyMpplVar *var);

MpplToken *mppl_entire_var__name(const MpplEntireVar *var);

MpplToken   *mppl_indexed_var__name(const MpplIndexedVar *var);
MpplToken   *mppl_indexed_var__lbracket_token(const MpplIndexedVar *var);
AnyMpplExpr *mppl_indexed_var__expr(const MpplIndexedVar *var);
MpplToken   *mppl_indexed_var__rbracket_token(const MpplIndexedVar *var);

MpplTypeKind mppl_type__kind(const AnyMpplType *type);

MpplStdTypeKind mppl_std_type__kind(const AnyMpplStdType *type);

MpplToken      *mppl_array_type__array_token(const MpplArrayType *type);
MpplToken      *mppl_array_type__lbracket_token(const MpplArrayType *type);
MpplLitNumber  *mppl_array_type__size(const MpplArrayType *type);
MpplToken      *mppl_array_type__rbracket_token(const MpplArrayType *type);
MpplToken      *mppl_array_type__of_token(const MpplArrayType *type);
AnyMpplStdType *mppl_array_type__std_type(const MpplArrayType *type);

MpplToken       *mppl_output_list__lparen_token(const MpplOutputList *list);
unsigned long    mppl_output_list__output_value_count(const MpplOutputList *list);
MpplOutputValue *mppl_output_list__output_value(const MpplOutputList *list, unsigned long index);
MpplToken       *mppl_output_list__comma_token(const MpplOutputList *list, unsigned long index);
MpplToken       *mppl_output_list__rparen_token(const MpplOutputList *list);

AnyMpplExpr   *mppl_output_value__expr(const MpplOutputValue *value);
MpplToken     *mppl_output_value__colon_token(const MpplOutputValue *value);
MpplLitNumber *mppl_output_value__width(const MpplOutputValue *value);

MpplLitKind mppl_lit__kind(const AnyMpplLit *lit);

void *mppl_free(void *ast);

#endif
