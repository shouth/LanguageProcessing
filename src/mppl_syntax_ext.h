/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef MPPL_SYNTAX_EXT_H
#define MPPL_SYNTAX_EXT_H

#include "context_fwd.h"
#include "mppl_syntax.h"

typedef struct MpplAstWalker MpplAstWalker;

struct MpplAstWalker {
  void (*visit_program)(const MpplAstWalker *walker, const MpplProgram *program, void *data);
  void (*visit_decl_part)(const MpplAstWalker *walker, const AnyMpplDeclPart *part, void *data);
  void (*visit_var_decl_part)(const MpplAstWalker *walker, const MpplVarDeclPart *part, void *data);
  void (*visit_var_decl)(const MpplAstWalker *walker, const MpplVarDecl *decl, void *data);
  void (*visit_proc_decl)(const MpplAstWalker *walker, const MpplProcDecl *decl, void *data);
  void (*visit_fml_param_list)(const MpplAstWalker *walker, const MpplFmlParamList *list, void *data);
  void (*visit_fml_param_sec)(const MpplAstWalker *walker, const MpplFmlParamSec *sec, void *data);
  void (*visit_stmt)(const MpplAstWalker *walker, const AnyMpplStmt *stmt, void *data);
  void (*visit_assign_stmt)(const MpplAstWalker *walker, const MpplAssignStmt *stmt, void *data);
  void (*visit_if_stmt)(const MpplAstWalker *walker, const MpplIfStmt *stmt, void *data);
  void (*visit_while_stmt)(const MpplAstWalker *walker, const MpplWhileStmt *stmt, void *data);
  void (*visit_break_stmt)(const MpplAstWalker *walker, const MpplBreakStmt *stmt, void *data);
  void (*visit_call_stmt)(const MpplAstWalker *walker, const MpplCallStmt *stmt, void *data);
  void (*visit_return_stmt)(const MpplAstWalker *walker, const MpplReturnStmt *stmt, void *data);
  void (*visit_input_stmt)(const MpplAstWalker *walker, const MpplInputStmt *stmt, void *data);
  void (*visit_output_stmt)(const MpplAstWalker *walker, const MpplOutputStmt *stmt, void *data);
  void (*visit_comp_stmt)(const MpplAstWalker *walker, const MpplCompStmt *stmt, void *data);
  void (*visit_act_param_list)(const MpplAstWalker *walker, const MpplActParamList *list, void *data);
  void (*visit_expr)(const MpplAstWalker *walker, const AnyMpplExpr *expr, void *data);
  void (*visit_binary_expr)(const MpplAstWalker *walker, const MpplBinaryExpr *expr, void *data);
  void (*visit_paren_expr)(const MpplAstWalker *walker, const MpplParenExpr *expr, void *data);
  void (*visit_not_expr)(const MpplAstWalker *walker, const MpplNotExpr *expr, void *data);
  void (*visit_cast_expr)(const MpplAstWalker *walker, const MpplCastExpr *expr, void *data);
  void (*visit_var)(const MpplAstWalker *walker, const AnyMpplVar *var, void *data);
  void (*visit_entire_var)(const MpplAstWalker *walker, const MpplEntireVar *var, void *data);
  void (*visit_indexed_var)(const MpplAstWalker *walker, const MpplIndexedVar *var, void *data);
  void (*visit_type)(const MpplAstWalker *walker, const AnyMpplType *type, void *data);
  void (*visit_array_type)(const MpplAstWalker *walker, const MpplArrayType *type, void *data);
  void (*visit_std_type)(const MpplAstWalker *walker, const AnyMpplStdType *type, void *data);
  void (*visit_std_type_integer)(const MpplAstWalker *walker, const MpplStdTypeInteger *type, void *data);
  void (*visit_std_type_boolean)(const MpplAstWalker *walker, const MpplStdTypeBoolean *type, void *data);
  void (*visit_std_type_char)(const MpplAstWalker *walker, const MpplStdTypeChar *type, void *data);
  void (*visit_input_list)(const MpplAstWalker *walker, const MpplInputList *list, void *data);
  void (*visit_output_list)(const MpplAstWalker *walker, const MpplOutList *list, void *data);
  void (*visit_output_value)(const MpplAstWalker *walker, const MpplOutValue *value, void *data);
  void (*visit_lit)(const MpplAstWalker *walker, const AnyMpplLit *lit, void *data);
  void (*visit_number_lit)(const MpplAstWalker *walker, const MpplNumberLit *lit, void *data);
  void (*visit_boolean_lit)(const MpplAstWalker *walker, const MpplBooleanLit *lit, void *data);
  void (*visit_string_lit)(const MpplAstWalker *walker, const MpplStringLit *lit, void *data);
};

const Type *mppl_std_type__to_type(const AnyMpplStdType *syntax);

