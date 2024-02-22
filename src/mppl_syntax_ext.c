#include <stdlib.h>

#include "context.h"
#include "context_fwd.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

const Type *mppl_std_type__to_type(const AnyMpplStdType *syntax)
{
  switch (mppl_std_type__kind(syntax)) {
  case MPPL_STD_TYPE_BOOLEAN:
    return ctx_type(TYPE_BOOLEAN);

  case MPPL_STD_TYPE_CHAR:
    return ctx_type(TYPE_CHAR);

  case MPPL_STD_TYPE_INTEGER:
    return ctx_type(TYPE_INTEGER);

  default:
    unreachable();
  }
}

const Type *mppl_type__to_type(const AnyMpplType *syntax, Ctx *ctx)
{
  switch (mppl_type__kind(syntax)) {
  case MPPL_TYPE_STD:
    return mppl_std_type__to_type((const AnyMpplStdType *) syntax);

  case MPPL_TYPE_ARRAY: {
    const MpplArrayType *array_syntax = (const MpplArrayType *) syntax;
    AnyMpplStdType      *elem_syntax  = mppl_array_type__type(array_syntax);
    MpplNumberLit       *size_syntax  = mppl_array_type__size(array_syntax);
    const Type          *base         = mppl_std_type__to_type(elem_syntax);
    long                 size         = mppl_lit_number__to_long(size_syntax);

    mppl_unref(size_syntax);
    mppl_unref(elem_syntax);
    return ctx_array_type(ctx, base, size);
  }

  default:
    unreachable();
  }
}

long mppl_lit_number__to_long(const MpplNumberLit *syntax)
{
  const RawSyntaxToken *token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  return atol(string_data(token->string));
}

char *mppl_lit_string__to_string(const MpplStringLit *syntax)
{
  const RawSyntaxToken *token  = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  char                 *result = xmalloc(string_length(token->string) + 1);
  unsigned long         i, j;
  for (i = 0, j = 0; i < string_length(token->string); ++i, ++j) {
    if (string_data(token->string)[i] == '\'') {
      ++i;
    }
    result[j] = string_data(token->string)[i];
  }
  result[j] = '\0';
  return result;
}

int mppl_lit_boolean__to_int(const MpplBooleanLit *syntax)
{
  const RawSyntaxToken *token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  return token->kind == SYNTAX_TRUE_KW;
}

/* visit ast node */

void mppl_ast_walker__setup(MpplAstWalker *walker)
{
  walker->visit_program          = &mppl_ast__walk_program;
  walker->visit_decl_part        = &mppl_ast__walk_decl_part;
  walker->visit_var_decl_part    = &mppl_ast__walk_var_decl_part;
  walker->visit_var_decl         = &mppl_ast__walk_var_decl;
  walker->visit_proc_decl        = &mppl_ast__walk_proc_decl;
  walker->visit_fml_param_list   = &mppl_ast__walk_fml_param_list;
  walker->visit_fml_param_sec    = &mppl_ast__walk_fml_param_sec;
  walker->visit_stmt             = &mppl_ast__walk_stmt;
  walker->visit_assign_stmt      = &mppl_ast__walk_assign_stmt;
  walker->visit_if_stmt          = &mppl_ast__walk_if_stmt;
  walker->visit_while_stmt       = &mppl_ast__walk_while_stmt;
  walker->visit_break_stmt       = &mppl_ast__walk_break_stmt;
  walker->visit_call_stmt        = &mppl_ast__walk_call_stmt;
  walker->visit_return_stmt      = &mppl_ast__walk_return_stmt;
  walker->visit_input_stmt       = &mppl_ast__walk_input_stmt;
  walker->visit_output_stmt      = &mppl_ast__walk_output_stmt;
  walker->visit_comp_stmt        = &mppl_ast__walk_comp_stmt;
  walker->visit_act_param_list   = &mppl_ast__walk_act_param_list;
  walker->visit_expr             = &mppl_ast__walk_expr;
  walker->visit_binary_expr      = &mppl_ast__walk_binary_expr;
  walker->visit_paren_expr       = &mppl_ast__walk_paren_expr;
  walker->visit_not_expr         = &mppl_ast__walk_not_expr;
  walker->visit_cast_expr        = &mppl_ast__walk_cast_expr;
  walker->visit_var              = &mppl_ast__walk_var;
  walker->visit_entire_var       = &mppl_ast__walk_entire_var;
  walker->visit_indexed_var      = &mppl_ast__walk_indexed_var;
  walker->visit_type             = &mppl_ast__walk_type;
  walker->visit_array_type       = &mppl_ast__walk_array_type;
  walker->visit_std_type         = &mppl_ast__walk_std_type;
  walker->visit_std_type_boolean = &mppl_ast__walk_std_type_boolean;
  walker->visit_std_type_char    = &mppl_ast__walk_std_type_char;
  walker->visit_std_type_integer = &mppl_ast__walk_std_type_integer;
  walker->visit_input_list       = &mppl_ast__walk_input_list;
  walker->visit_output_list      = &mppl_ast__walk_output_list;
  walker->visit_output_value     = &mppl_ast__walk_output_value;
  walker->visit_lit              = &mppl_ast__walk_lit;
  walker->visit_number_lit       = &mppl_ast__walk_number_lit;
  walker->visit_boolean_lit      = &mppl_ast__walk_boolean_lit;
  walker->visit_string_lit       = &mppl_ast__walk_string_lit;
}

