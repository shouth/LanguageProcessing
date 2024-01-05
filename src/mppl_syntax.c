#include <stddef.h>

#include "mppl_syntax.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "utility.h"

static const SyntaxTree *syntax(const void *node)
{
  return (const SyntaxTree *) node;
}

/* program */

MpplToken *mppl_program__program_token(const MpplProgram *program)
{
  return (MpplToken *) syntax_tree_child(syntax(program), 0);
}

MpplToken *mppl_program__name(const MpplProgram *program)
{
  return (MpplToken *) syntax_tree_child(syntax(program), 1);
}

MpplToken *mppl_program__semi_token(const MpplProgram *program)
{
  return (MpplToken *) syntax_tree_child(syntax(program), 2);
}

unsigned long mppl_program__decl_part_count(const MpplProgram *program)
{
  return syntax_tree_child_count(syntax(program)) - 6;
}

AnyMpplDeclPart *mppl_program__decl_part(const MpplProgram *program, unsigned long index)
{
  return (AnyMpplDeclPart *) syntax_tree_child(syntax(program), 3 + index);
}

MpplCompStmt *mppl_program__comp_stmt(const MpplProgram *program)
{
  return (MpplCompStmt *) syntax_tree_child(syntax(program), syntax_tree_child_count(syntax(program)) - 3);
}

MpplToken *mppl_program__dot_token(const MpplProgram *program)
{
  return (MpplToken *) syntax_tree_child(syntax(program), syntax_tree_child_count(syntax(program)) - 2);
}

MpplToken *mppl_program__eof_token(const MpplProgram *program)
{
  return (MpplToken *) syntax_tree_child(syntax(program), syntax_tree_child_count(syntax(program)) - 1);
}

/* declaration part */

MpplDeclPartKind mppl_decl_part__kind(const AnyMpplDeclPart *part)
{
  switch (syntax_tree_kind(syntax(part))) {
  case SYNTAX_VAR_DECL_PART:
    return MPPL_DECL_PART_VAR;
  case SYNTAX_PROC_DECL:
    return MPPL_DECL_PART_PROC;
  default:
    unreachable();
  }
}

/* variable declaration part */

MpplToken *mppl_var_decl_part__var_token(const MpplVarDeclPart *part)
{
  return (MpplToken *) syntax_tree_child(syntax(part), 0);
}

unsigned long mppl_var_decl_part__var_decl_count(const MpplVarDeclPart *part)
{
  return (syntax_tree_child_count(syntax(part)) - 1) / 2;
}

MpplVarDecl *mppl_var_decl_part__var_decl(const MpplVarDeclPart *part, unsigned long index)
{
  return (MpplVarDecl *) syntax_tree_child(syntax(part), 1 + 2 * index);
}

MpplToken *mppl_var_decl_part__semi_token(const MpplVarDeclPart *part, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(part), 2 + 2 * index);
}

/* variable declaration */

unsigned long mppl_var_decl__name_count(const MpplVarDecl *decl)
{
  return (syntax_tree_child_count(syntax(decl)) - 1) / 2;
}

MpplToken *mppl_var_decl__name(const MpplVarDecl *decl, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), index * 2);
}

MpplToken *mppl_var_decl__comma_token(const MpplVarDecl *decl, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), 1 + index * 2);
}

MpplToken *mppl_var_decl__colon_token(const MpplVarDecl *decl)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), syntax_tree_child_count(syntax(decl)) - 2);
}

AnyMpplType *mppl_var_decl__type(const MpplVarDecl *decl)
{
  return (AnyMpplType *) syntax_tree_child(syntax(decl), syntax_tree_child_count(syntax(decl)) - 1);
}

/* procedure declaration */

MpplToken *mppl_proc_decl__procedure_token(const MpplProcDecl *decl)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), 0);
}

MpplToken *mppl_proc_decl__name(const MpplProcDecl *decl)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), 1);
}

MpplFmlParamList *mppl_proc_decl__fml_param_list(const MpplProcDecl *decl)
{
  return (MpplFmlParamList *) syntax_tree_child(syntax(decl), 2);
}