const Type *mppl_type__to_type(const AnyMpplType *syntax, Ctx *ctx);

long mppl_lit_number__to_long(const MpplNumberLit *syntax);

char *mppl_lit_string__to_string(const MpplStringLit *syntax);

int mppl_lit_boolean__to_int(const MpplBooleanLit *syntax);

void mppl_ast_walker__setup(MpplAstWalker *walker);
void mppl_ast_walker__travel(MpplAstWalker *walker, const MpplProgram *syntax, void *data);

void mppl_ast__walk_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *data);
void mppl_ast__walk_decl_part(const MpplAstWalker *walker, const AnyMpplDeclPart *syntax, void *data);
void mppl_ast__walk_var_decl_part(const MpplAstWalker *walker, const MpplVarDeclPart *syntax, void *data);
void mppl_ast__walk_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *data);
void mppl_ast__walk_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *data);
void mppl_ast__walk_fml_param_list(const MpplAstWalker *walker, const MpplFmlParamList *syntax, void *data);
void mppl_ast__walk_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *data);
void mppl_ast__walk_stmt(const MpplAstWalker *walker, const AnyMpplStmt *syntax, void *data);
void mppl_ast__walk_assign_stmt(const MpplAstWalker *walker, const MpplAssignStmt *syntax, void *data);
void mppl_ast__walk_if_stmt(const MpplAstWalker *walker, const MpplIfStmt *syntax, void *data);
void mppl_ast__walk_while_stmt(const MpplAstWalker *walker, const MpplWhileStmt *syntax, void *data);
void mppl_ast__walk_break_stmt(const MpplAstWalker *walker, const MpplBreakStmt *syntax, void *data);
void mppl_ast__walk_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *data);
void mppl_ast__walk_return_stmt(const MpplAstWalker *walker, const MpplReturnStmt *syntax, void *data);
void mppl_ast__walk_input_stmt(const MpplAstWalker *walker, const MpplInputStmt *syntax, void *data);
void mppl_ast__walk_output_stmt(const MpplAstWalker *walker, const MpplOutputStmt *syntax, void *data);
void mppl_ast__walk_comp_stmt(const MpplAstWalker *walker, const MpplCompStmt *syntax, void *data);
void mppl_ast__walk_act_param_list(const MpplAstWalker *walker, const MpplActParamList *syntax, void *data);
void mppl_ast__walk_expr(const MpplAstWalker *walker, const AnyMpplExpr *syntax, void *data);
void mppl_ast__walk_binary_expr(const MpplAstWalker *walker, const MpplBinaryExpr *syntax, void *data);
void mppl_ast__walk_paren_expr(const MpplAstWalker *walker, const MpplParenExpr *syntax, void *data);
void mppl_ast__walk_not_expr(const MpplAstWalker *walker, const MpplNotExpr *syntax, void *data);
void mppl_ast__walk_cast_expr(const MpplAstWalker *walker, const MpplCastExpr *syntax, void *data);
void mppl_ast__walk_var(const MpplAstWalker *walker, const AnyMpplVar *syntax, void *data);
void mppl_ast__walk_entire_var(const MpplAstWalker *walker, const MpplEntireVar *syntax, void *data);
void mppl_ast__walk_indexed_var(const MpplAstWalker *walker, const MpplIndexedVar *syntax, void *data);
void mppl_ast__walk_type(const MpplAstWalker *walker, const AnyMpplType *syntax, void *data);
void mppl_ast__walk_array_type(const MpplAstWalker *walker, const MpplArrayType *syntax, void *data);
void mppl_ast__walk_std_type(const MpplAstWalker *walker, const AnyMpplStdType *syntax, void *data);
void mppl_ast__walk_std_type_integer(const MpplAstWalker *walker, const MpplStdTypeInteger *syntax, void *data);
void mppl_ast__walk_std_type_boolean(const MpplAstWalker *walker, const MpplStdTypeBoolean *syntax, void *data);
void mppl_ast__walk_std_type_char(const MpplAstWalker *walker, const MpplStdTypeChar *syntax, void *data);
void mppl_ast__walk_input_list(const MpplAstWalker *walker, const MpplInputList *syntax, void *data);
void mppl_ast__walk_output_list(const MpplAstWalker *walker, const MpplOutList *syntax, void *data);
void mppl_ast__walk_output_value(const MpplAstWalker *walker, const MpplOutValue *syntax, void *data);
void mppl_ast__walk_lit(const MpplAstWalker *walker, const AnyMpplLit *syntax, void *data);
void mppl_ast__walk_number_lit(const MpplAstWalker *walker, const MpplNumberLit *syntax, void *data);
void mppl_ast__walk_boolean_lit(const MpplAstWalker *walker, const MpplBooleanLit *syntax, void *data);
void mppl_ast__walk_string_lit(const MpplAstWalker *walker, const MpplStringLit *syntax, void *data);

#endif