#define VISITOR_MpplProgram        visit_program
#define VISITOR_AnyMpplDeclPart    visit_decl_part
#define VISITOR_MpplVarDeclPart    visit_var_decl_part
#define VISITOR_MpplVarDecl        visit_var_decl
#define VISITOR_MpplProcDecl       visit_proc_decl
#define VISITOR_MpplFmlParamList   visit_fml_param_list
#define VISITOR_MpplFmlParamSec    visit_fml_param_sec
#define VISITOR_AnyMpplStmt        visit_stmt
#define VISITOR_MpplAssignStmt     visit_assign_stmt
#define VISITOR_MpplIfStmt         visit_if_stmt
#define VISITOR_MpplWhileStmt      visit_while_stmt
#define VISITOR_MpplBreakStmt      visit_break_stmt
#define VISITOR_MpplCallStmt       visit_call_stmt
#define VISITOR_MpplReturnStmt     visit_return_stmt
#define VISITOR_MpplInputStmt      visit_input_stmt
#define VISITOR_MpplOutputStmt     visit_output_stmt
#define VISITOR_MpplCompStmt       visit_comp_stmt
#define VISITOR_MpplActParamList   visit_act_param_list
#define VISITOR_AnyMpplExpr        visit_expr
#define VISITOR_MpplBinaryExpr     visit_binary_expr
#define VISITOR_MpplParenExpr      visit_paren_expr
#define VISITOR_MpplNotExpr        visit_not_expr
#define VISITOR_MpplCastExpr       visit_cast_expr
#define VISITOR_AnyMpplVar         visit_var
#define VISITOR_MpplEntireVar      visit_entire_var
#define VISITOR_MpplIndexedVar     visit_indexed_var
#define VISITOR_AnyMpplType        visit_type
#define VISITOR_MpplArrayType      visit_array_type
#define VISITOR_AnyMpplStdType     visit_std_type
#define VISITOR_MpplStdTypeBoolean visit_std_type_boolean
#define VISITOR_MpplStdTypeChar    visit_std_type_char
#define VISITOR_MpplStdTypeInteger visit_std_type_integer
#define VISITOR_MpplInputList      visit_input_list
#define VISITOR_MpplOutList        visit_output_list
#define VISITOR_MpplOutValue       visit_output_value
#define VISITOR_AnyMpplLit         visit_lit
#define VISITOR_MpplNumberLit      visit_number_lit
#define VISITOR_MpplBooleanLit     visit_boolean_lit
#define VISITOR_MpplStringLit      visit_string_lit

#define VISIT(walker, type, syntax, data)               \
  if (walker->VISITOR_##type) {                         \
    const type *visit_syntax = syntax;                  \
    walker->VISITOR_##type(walker, visit_syntax, data); \
  }