MpplToken *mppl_proc_decl__semi_token_0(const MpplProcDecl *decl)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), 3);
}

MpplVarDeclPart *mppl_proc_decl__var_decl_part(const MpplProcDecl *decl)
{
  return (MpplVarDeclPart *) syntax_tree_child(syntax(decl), 4);
}

MpplCompStmt *mppl_proc_decl__comp_stmt(const MpplProcDecl *decl)
{
  return (MpplCompStmt *) syntax_tree_child(syntax(decl), 5);
}

MpplToken *mppl_proc_decl__semi_token_1(const MpplProcDecl *decl)
{
  return (MpplToken *) syntax_tree_child(syntax(decl), 6);
}

/* formal parameter list */

MpplToken *mppl_fml_param_list__lparen_token(const MpplFmlParamList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 0);
}

unsigned long mppl_fml_param_list__fml_param_sec_count(const MpplFmlParamList *list)
{
  return (syntax_tree_child_count(syntax(list)) - 1) / 2;
}

MpplFmlParamSec *mppl_fml_param_list__fml_param_sec(const MpplFmlParamList *list, unsigned long index)
{
  return (MpplFmlParamSec *) syntax_tree_child(syntax(list), 1 + index);
}

MpplToken *mppl_fml_param_list__semi_token(const MpplFmlParamList *list, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 2 + index);
}

MpplToken *mppl_fml_param_list__rparen_token(const MpplFmlParamList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), syntax_tree_child_count(syntax(list)) - 1);
}

/* formal parameter section */

unsigned long mppl_fml_param_sec__name_count(const MpplFmlParamSec *sec)
{
  return (syntax_tree_child_count(syntax(sec)) - 3) / 2;
}

MpplToken *mppl_fml_param_sec__name(const MpplFmlParamSec *sec, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(sec), index * 2);
}

MpplToken *mppl_fml_param_sec__comma_token(const MpplFmlParamSec *sec, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(sec), 1 + index * 2);
}

MpplToken *mppl_fml_param_sec__colon_token(const MpplFmlParamSec *sec)
{
  return (MpplToken *) syntax_tree_child(syntax(sec), syntax_tree_child_count(syntax(sec)) - 2);
}

AnyMpplType *mppl_fml_param_sec__type(const MpplFmlParamSec *sec)
{
  return (AnyMpplType *) syntax_tree_child(syntax(sec), syntax_tree_child_count(syntax(sec)) - 1);
}

/* statement */

MpplStmtKind mppl_stmt__kind(const AnyMpplStmt *stmt)
{
  switch (syntax_tree_kind(syntax(stmt))) {
  case SYNTAX_ASSIGN_STMT:
    return MPPL_STMT_ASSIGN;
  case SYNTAX_IF_STMT:
    return MPPL_STMT_IF;
  case SYNTAX_WHILE_STMT:
    return MPPL_STMT_WHILE;
  case SYNTAX_BREAK_STMT:
    return MPPL_STMT_BREAK;
  case SYNTAX_CALL_STMT:
    return MPPL_STMT_CALL;
  case SYNTAX_RETURN_STMT:
    return MPPL_STMT_RETURN;
  case SYNTAX_INPUT_STMT:
    return MPPL_STMT_INPUT;
  case SYNTAX_OUTPUT_STMT:
    return MPPL_STMT_OUTPUT;
  case SYNTAX_COMP_STMT:
    return MPPL_STMT_COMP;
  default:
    unreachable();
  }
}

/* assignment statement */

AnyMpplVar *mppl_assign_stmt__var(const MpplAssignStmt *stmt)
{
  return (AnyMpplVar *) syntax_tree_child(syntax(stmt), 0);
}

MpplToken *mppl_assign_stmt__assign_token(const MpplAssignStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 1);
}

AnyMpplExpr *mppl_assign_stmt__expr(const MpplAssignStmt *stmt)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(stmt), 2);
}

/* if statement */

MpplToken *mppl_if_stmt__if_token(const MpplIfStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

AnyMpplExpr *mppl_if_stmt__expr(const MpplIfStmt *stmt)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(stmt), 1);
}

