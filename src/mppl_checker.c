/* SPDX-License-Identifier: Apache-2.0 */

#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "mppl_ty_ctxt.h"

typedef struct Checker Checker;

struct Checker {
  const MpplSemantics *semantics;
  MpplTyCtxt          *ctxt;
  Vec(Report *) diags;
};

static void handle_var_decl(const MpplVarDecl *var_decl, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_proc_decl_heading(const MpplProcDecl *proc_decl, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_assign_stmt(const MpplAssignStmt *assign_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_while_stmt(const MpplWhileStmt *while_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_if_stmt(const MpplIfStmt *if_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_call_stmt(const MpplCallStmt *call_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_input_stmt(const MpplInputStmt *input_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static void handle_output_stmt(const MpplOutputStmt *output_stmt, Checker *checker)
{
  /* TODO: Implement */
}

static int syntax_visitor(const SyntaxTree *syntax, int enter, void *data)
{
  Checker *checker = data;
  if (enter) {
    switch (syntax->raw->node.kind) {
    case MPPL_SYNTAX_VAR_DECL:
      handle_var_decl((const MpplVarDecl *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_PROC_HEADING:
      handle_proc_decl_heading((const MpplProcDecl *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_ASSIGN_STMT:
      handle_assign_stmt((const MpplAssignStmt *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_WHILE_STMT:
      handle_while_stmt((const MpplWhileStmt *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_IF_STMT:
      handle_if_stmt((const MpplIfStmt *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_CALL_STMT:
      handle_call_stmt((const MpplCallStmt *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_INPUT_STMT:
      handle_input_stmt((const MpplInputStmt *) syntax, checker);
      return 0;

    case MPPL_SYNTAX_OUTPUT_STMT:
      handle_output_stmt((const MpplOutputStmt *) syntax, checker);
      return 0;

    default:
      /* do nothing */
      break;
    }
  }
  return 1;
}

MpplCheckResult mppl_check(const MpplRoot *syntax, const MpplSemantics *semantics)
{
  MpplCheckResult result;
  Checker         checker;
  checker.semantics = semantics;
  checker.ctxt      = mppl_ty_ctxt_alloc();
  vec_alloc(&checker.diags, 0);

  syntax_tree_visit((const SyntaxTree *) syntax, &syntax_visitor, &checker);

  result.ctxt        = checker.ctxt;
  result.diags.ptr   = checker.diags.ptr;
  result.diags.count = checker.diags.count;

  return result;
}