#define CONSUME(walker, type, syntax, data)    \
  {                                            \
    type *consume_syntax = syntax;             \
    VISIT(walker, type, consume_syntax, data); \
    mppl_unref(consume_syntax);                \
  }

#define IGNORE(walker, syntax, data) \
  (void) walker;                     \
  (void) syntax;                     \
  (void) data;

void mppl_ast_walker__travel(MpplAstWalker *walker, const MpplProgram *syntax, void *data)
{
  VISIT(walker, MpplProgram, syntax, data);
}

void mppl_ast__walk_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
      CONSUME(walker, AnyMpplDeclPart, mppl_program__decl_part(syntax, i), data);
    }
    CONSUME(walker, MpplCompStmt, mppl_program__stmt(syntax), data);
  }
}

void mppl_ast__walk_decl_part(const MpplAstWalker *walker, const AnyMpplDeclPart *syntax, void *data)
{
  if (syntax) {
    switch (mppl_decl_part__kind(syntax)) {
    case MPPL_DECL_PART_VAR:
      VISIT(walker, MpplVarDeclPart, (const MpplVarDeclPart *) syntax, data);
      break;

    case MPPL_DECL_PART_PROC:
      VISIT(walker, MpplProcDecl, (const MpplProcDecl *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_var_decl_part(const MpplAstWalker *walker, const MpplVarDeclPart *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_var_decl_part__var_decl_count(syntax); ++i) {
      CONSUME(walker, MpplVarDecl, mppl_var_decl_part__var_decl(syntax, i), data);
    }
  }
}

void mppl_ast__walk_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplType, mppl_var_decl__type(syntax), data);
  }
}

void mppl_ast__walk_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, MpplFmlParamList, mppl_proc_decl__fml_param_list(syntax), data);
    CONSUME(walker, MpplVarDeclPart, mppl_proc_decl__var_decl_part(syntax), data);
    CONSUME(walker, MpplCompStmt, mppl_proc_decl__comp_stmt(syntax), data);
  }
}

void mppl_ast__walk_fml_param_list(const MpplAstWalker *walker, const MpplFmlParamList *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_fml_param_list__sec_count(syntax); ++i) {
      CONSUME(walker, MpplFmlParamSec, mppl_fml_param_list__sec(syntax, i), data);
    }
  }
}

void mppl_ast__walk_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplType, mppl_fml_param_sec__type(syntax), data);
  }
}

void mppl_ast__walk_stmt(const MpplAstWalker *walker, const AnyMpplStmt *syntax, void *data)
{
  if (syntax) {
    switch (mppl_stmt__kind(syntax)) {
    case MPPL_STMT_ASSIGN:
      VISIT(walker, MpplAssignStmt, (const MpplAssignStmt *) syntax, data);
      break;

    case MPPL_STMT_IF:
      VISIT(walker, MpplIfStmt, (const MpplIfStmt *) syntax, data);
      break;

    case MPPL_STMT_WHILE:
      VISIT(walker, MpplWhileStmt, (const MpplWhileStmt *) syntax, data);
      break;

    case MPPL_STMT_BREAK:
      VISIT(walker, MpplBreakStmt, (const MpplBreakStmt *) syntax, data);
      break;

    case MPPL_STMT_CALL:
      VISIT(walker, MpplCallStmt, (const MpplCallStmt *) syntax, data);
      break;

    case MPPL_STMT_RETURN:
      VISIT(walker, MpplReturnStmt, (const MpplReturnStmt *) syntax, data);
      break;

    case MPPL_STMT_INPUT:
      VISIT(walker, MpplInputStmt, (const MpplInputStmt *) syntax, data);
      break;

    case MPPL_STMT_OUTPUT:
      VISIT(walker, MpplOutputStmt, (const MpplOutputStmt *) syntax, data);
      break;

    case MPPL_STMT_COMP:
      VISIT(walker, MpplCompStmt, (const MpplCompStmt *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_assign_stmt(const MpplAstWalker *walker, const MpplAssignStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplVar, mppl_assign_stmt__lhs(syntax), data);
    CONSUME(walker, AnyMpplExpr, mppl_assign_stmt__rhs(syntax), data);
  }
}

void mppl_ast__walk_if_stmt(const MpplAstWalker *walker, const MpplIfStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_if_stmt__cond(syntax), data);
    CONSUME(walker, AnyMpplStmt, mppl_if_stmt__then_stmt(syntax), data);
    CONSUME(walker, AnyMpplStmt, mppl_if_stmt__else_stmt(syntax), data);
  }
}

void mppl_ast__walk_while_stmt(const MpplAstWalker *walker, const MpplWhileStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_while_stmt__cond(syntax), data);
    CONSUME(walker, AnyMpplStmt, mppl_while_stmt__do_stmt(syntax), data);
  }
}