MpplToken *mppl_if_stmt__then_token(const MpplIfStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 2);
}

AnyMpplStmt *mppl_if_stmt__then_stmt(const MpplIfStmt *stmt)
{
  return (AnyMpplStmt *) syntax_tree_child(syntax(stmt), 3);
}

MpplToken *mppl_if_stmt__else_token(const MpplIfStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 4);
}

AnyMpplStmt *mppl_if_stmt__else_stmt(const MpplIfStmt *stmt)
{
  return (AnyMpplStmt *) syntax_tree_child(syntax(stmt), 5);
}

/* while statement */

MpplToken *mppl_while_stmt__while_token(const MpplWhileStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

AnyMpplExpr *mppl_while_stmt__expr(const MpplWhileStmt *stmt)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(stmt), 1);
}

MpplToken *mppl_while_stmt__do_token(const MpplWhileStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 2);
}

AnyMpplStmt *mppl_while_stmt__stmt(const MpplWhileStmt *stmt)
{
  return (AnyMpplStmt *) syntax_tree_child(syntax(stmt), 3);
}

/* break statement */

MpplToken *mppl_break_stmt__break_token(const MpplBreakStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

/* call statement */

MpplToken *mppl_call_stmt__call_token(const MpplCallStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

MpplToken *mppl_call_stmt__name(const MpplCallStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 1);
}

MpplActParamList *mppl_call_stmt__act_param_list(const MpplCallStmt *stmt)
{
  return (MpplActParamList *) syntax_tree_child(syntax(stmt), 2);
}

/* return statement */

MpplToken *mppl_return_stmt__return_token(const MpplReturnStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

/* input statement */

MpplToken *mppl_input_stmt__read_token(const MpplInputStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

MpplInputList *mppl_input_stmt__input_list(const MpplInputStmt *stmt)
{
  return (MpplInputList *) syntax_tree_child(syntax(stmt), 1);
}

/* output statement */

MpplToken *mppl_output_stmt__write_token(const MpplOutputStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

MpplOutputList *mppl_output_stmt__output_list(const MpplOutputStmt *stmt)
{
  return (MpplOutputList *) syntax_tree_child(syntax(stmt), 1);
}

/* compound statement */

MpplToken *mppl_comp_stmt__begin_token(const MpplCompStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 0);
}

unsigned long mppl_comp_stmt__stmt_count(const MpplCompStmt *stmt)
{
  return (syntax_tree_child_count(syntax(stmt)) - 1) / 2;
}

AnyMpplStmt *mppl_comp_stmt__stmt(const MpplCompStmt *stmt, unsigned long index)
{
  return (AnyMpplStmt *) syntax_tree_child(syntax(stmt), 1 + index * 2);
}

MpplToken *mppl_comp_stmt__semi_token(const MpplCompStmt *stmt, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), 2 + index * 2);
}

MpplToken *mppl_comp_stmt__end_token(const MpplCompStmt *stmt)
{
  return (MpplToken *) syntax_tree_child(syntax(stmt), syntax_tree_child_count(syntax(stmt)) - 1);
}

/* actual parameter list */

MpplToken *mppl_act_param_list__lparen_token(const MpplActParamList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 0);
}

unsigned long mppl_act_param_list__expr_count(const MpplActParamList *list)
{
  return (syntax_tree_child_count(syntax(list)) - 1) / 2;
}

AnyMpplExpr *mppl_act_param_list__expr(const MpplActParamList *list, unsigned long index)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(list), 1 + index * 2);
}

MpplToken *mppl_act_param_list__comma_token(const MpplActParamList *list, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 2 + index * 2);
}

MpplToken *mppl_act_param_list__rparen_token(const MpplActParamList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), syntax_tree_child_count(syntax(list)) - 1);
}

/* input list */

MpplToken *mppl_input_list__lparen_token(const MpplInputList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 0);
}

unsigned long mppl_input_list__var_count(const MpplInputList *list)
{
  return (syntax_tree_child_count(syntax(list)) - 1) / 2;
}

