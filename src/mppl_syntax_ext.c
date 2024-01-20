#include <stdlib.h>

#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "string.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "type.h"
#include "utility.h"

Type *mppl_std_type__to_type(const AnyMpplStdType *syntax)
{
  switch (mppl_std_type__kind(syntax)) {
  case MPPL_STD_TYPE_BOOLEAN:
    return type_new(TYPE_BOOLEAN);

  case MPPL_STD_TYPE_CHAR:
    return type_new(TYPE_CHAR);

  case MPPL_STD_TYPE_INTEGER:
    return type_new(TYPE_INTEGER);

  default:
    unreachable();
  }
}

Type *mppl_type__to_type(const AnyMpplType *syntax)
{
  switch (mppl_type__kind(syntax)) {
  case MPPL_TYPE_STD:
    return mppl_std_type__to_type((const AnyMpplStdType *) syntax);

  case MPPL_TYPE_ARRAY: {
    const MpplArrayType *array_syntax = (const MpplArrayType *) syntax;
    AnyMpplStdType      *elem_syntax  = mppl_array_type__type(array_syntax);
    MpplNumberLit       *size_syntax  = mppl_array_type__size(array_syntax);
    Type                *elem_type    = mppl_std_type__to_type(elem_syntax);
    long                 size         = mppl_lit_number__to_long(size_syntax);

    mppl_free(size_syntax);
    mppl_free(elem_syntax);
    return type_new_array(elem_type, size);
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

#define DEFINE(name, type)                                                              \
  static void visit_##name(const MpplAstWalker *walker, const type *syntax, void *data) \
  {                                                                                     \
    if (walker->visit_##name && syntax) {                                               \
      walker->visit_##name(walker, syntax, data);                                       \
    }                                                                                   \
  }                                                                                     \
  static void consume_##name(const MpplAstWalker *walker, type *syntax, void *data)     \
  {                                                                                     \
    visit_##name(walker, syntax, data);                                                 \
    mppl_free(syntax);                                                                  \
  }

DEFINE(program, MpplProgram)
DEFINE(decl_part, AnyMpplDeclPart)
DEFINE(var_decl_part, MpplVarDeclPart)
DEFINE(var_decl, MpplVarDecl)
DEFINE(proc_decl, MpplProcDecl)
DEFINE(fml_param_list, MpplFmlParamList)
DEFINE(fml_param_sec, MpplFmlParamSec)
DEFINE(stmt, AnyMpplStmt)
DEFINE(assign_stmt, MpplAssignStmt)
DEFINE(if_stmt, MpplIfStmt)
DEFINE(while_stmt, MpplWhileStmt)
DEFINE(break_stmt, MpplBreakStmt)
DEFINE(call_stmt, MpplCallStmt)
DEFINE(return_stmt, MpplReturnStmt)
DEFINE(input_stmt, MpplInputStmt)
DEFINE(output_stmt, MpplOutputStmt)
DEFINE(comp_stmt, MpplCompStmt)
DEFINE(act_param_list, MpplActParamList)
DEFINE(expr, AnyMpplExpr)
DEFINE(binary_expr, MpplBinaryExpr)
DEFINE(paren_expr, MpplParenExpr)
DEFINE(not_expr, MpplNotExpr)
DEFINE(cast_expr, MpplCastExpr)
DEFINE(var, AnyMpplVar)
DEFINE(entire_var, MpplEntireVar)
DEFINE(indexed_var, MpplIndexedVar)
DEFINE(type, AnyMpplType)
DEFINE(array_type, MpplArrayType)
DEFINE(std_type, AnyMpplStdType)
DEFINE(std_type_integer, MpplStdTypeInteger)
DEFINE(std_type_boolean, MpplStdTypeBoolean)
DEFINE(std_type_char, MpplStdTypeChar)
DEFINE(input_list, MpplInputList)
DEFINE(output_list, MpplOutList)
DEFINE(output_value, MpplOutValue)
DEFINE(lit, AnyMpplLit)
DEFINE(number_lit, MpplNumberLit)
DEFINE(boolean_lit, MpplBooleanLit)
DEFINE(string_lit, MpplStringLit)

#undef DEFINE

void mppl_ast_walker__travel(MpplAstWalker *walker, const MpplProgram *syntax, void *data)
{
  visit_program(walker, syntax, data);
}

void mppl_ast__walk_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
      consume_decl_part(walker, mppl_program__decl_part(syntax, i), data);
    }
    consume_comp_stmt(walker, mppl_program__stmt(syntax), data);
  }
}