void mppl_ast__walk_break_stmt(const MpplAstWalker *walker, const MpplBreakStmt *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}

void mppl_ast__walk_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, MpplActParamList, mppl_call_stmt__act_param_list(syntax), data);
  }
}

void mppl_ast__walk_return_stmt(const MpplAstWalker *walker, const MpplReturnStmt *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}

void mppl_ast__walk_input_stmt(const MpplAstWalker *walker, const MpplInputStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, MpplInputList, mppl_input_stmt__input_list(syntax), data);
  }
}

void mppl_ast__walk_output_stmt(const MpplAstWalker *walker, const MpplOutputStmt *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, MpplOutList, mppl_output_stmt__output_list(syntax), data);
  }
}

void mppl_ast__walk_comp_stmt(const MpplAstWalker *walker, const MpplCompStmt *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_comp_stmt__stmt_count(syntax); ++i) {
      CONSUME(walker, AnyMpplStmt, mppl_comp_stmt__stmt(syntax, i), data);
    }
  }
}

void mppl_ast__walk_act_param_list(const MpplAstWalker *walker, const MpplActParamList *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_act_param_list__expr_count(syntax); ++i) {
      CONSUME(walker, AnyMpplExpr, mppl_act_param_list__expr(syntax, i), data);
    }
  }
}

