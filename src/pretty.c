#include <stddef.h>
#include <stdio.h>

#include "fmt.h"
#include "syn.h"

static void pretty_kw(struct syn_tok const *tok, FILE *out)
{
  struct fmt_style s = { 0 };
  s.color = FMT_BRIGHT_RED;
  fmt_print(out, &s, "%s", tok->text);
}

static void pretty_op(struct syn_tok const *op, FILE *out)
{
  struct fmt_style s = { 0 };
  s.color = FMT_BRIGHT_RED;
  fmt_print(out, &s, "%s", op->text);
}

static void pretty_num(struct syn_tok const *num, FILE *out)
{
  struct fmt_style s = { 0 };
  s.color = FMT_BRIGHT_BLUE;
  fmt_print(out, &s, "%s", num->text);
}

static void pretty_str(struct syn_tok const *str, FILE *out)
{
  struct fmt_style s = { 0 };
  s.color = FMT_BRIGHT_YELLOW;
  fmt_print(out, &s, "%s", str->text);
}

static void pretty_tok(struct syn_tok const *tok, FILE *out)
{
  fprintf(out, "%s", tok->text);
}

static void pretty_indent(int indent, FILE *out)
{
  fprintf(out, "%*s", indent, "");
}

static void pretty_space(FILE *out)
{
  fprintf(out, " ");
}

static void pretty_newline(FILE *out)
{
  fprintf(out, "\n");
}

static void pretty_expr(struct any_syn_expr const *expr, FILE *out);

static void pretty_stmt(struct any_syn_stmt const *stmt, int indent, FILE *out);

static void pretty_type(struct any_syn_type const *type, FILE *out)
{
  switch (any_syn_type_kind(type)) {
  case ANY_SYN_TYPE_INT: {
    struct syn_int_type const *int_type = (struct syn_int_type *) type;
    pretty_kw(int_type->integer_kw, out);
    break;
  }

  case ANY_SYN_TYPE_BOOL: {
    struct syn_bool_type const *bool_type = (struct syn_bool_type *) type;
    pretty_kw(bool_type->boolean_kw, out);
    break;
  }

  case ANY_SYN_TYPE_CHAR: {
    struct syn_char_type const *char_type = (struct syn_char_type *) type;
    pretty_kw(char_type->char_kw, out);
    break;
  }

  case ANY_SYN_TYPE_ARRAY: {
    struct syn_array_type const *array_type = (struct syn_array_type *) type;
    pretty_kw(array_type->array_kw, out);
    pretty_space(out);
    pretty_kw(array_type->lbrkt, out);
    pretty_expr(array_type->size, out);
    pretty_kw(array_type->rbrkt, out);
    pretty_space(out);
    pretty_kw(array_type->of_kw, out);
    pretty_space(out);
    pretty_type(array_type->type, out);
    break;
  }

  default:
    break;
  }
}