void mppl_ast__walk_decl_part(const MpplAstWalker *walker, const AnyMpplDeclPart *syntax, void *data)
{
  if (syntax) {
    switch (mppl_decl_part__kind(syntax)) {
    case MPPL_DECL_PART_VAR:
      visit_var_decl_part(walker, (const MpplVarDeclPart *) syntax, data);
      break;

    case MPPL_DECL_PART_PROC:
      visit_proc_decl(walker, (const MpplProcDecl *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_var_decl_part(const MpplAstWalker *walker, const MpplVarDeclPart *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_var_decl_part__var_decl_count(syntax); ++i) {
      consume_var_decl(walker, mppl_var_decl_part__var_decl(syntax, i), data);
    }
  }
}

void mppl_ast__walk_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *data)
{
  if (syntax) {
    consume_type(walker, mppl_var_decl__type(syntax), data);
  }
}

void mppl_ast__walk_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *data)
{
  if (syntax) {
    consume_fml_param_list(walker, mppl_proc_decl__fml_param_list(syntax), data);
    consume_var_decl_part(walker, mppl_proc_decl__var_decl_part(syntax), data);
    consume_comp_stmt(walker, mppl_proc_decl__comp_stmt(syntax), data);
  }
}

void mppl_ast__walk_fml_param_list(const MpplAstWalker *walker, const MpplFmlParamList *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_fml_param_list__sec_count(syntax); ++i) {
      consume_fml_param_sec(walker, mppl_fml_param_list__sec(syntax, i), data);
    }
  }
}

void mppl_ast__walk_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *data)
{
  if (syntax) {
    consume_type(walker, mppl_fml_param_sec__type(syntax), data);
  }
}

void mppl_ast__walk_stmt(const MpplAstWalker *walker, const AnyMpplStmt *syntax, void *data)
{
  if (syntax) {
    switch (mppl_stmt__kind(syntax)) {
    case MPPL_STMT_ASSIGN:
      visit_assign_stmt(walker, (const MpplAssignStmt *) syntax, data);
      break;

    case MPPL_STMT_IF:
      visit_if_stmt(walker, (const MpplIfStmt *) syntax, data);
      break;

    case MPPL_STMT_WHILE:
      visit_while_stmt(walker, (const MpplWhileStmt *) syntax, data);
      break;

    case MPPL_STMT_BREAK:
      visit_break_stmt(walker, (const MpplBreakStmt *) syntax, data);
      break;

    case MPPL_STMT_CALL:
      visit_call_stmt(walker, (const MpplCallStmt *) syntax, data);
      break;

    case MPPL_STMT_RETURN:
      visit_return_stmt(walker, (const MpplReturnStmt *) syntax, data);
      break;

    case MPPL_STMT_INPUT:
      visit_input_stmt(walker, (const MpplInputStmt *) syntax, data);
      break;

    case MPPL_STMT_OUTPUT:
      visit_output_stmt(walker, (const MpplOutputStmt *) syntax, data);
      break;

    case MPPL_STMT_COMP:
      visit_comp_stmt(walker, (const MpplCompStmt *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_assign_stmt(const MpplAstWalker *walker, const MpplAssignStmt *syntax, void *data)
{
  if (syntax) {
    consume_var(walker, mppl_assign_stmt__lhs(syntax), data);
    consume_expr(walker, mppl_assign_stmt__rhs(syntax), data);
  }
}

void mppl_ast__walk_if_stmt(const MpplAstWalker *walker, const MpplIfStmt *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_if_stmt__cond(syntax), data);
    consume_stmt(walker, mppl_if_stmt__then_stmt(syntax), data);
    consume_stmt(walker, mppl_if_stmt__else_stmt(syntax), data);
  }
}

void mppl_ast__walk_while_stmt(const MpplAstWalker *walker, const MpplWhileStmt *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_while_stmt__cond(syntax), data);
    consume_stmt(walker, mppl_while_stmt__do_stmt(syntax), data);
  }
}

void mppl_ast__walk_break_stmt(const MpplAstWalker *walker, const MpplBreakStmt *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}

void mppl_ast__walk_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *data)
{
  if (syntax) {
    consume_act_param_list(walker, mppl_call_stmt__act_param_list(syntax), data);
  }
}