void mppl_ast__walk_expr(const MpplAstWalker *walker, const AnyMpplExpr *syntax, void *data)
{
  if (syntax) {
    switch (mppl_expr__kind(syntax)) {
    case MPPL_EXPR_BINARY:
      VISIT(walker, MpplBinaryExpr, (const MpplBinaryExpr *) syntax, data);
      break;

    case MPPL_EXPR_PAREN:
      VISIT(walker, MpplParenExpr, (const MpplParenExpr *) syntax, data);
      break;

    case MPPL_EXPR_NOT:
      VISIT(walker, MpplNotExpr, (const MpplNotExpr *) syntax, data);
      break;

    case MPPL_EXPR_CAST:
      VISIT(walker, MpplCastExpr, (const MpplCastExpr *) syntax, data);
      break;

    case MPPL_EXPR_VAR:
      VISIT(walker, AnyMpplVar, (const AnyMpplVar *) syntax, data);
      break;

    case MPPL_EXPR_LIT:
      VISIT(walker, AnyMpplLit, (const AnyMpplLit *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_binary_expr(const MpplAstWalker *walker, const MpplBinaryExpr *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_binary_expr__lhs(syntax), data);
    CONSUME(walker, AnyMpplExpr, mppl_binary_expr__rhs(syntax), data);
  }
}

void mppl_ast__walk_paren_expr(const MpplAstWalker *walker, const MpplParenExpr *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_paren_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_not_expr(const MpplAstWalker *walker, const MpplNotExpr *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_not_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_cast_expr(const MpplAstWalker *walker, const MpplCastExpr *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplStdType, mppl_cast_expr__type(syntax), data);
    CONSUME(walker, AnyMpplExpr, mppl_cast_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_var(const MpplAstWalker *walker, const AnyMpplVar *syntax, void *data)
{
  if (syntax) {
    switch (mppl_var__kind(syntax)) {
    case MPPL_VAR_ENTIRE:
      VISIT(walker, MpplEntireVar, (const MpplEntireVar *) syntax, data);
      break;

    case MPPL_VAR_INDEXED:
      VISIT(walker, MpplIndexedVar, (const MpplIndexedVar *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_entire_var(const MpplAstWalker *walker, const MpplEntireVar *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}

void mppl_ast__walk_indexed_var(const MpplAstWalker *walker, const MpplIndexedVar *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplExpr, mppl_indexed_var__expr(syntax), data);
  }
}

void mppl_ast__walk_type(const MpplAstWalker *walker, const AnyMpplType *syntax, void *data)
{
  if (syntax) {
    switch (mppl_type__kind(syntax)) {
    case MPPL_TYPE_ARRAY:
      VISIT(walker, MpplArrayType, (const MpplArrayType *) syntax, data);
      break;

    case MPPL_TYPE_STD:
      VISIT(walker, AnyMpplStdType, (const AnyMpplStdType *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_array_type(const MpplAstWalker *walker, const MpplArrayType *syntax, void *data)
{
  if (syntax) {
    CONSUME(walker, AnyMpplStdType, mppl_array_type__type(syntax), data);
    CONSUME(walker, MpplNumberLit, mppl_array_type__size(syntax), data);
  }
}

void mppl_ast__walk_std_type(const MpplAstWalker *walker, const AnyMpplStdType *type, void *data)
{
  if (type) {
    switch (mppl_std_type__kind(type)) {
    case MPPL_STD_TYPE_BOOLEAN:
      VISIT(walker, MpplStdTypeBoolean, (const MpplStdTypeBoolean *) type, data);
      break;

    case MPPL_STD_TYPE_CHAR:
      VISIT(walker, MpplStdTypeChar, (const MpplStdTypeChar *) type, data);
      break;

    case MPPL_STD_TYPE_INTEGER:
      VISIT(walker, MpplStdTypeInteger, (const MpplStdTypeInteger *) type, data);
      break;
    }
  }
}

void mppl_ast__walk_std_type_boolean(const MpplAstWalker *walker, const MpplStdTypeBoolean *type, void *data)
{
  IGNORE(walker, type, data);
}

void mppl_ast__walk_std_type_char(const MpplAstWalker *walker, const MpplStdTypeChar *type, void *data)
{
  IGNORE(walker, type, data);
}

void mppl_ast__walk_std_type_integer(const MpplAstWalker *walker, const MpplStdTypeInteger *type, void *data)
{
  IGNORE(walker, type, data);
}

void mppl_ast__walk_input_list(const MpplAstWalker *walker, const MpplInputList *list, void *data)
{
  if (list) {
    unsigned long i;
    for (i = 0; i < mppl_input_list__var_count(list); ++i) {
      CONSUME(walker, AnyMpplVar, mppl_input_list__var(list, i), data);
    }
  }
}

void mppl_ast__walk_output_list(const MpplAstWalker *walker, const MpplOutList *list, void *data)
{
  if (list) {
    unsigned long i;
    for (i = 0; i < mppl_out_list__out_value_count(list); ++i) {
      CONSUME(walker, MpplOutValue, mppl_out_list__out_value(list, i), data);
    }
  }
}

void mppl_ast__walk_output_value(const MpplAstWalker *walker, const MpplOutValue *value, void *data)
{
  if (value) {
    CONSUME(walker, AnyMpplExpr, mppl_out_value__expr(value), data);
    CONSUME(walker, MpplNumberLit, mppl_out_value__width(value), data);
  }
}

void mppl_ast__walk_lit(const MpplAstWalker *walker, const AnyMpplLit *syntax, void *data)
{
  if (syntax) {
    switch (mppl_lit__kind(syntax)) {
    case MPPL_LIT_NUMBER:
      VISIT(walker, MpplNumberLit, (const MpplNumberLit *) syntax, data);
      break;

    case MPPL_LIT_BOOLEAN:
      VISIT(walker, MpplBooleanLit, (const MpplBooleanLit *) syntax, data);
      break;

    case MPPL_LIT_STRING:
      VISIT(walker, MpplStringLit, (const MpplStringLit *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_number_lit(const MpplAstWalker *walker, const MpplNumberLit *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}

void mppl_ast__walk_boolean_lit(const MpplAstWalker *walker, const MpplBooleanLit *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}

void mppl_ast__walk_string_lit(const MpplAstWalker *walker, const MpplStringLit *syntax, void *data)
{
  IGNORE(walker, syntax, data);
}