static void pretty_expr(struct any_syn_expr const *expr, FILE *out)
{
  switch (any_syn_expr_kind(expr)) {
  case ANY_SYN_EXPR_ENTIRE_VAR: {
    struct syn_entire_var_expr const *entire_var_expr = (struct syn_entire_var_expr *) expr;
    pretty_tok(entire_var_expr->name, out);
    break;
  }

  case ANY_SYN_EXPR_IDX_VAR: {
    struct syn_idx_var_expr const *idx_var_expr = (struct syn_idx_var_expr *) expr;
    pretty_tok(idx_var_expr->name, out);
    pretty_tok(idx_var_expr->lbrkt, out);
    pretty_expr(idx_var_expr->index, out);
    pretty_tok(idx_var_expr->rbrkt, out);
    break;
  }

  case ANY_SYN_EXPR_INT_LIT: {
    struct syn_int_lit_expr const *int_lit_expr = (struct syn_int_lit_expr *) expr;
    pretty_num(int_lit_expr->int_lit, out);
    break;
  }

  case ANY_SYN_EXPR_BOOL_LIT: {
    struct syn_bool_lit_expr const *bool_lit_expr = (struct syn_bool_lit_expr *) expr;
    pretty_kw((struct syn_tok *) bool_lit_expr->bool_lit, out);
    break;
  }

  case ANY_SYN_EXPR_STR_LIT: {
    struct syn_str_lit_expr const *str_lit_expr = (struct syn_str_lit_expr *) expr;
    pretty_str(str_lit_expr->str_lit, out);
    break;
  }

  case ANY_SYN_EXPR_PAREN: {
    struct syn_paren_expr const *paren_expr = (struct syn_paren_expr *) expr;
    pretty_tok(paren_expr->lparen, out);
    pretty_expr(paren_expr->expr, out);
    pretty_tok(paren_expr->rparen, out);
    break;
  }

  case ANY_SYN_EXPR_CAST: {
    struct syn_cast_expr const *cast_expr = (struct syn_cast_expr *) expr;
    pretty_type(cast_expr->type, out);
    pretty_tok(cast_expr->lparen, out);
    pretty_expr(cast_expr->expr, out);
    pretty_tok(cast_expr->rparen, out);
    break;
  }

  case ANY_SYN_EXPR_UNARY: {
    struct syn_unary_expr const *unary_expr = (struct syn_unary_expr *) expr;
    pretty_op((struct syn_tok *) unary_expr->op, out);
    pretty_expr(unary_expr->expr, out);
    break;
  }

  case ANY_SYN_EXPR_BINARY: {
    struct syn_binary_expr const *binary_expr = (struct syn_binary_expr *) expr;
    pretty_expr(binary_expr->lhs, out);
    pretty_space(out);
    pretty_op((struct syn_tok *) binary_expr->op, out);
    pretty_space(out);
    pretty_expr(binary_expr->rhs, out);
    break;
  }

  default:
    break;
  }
}

static void pretty_assign_stmt(struct syn_assign_stmt const *assign_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_expr(assign_stmt->lhs, out);
  pretty_space(out);
  pretty_op(assign_stmt->op, out);
  pretty_space(out);
  pretty_expr(assign_stmt->rhs, out);
}

static void pretty_if_stmt(struct syn_if_stmt const *if_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(if_stmt->if_kw, out);
  pretty_space(out);
  pretty_expr(if_stmt->cond, out);
  pretty_space(out);
  pretty_kw(if_stmt->then_kw, out);

  pretty_newline(out);
  if (any_syn_stmt_kind(if_stmt->then_stmt) == ANY_SYN_STMT_COMP) {
    pretty_stmt(if_stmt->then_stmt, indent, out);
  } else {
    pretty_stmt(if_stmt->then_stmt, indent + 4, out);
  }

  if (if_stmt->else_clause) {
    pretty_newline(out);
    pretty_indent(indent, out);
    pretty_kw(if_stmt->else_clause->else_kw, out);
    pretty_newline(out);
    if (any_syn_stmt_kind(if_stmt->else_clause->else_stmt) == ANY_SYN_STMT_COMP) {
      pretty_stmt(if_stmt->else_clause->else_stmt, indent, out);
    } else {
      pretty_stmt(if_stmt->else_clause->else_stmt, indent + 4, out);
    }
  }
}

static void pretty_while_stmt(struct syn_while_stmt const *while_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(while_stmt->while_kw, out);
  pretty_space(out);
  pretty_expr(while_stmt->cond, out);
  pretty_space(out);
  pretty_kw(while_stmt->do_kw, out);

  pretty_newline(out);
  if (any_syn_stmt_kind(while_stmt->do_stmt) == ANY_SYN_STMT_COMP) {
    pretty_stmt(while_stmt->do_stmt, indent, out);
  } else {
    pretty_stmt(while_stmt->do_stmt, indent + 4, out);
  }
}

static void pretty_break_stmt(struct syn_break_stmt const *break_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(break_stmt->break_kw, out);
}