void mppl_ast__walk_return_stmt(const MpplAstWalker *walker, const MpplReturnStmt *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}

void mppl_ast__walk_input_stmt(const MpplAstWalker *walker, const MpplInputStmt *syntax, void *data)
{
  if (syntax) {
    consume_input_list(walker, mppl_input_stmt__input_list(syntax), data);
  }
}

void mppl_ast__walk_output_stmt(const MpplAstWalker *walker, const MpplOutputStmt *syntax, void *data)
{
  if (syntax) {
    consume_output_list(walker, mppl_output_stmt__output_list(syntax), data);
  }
}

void mppl_ast__walk_comp_stmt(const MpplAstWalker *walker, const MpplCompStmt *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_comp_stmt__stmt_count(syntax); ++i) {
      consume_stmt(walker, mppl_comp_stmt__stmt(syntax, i), data);
    }
  }
}

void mppl_ast__walk_act_param_list(const MpplAstWalker *walker, const MpplActParamList *syntax, void *data)
{
  if (syntax) {
    unsigned long i;
    for (i = 0; i < mppl_act_param_list__expr_count(syntax); ++i) {
      consume_expr(walker, mppl_act_param_list__expr(syntax, i), data);
    }
  }
}

void mppl_ast__walk_expr(const MpplAstWalker *walker, const AnyMpplExpr *syntax, void *data)
{
  if (syntax) {
    switch (mppl_expr__kind(syntax)) {
    case MPPL_EXPR_BINARY:
      visit_binary_expr(walker, (const MpplBinaryExpr *) syntax, data);
      break;

    case MPPL_EXPR_PAREN:
      visit_paren_expr(walker, (const MpplParenExpr *) syntax, data);
      break;

    case MPPL_EXPR_NOT:
      visit_not_expr(walker, (const MpplNotExpr *) syntax, data);
      break;

    case MPPL_EXPR_CAST:
      visit_cast_expr(walker, (const MpplCastExpr *) syntax, data);
      break;

    case MPPL_EXPR_VAR:
      visit_var(walker, (const AnyMpplVar *) syntax, data);
      break;

    case MPPL_EXPR_LIT:
      visit_lit(walker, (const AnyMpplLit *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_binary_expr(const MpplAstWalker *walker, const MpplBinaryExpr *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_binary_expr__lhs(syntax), data);
    consume_expr(walker, mppl_binary_expr__rhs(syntax), data);
  }
}

void mppl_ast__walk_paren_expr(const MpplAstWalker *walker, const MpplParenExpr *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_paren_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_not_expr(const MpplAstWalker *walker, const MpplNotExpr *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_not_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_cast_expr(const MpplAstWalker *walker, const MpplCastExpr *syntax, void *data)
{
  if (syntax) {
    consume_std_type(walker, mppl_cast_expr__type(syntax), data);
    consume_expr(walker, mppl_cast_expr__expr(syntax), data);
  }
}

void mppl_ast__walk_var(const MpplAstWalker *walker, const AnyMpplVar *syntax, void *data)
{
  if (syntax) {
    switch (mppl_var__kind(syntax)) {
    case MPPL_VAR_ENTIRE:
      visit_entire_var(walker, (const MpplEntireVar *) syntax, data);
      break;

    case MPPL_VAR_INDEXED:
      visit_indexed_var(walker, (const MpplIndexedVar *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_entire_var(const MpplAstWalker *walker, const MpplEntireVar *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}

void mppl_ast__walk_indexed_var(const MpplAstWalker *walker, const MpplIndexedVar *syntax, void *data)
{
  if (syntax) {
    consume_expr(walker, mppl_indexed_var__expr(syntax), data);
  }
}

void mppl_ast__walk_type(const MpplAstWalker *walker, const AnyMpplType *syntax, void *data)
{
  if (syntax) {
    switch (mppl_type__kind(syntax)) {
    case MPPL_TYPE_ARRAY:
      visit_array_type(walker, (const MpplArrayType *) syntax, data);
      break;

    case MPPL_TYPE_STD:
      visit_std_type(walker, (const AnyMpplStdType *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_array_type(const MpplAstWalker *walker, const MpplArrayType *syntax, void *data)
{
  if (syntax) {
    consume_number_lit(walker, mppl_array_type__size(syntax), data);
    consume_std_type(walker, mppl_array_type__type(syntax), data);
  }
}

void mppl_ast__walk_std_type(const MpplAstWalker *walker, const AnyMpplStdType *type, void *data)
{
  if (type) {
    switch (mppl_std_type__kind(type)) {
    case MPPL_STD_TYPE_BOOLEAN:
      visit_std_type_boolean(walker, (const MpplStdTypeBoolean *) type, data);
      break;

    case MPPL_STD_TYPE_CHAR:
      visit_std_type_char(walker, (const MpplStdTypeChar *) type, data);
      break;

    case MPPL_STD_TYPE_INTEGER:
      visit_std_type_integer(walker, (const MpplStdTypeInteger *) type, data);
      break;
    }
  }
}

void mppl_ast__walk_std_type_boolean(const MpplAstWalker *walker, const MpplStdTypeBoolean *type, void *data)
{
  (void) walker;
  (void) type;
  (void) data;
}

void mppl_ast__walk_std_type_char(const MpplAstWalker *walker, const MpplStdTypeChar *type, void *data)
{
  (void) walker;
  (void) type;
  (void) data;
}

void mppl_ast__walk_std_type_integer(const MpplAstWalker *walker, const MpplStdTypeInteger *type, void *data)
{
  (void) walker;
  (void) type;
  (void) data;
}

void mppl_ast__walk_input_list(const MpplAstWalker *walker, const MpplInputList *list, void *data)
{
  if (list) {
    unsigned long i;
    for (i = 0; i < mppl_input_list__var_count(list); ++i) {
      consume_var(walker, mppl_input_list__var(list, i), data);
    }
  }
}

void mppl_ast__walk_output_list(const MpplAstWalker *walker, const MpplOutList *list, void *data)
{
  if (list) {
    unsigned long i;
    for (i = 0; i < mppl_out_list__out_value_count(list); ++i) {
      consume_output_value(walker, mppl_out_list__out_value(list, i), data);
    }
  }
}

void mppl_ast__walk_output_value(const MpplAstWalker *walker, const MpplOutValue *value, void *data)
{
  if (value) {
    consume_expr(walker, mppl_out_value__expr(value), data);
    consume_number_lit(walker, mppl_out_value__width(value), data);
  }
}

void mppl_ast__walk_lit(const MpplAstWalker *walker, const AnyMpplLit *syntax, void *data)
{
  if (syntax) {
    switch (mppl_lit__kind(syntax)) {
    case MPPL_LIT_NUMBER:
      visit_number_lit(walker, (const MpplNumberLit *) syntax, data);
      break;

    case MPPL_LIT_BOOLEAN:
      visit_boolean_lit(walker, (const MpplBooleanLit *) syntax, data);
      break;

    case MPPL_LIT_STRING:
      visit_string_lit(walker, (const MpplStringLit *) syntax, data);
      break;
    }
  }
}

void mppl_ast__walk_number_lit(const MpplAstWalker *walker, const MpplNumberLit *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}

void mppl_ast__walk_boolean_lit(const MpplAstWalker *walker, const MpplBooleanLit *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}

void mppl_ast__walk_string_lit(const MpplAstWalker *walker, const MpplStringLit *syntax, void *data)
{
  (void) walker;
  (void) syntax;
  (void) data;
}