AnyMpplVar *mppl_input_list__var(const MpplInputList *list, unsigned long index)
{
  return (AnyMpplVar *) syntax_tree_child(syntax(list), 1 + index * 2);
}

MpplToken *mppl_input_list__comma_token(const MpplInputList *list, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 2 + index * 2);
}

MpplToken *mppl_input_list__rparen_token(const MpplInputList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), syntax_tree_child_count(syntax(list)) - 1);
}

/* expression */

MpplExprKind mppl_expr__kind(const AnyMpplExpr *expr)
{
  switch (syntax_tree_kind(syntax(expr))) {
  case SYNTAX_BINARY_EXPR:
    return MPPL_EXPR_BINARY;
  case SYNTAX_PAREN_EXPR:
    return MPPL_EXPR_PAREN;
  case SYNTAX_NOT_EXPR:
    return MPPL_EXPR_NOT;
  case SYNTAX_CAST_EXPR:
    return MPPL_EXPR_CAST;
  case SYNTAX_ENTIRE_VAR:
  case SYNTAX_INDEXED_VAR:
    return MPPL_EXPR_VAR;
  case SYNTAX_NUMBER_LIT:
  case SYNTAX_STRING_LIT:
  case SYNTAX_TRUE_KW:
  case SYNTAX_FALSE_KW:
    return MPPL_EXPR_LIT;
  default:
    unreachable();
  }
}

/* binary expression */

AnyMpplExpr *mppl_binary_expr__lhs(const MpplBinaryExpr *expr)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(expr), 0);
}

MpplToken *mppl_binary_expr__op_token(const MpplBinaryExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 1);
}

AnyMpplExpr *mppl_binary_expr__rhs(const MpplBinaryExpr *expr)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(expr), 2);
}

/* parenthesized expression */

MpplToken *mppl_paren_expr__lparen_token(const MpplParenExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 0);
}

AnyMpplExpr *mppl_paren_expr__expr(const MpplParenExpr *expr)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(expr), 1);
}

MpplToken *mppl_paren_expr__rparen_token(const MpplParenExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 2);
}

/* not expression */

MpplToken *mppl_not_expr__not_token(const MpplNotExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 0);
}

AnyMpplExpr *mppl_not_expr__expr(const MpplNotExpr *expr)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(expr), 1);
}

/* cast expression */

AnyMpplStdType *mppl_cast_expr__type(const MpplCastExpr *expr)
{
  return (AnyMpplStdType *) syntax_tree_child(syntax(expr), 0);
}

MpplToken *mppl_cast_expr__lparen_token(const MpplCastExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 1);
}

AnyMpplExpr *mppl_cast_expr__expr(const MpplCastExpr *expr)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(expr), 2);
}

MpplToken *mppl_cast_expr__rparen_token(const MpplCastExpr *expr)
{
  return (MpplToken *) syntax_tree_child(syntax(expr), 3);
}

/* variable */

MpplVarKind mppl_var__kind(const AnyMpplVar *var)
{
  switch (syntax_tree_kind(syntax(var))) {
  case SYNTAX_ENTIRE_VAR:
    return MPPL_VAR_ENTIRE;
  case SYNTAX_INDEXED_VAR:
    return MPPL_VAR_INDEXED;
  default:
    unreachable();
  }
}

/* entire variable */

MpplToken *mppl_entire_var__name(const MpplEntireVar *var)
{
  return (MpplToken *) syntax_tree_child(syntax(var), 0);
}

/* indexed variable */

MpplToken *mppl_indexed_var__name(const MpplIndexedVar *var)
{
  return (MpplToken *) syntax_tree_child(syntax(var), 0);
}

MpplToken *mppl_indexed_var__lbracket_token(const MpplIndexedVar *var)
{
  return (MpplToken *) syntax_tree_child(syntax(var), 1);
}

AnyMpplExpr *mppl_indexed_var__expr(const MpplIndexedVar *var)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(var), 2);
}

MpplToken *mppl_indexed_var__rbracket_token(const MpplIndexedVar *var)
{
  return (MpplToken *) syntax_tree_child(syntax(var), 3);
}