static void pretty_act_params(struct syn_act_params const *act_params, FILE *out)
{
  size_t i;
  pretty_tok(act_params->lparen, out);
  for (i = 0; i < act_params->expr_list->count; ++i) {
    struct syn_expr_list_item const *item = act_params->expr_list->children[i];
    pretty_expr(item->expr, out);
    if (item->comma) {
      pretty_tok(item->comma, out);
      pretty_space(out);
    }
  }
  pretty_tok(act_params->rparen, out);
}

static void pretty_call_stmt(struct syn_call_stmt const *call_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(call_stmt->call_kw, out);
  pretty_space(out);
  pretty_tok(call_stmt->name, out);
  if (call_stmt->act_params) {
    pretty_act_params(call_stmt->act_params, out);
  }
}

static void pretty_return_stmt(struct syn_return_stmt const *return_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(return_stmt->return_kw, out);
}

static void pretty_input_stmt(struct syn_input_stmt const *input_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(input_stmt->read_kw, out);
  if (input_stmt->act_params) {
    pretty_act_params(input_stmt->act_params, out);
  }
}

static void pretty_output_values(struct syn_output_values const *output_values, FILE *out)
{
  size_t i;
  pretty_tok(output_values->lparen, out);
  for (i = 0; i < output_values->output_value_list->count; ++i) {
    struct syn_output_value_list_item const *item = output_values->output_value_list->children[i];
    pretty_expr(item->output_value->expr, out);
    if (item->output_value->output_fmt) {
      pretty_space(out);
      pretty_tok(item->output_value->output_fmt->colon, out);
      pretty_space(out);
      pretty_expr(item->output_value->output_fmt->width, out);
    }
    if (item->comma) {
      pretty_tok(item->comma, out);
      pretty_space(out);
    }
  }
  pretty_tok(output_values->rparen, out);
}

static void pretty_output_stmt(struct syn_output_stmt const *output_stmt, int indent, FILE *out)
{
  pretty_indent(indent, out);
  pretty_kw(output_stmt->write_kw, out);
  if (output_stmt->output_values) {
    pretty_output_values(output_stmt->output_values, out);
  }
}

static void pretty_comp_stmt(struct syn_comp_stmt const *comp_stmt, int indent, FILE *out)
{
  size_t i;

  pretty_indent(indent, out);
  pretty_kw(comp_stmt->begin_kw, out);
  pretty_newline(out);

  for (i = 0; i < comp_stmt->stmt_list->count; ++i) {
    struct syn_stmt_list_item const *stmt_list_item = comp_stmt->stmt_list->children[i];

    if (i + 1 == comp_stmt->stmt_list->count && any_syn_stmt_kind(stmt_list_item->stmt) == ANY_SYN_STMT_EMPTY) {
      break;
    }

    pretty_stmt(stmt_list_item->stmt, indent + 4, out);
    if (stmt_list_item->semi) {
      pretty_tok(stmt_list_item->semi, out);
    }
    pretty_newline(out);
  }

  pretty_indent(indent, out);
  pretty_kw(comp_stmt->end_kw, out);
}

