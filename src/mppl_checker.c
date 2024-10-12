/* SPDX-License-Identifier: Apache-2.0 */

#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "mppl_ty_ctxt.h"
#include "syntax_tree.h"

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

static void check_syntax(Checker *checker, const SyntaxTree *syntax)
{
  SyntaxEvent event = syntax_event_alloc(syntax);
  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER) {
      switch (syntax->raw->node.kind) {
      case MPPL_SYNTAX_VAR_DECL:
        handle_var_decl((const MpplVarDecl *) syntax, checker);
        break;

      case MPPL_SYNTAX_PROC_HEADING:
        handle_proc_decl_heading((const MpplProcDecl *) syntax, checker);
        break;

      case MPPL_SYNTAX_ASSIGN_STMT:
        handle_assign_stmt((const MpplAssignStmt *) syntax, checker);
        break;

      case MPPL_SYNTAX_WHILE_STMT:
        handle_while_stmt((const MpplWhileStmt *) syntax, checker);
        break;

      case MPPL_SYNTAX_IF_STMT:
        handle_if_stmt((const MpplIfStmt *) syntax, checker);
        break;

      case MPPL_SYNTAX_CALL_STMT:
        handle_call_stmt((const MpplCallStmt *) syntax, checker);
        break;

      case MPPL_SYNTAX_INPUT_STMT:
        handle_input_stmt((const MpplInputStmt *) syntax, checker);
        break;

      case MPPL_SYNTAX_OUTPUT_STMT:
        handle_output_stmt((const MpplOutputStmt *) syntax, checker);
        break;

      default:
        /* do nothing */
        break;
      }
    }
  }
}

MpplCheckResult mppl_check(const MpplRoot *syntax, const MpplSemantics *semantics)
{
  MpplCheckResult result;
  Checker         checker;
  checker.semantics = semantics;
  checker.ctxt      = mppl_ty_ctxt_alloc();
  vec_alloc(&checker.diags, 0);

  check_syntax(&checker, &syntax->syntax);

  result.ctxt        = checker.ctxt;
  result.diags.ptr   = checker.diags.ptr;
  result.diags.count = checker.diags.count;

  return result;
}