/* type */

MpplTypeKind mppl_type__kind(const AnyMpplType *type)
{
  switch (syntax_tree_kind(syntax(type))) {
  case SYNTAX_INTEGER_KW:
  case SYNTAX_BOOLEAN_KW:
  case SYNTAX_CHAR_KW:
    return MPPL_TYPE_STD;
  case SYNTAX_ARRAY_TYPE:
    return MPPL_TYPE_ARRAY;
  default:
    unreachable();
  }
}

/* standard type */

MpplStdTypeKind mppl_std_type__kind(const AnyMpplStdType *type)
{
  switch (syntax_tree_kind(syntax(type))) {
  case SYNTAX_INTEGER_KW:
    return MPPL_STD_TYPE_INTEGER;
  case SYNTAX_BOOLEAN_KW:
    return MPPL_STD_TYPE_BOOLEAN;
  case SYNTAX_CHAR_KW:
    return MPPL_STD_TYPE_CHAR;
  default:
    unreachable();
  }
}

/* array type */

MpplToken *mppl_array_type__array_token(const MpplArrayType *type)
{
  return (MpplToken *) syntax_tree_child(syntax(type), 0);
}

MpplToken *mppl_array_type__lbracket_token(const MpplArrayType *type)
{
  return (MpplToken *) syntax_tree_child(syntax(type), 1);
}

MpplLitNumber *mppl_array_type__size(const MpplArrayType *type)
{
  return (MpplLitNumber *) syntax_tree_child(syntax(type), 2);
}

MpplToken *mppl_array_type__rbracket_token(const MpplArrayType *type)
{
  return (MpplToken *) syntax_tree_child(syntax(type), 3);
}

MpplToken *mppl_array_type__of_token(const MpplArrayType *type)
{
  return (MpplToken *) syntax_tree_child(syntax(type), 4);
}

AnyMpplStdType *mppl_array_type__std_type(const MpplArrayType *type)
{
  return (AnyMpplStdType *) syntax_tree_child(syntax(type), 5);
}

/* output list */

MpplToken *mppl_output_list__lparen_token(const MpplOutputList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 0);
}

unsigned long mppl_output_list__output_value_count(const MpplOutputList *list)
{
  return (syntax_tree_child_count(syntax(list)) - 1) / 2;
}

MpplOutputValue *mppl_output_list__output_value(const MpplOutputList *list, unsigned long index)
{
  return (MpplOutputValue *) syntax_tree_child(syntax(list), 1 + index * 2);
}

MpplToken *mppl_output_list__comma_token(const MpplOutputList *list, unsigned long index)
{
  return (MpplToken *) syntax_tree_child(syntax(list), 2 + index * 2);
}

MpplToken *mppl_output_list__rparen_token(const MpplOutputList *list)
{
  return (MpplToken *) syntax_tree_child(syntax(list), syntax_tree_child_count(syntax(list)) - 1);
}

/* output value */

AnyMpplExpr *mppl_output_value__expr(const MpplOutputValue *value)
{
  return (AnyMpplExpr *) syntax_tree_child(syntax(value), 0);
}

MpplToken *mppl_output_value__colon_token(const MpplOutputValue *value)
{
  return (MpplToken *) syntax_tree_child(syntax(value), 1);
}

MpplLitNumber *mppl_output_value__width(const MpplOutputValue *value)
{
  return (MpplLitNumber *) syntax_tree_child(syntax(value), 2);
}

/* literal */

MpplLitKind mppl_lit__kind(const AnyMpplLit *lit)
{
  switch (syntax_tree_kind(syntax(lit))) {
  case SYNTAX_NUMBER_LIT:
    return MPPL_LIT_NUMBER;
  case SYNTAX_STRING_LIT:
    return MPPL_LIT_STRING;
  case SYNTAX_TRUE_KW:
    return MPPL_LIT_BOOLEAN;
  case SYNTAX_FALSE_KW:
    return MPPL_LIT_BOOLEAN;
  default:
    unreachable();
  }
}

/* free ast node */

void *mppl_free(void *ast)
{
  return syntax_tree_free(ast);
}