static void pretty_stmt(struct any_syn_stmt const *stmt, int indent, FILE *out)
{
  switch (any_syn_stmt_kind(stmt)) {
  case ANY_SYN_STMT_ASSIGN:
    pretty_assign_stmt((struct syn_assign_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_IF:
    pretty_if_stmt((struct syn_if_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_WHILE:
    pretty_while_stmt((struct syn_while_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_CALL:
    pretty_call_stmt((struct syn_call_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_BREAK:
    pretty_break_stmt((struct syn_break_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_RETURN:
    pretty_return_stmt((struct syn_return_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_INPUT:
    pretty_input_stmt((struct syn_input_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_OUTPUT:
    pretty_output_stmt((struct syn_output_stmt *) stmt, indent, out);
    break;

  case ANY_SYN_STMT_COMP:
    pretty_comp_stmt((struct syn_comp_stmt *) stmt, indent, out);
    break;

  default:
    break;
  }
}

static void pretty_ident_list(struct syn_ident_list const *ident_list, FILE *out)
{
  size_t i;
  for (i = 0; i < ident_list->count; ++i) {
    struct syn_ident_list_item const *item = ident_list->children[i];
    pretty_tok(item->name, out);
    if (item->comma) {
      pretty_tok(item->comma, out);
      pretty_space(out);
    }
  }
}

static void pretty_var_decl_part(struct syn_var_decl_part const *var_decl_part, int indent, FILE *out)
{
  size_t i;

  pretty_indent(indent, out);
  pretty_kw(var_decl_part->var_kw, out);
  pretty_newline(out);

  for (i = 0; i < var_decl_part->var_decl_list->count; ++i) {
    struct syn_var_decl *var_decl = var_decl_part->var_decl_list->children[i];
    pretty_indent(indent + 4, out);
    pretty_ident_list(var_decl->ident_list, out);
    pretty_space(out);
    pretty_tok(var_decl->colon, out);
    pretty_space(out);
    pretty_type(var_decl->type, out);
    pretty_tok(var_decl->semi, out);
    pretty_newline(out);
  }
}

static void pretty_proc_decl_head(struct syn_proc_decl_head const *proc_decl_head, FILE *out)
{
  pretty_indent(4, out);
  pretty_kw(proc_decl_head->procedure_kw, out);
  pretty_space(out);
  pretty_tok(proc_decl_head->name, out);
  if (proc_decl_head->fml_params) {
    size_t i;
    struct syn_fml_params const *fml_params = proc_decl_head->fml_params;
    pretty_tok(fml_params->lparen, out);
    for (i = 0; i < fml_params->fml_param_list->count; ++i) {
      struct syn_fml_param_sec *fml_param_sec = fml_params->fml_param_list->children[i];
      pretty_ident_list(fml_param_sec->ident_list, out);
      pretty_space(out);
      pretty_tok(fml_param_sec->colon, out);
      pretty_space(out);
      pretty_type(fml_param_sec->type, out);
      if (fml_param_sec->semi) {
        pretty_tok(fml_param_sec->semi, out);
        pretty_space(out);
      }
    }
    pretty_tok(fml_params->rparen, out);
  }
  pretty_tok(proc_decl_head->semi, out);
  pretty_newline(out);
}

static void pretty_proc_decl_part(struct syn_proc_decl_part const *proc_decl_part, FILE *out)
{
  pretty_proc_decl_head(proc_decl_part->proc_decl_head, out);
  pretty_var_decl_part(proc_decl_part->var_decl_part, 4, out);
  pretty_comp_stmt(proc_decl_part->comp_stmt, 4, out);
  pretty_tok(proc_decl_part->semi, out);
  pretty_newline(out);
}

static void pretty_block(struct syn_block const *block, FILE *out)
{
  size_t i;
  for (i = 0; i < block->decl_part_list->count; ++i) {
    switch (any_syn_decl_part_kind(block->decl_part_list->children[i])) {
    case ANY_SYN_DECL_PART_VAR:
      pretty_var_decl_part((struct syn_var_decl_part *) block->decl_part_list->children[i], 0, out);
      break;

    case ANY_SYN_DECL_PART_PROC:
      pretty_proc_decl_part((struct syn_proc_decl_part *) block->decl_part_list->children[i], out);
      break;

    default:
      break;
    }
    pretty_newline(out);
  }
  pretty_comp_stmt(block->comp_stmt, 0, out);
}

static void pretty_program(struct syn_program const *program, FILE *out)
{
  pretty_kw(program->program_kw, out);
  pretty_space(out);
  pretty_tok(program->name, out);
  pretty_tok(program->semi, out);
  pretty_newline(out);
  pretty_block(program->block, out);
  pretty_tok(program->dot, out);
  pretty_newline(out);
}

void pretty(struct syn_program const *program, FILE *out)
{
  pretty_program(program, out);
}
