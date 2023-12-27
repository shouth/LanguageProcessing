#include <stddef.h>
#include <stdio.h>

#include "array.h"
#include "pretty_printer.h"
#include "syntax_kind.h"
#include "token_tree.h"

typedef struct PrinterCursor PrinterCursor;
typedef struct Printer       Printer;

struct PrinterCursor {
  TokenNode   **node;
  unsigned long count;
  unsigned long index;
};

struct Printer {
  unsigned long indent;
  PrinterOption option;
  Array         stack;
};

static const TokenNode *node(Printer *printer)
{
  PrinterCursor *cursor = array_back(&printer->stack);
  return cursor->node[cursor->index];
}

static unsigned long node_count(Printer *printer)
{
  PrinterCursor *cursor = array_back(&printer->stack);
  return cursor->count;
}

static unsigned long node_index(Printer *printer)
{
  PrinterCursor *cursor = array_back(&printer->stack);
  return cursor->index;
}

static void push_stack(Printer *printer, TokenNode **node, unsigned long count)
{
  PrinterCursor cursor;
  cursor.node  = node;
  cursor.count = count;
  cursor.index = 0;
  array_push(&printer->stack, &cursor);
}

static void tree_start(Printer *printer)
{
  const TokenTree *tree = (TokenTree *) node(printer);
  push_stack(printer, tree->children, tree->children_count);
}

static void node_next(Printer *printer)
{
  PrinterCursor *cursor = array_back(&printer->stack);
  ++cursor->index;
}

static void tree_end(Printer *printer)
{
  array_pop(&printer->stack);
  node_next(printer);
}

static void space(void)
{
  printf(" ");
}

static void newline(void)
{
  printf("\n");
}

static void indent(Printer *printer)
{
  printf("%*.s", (int) printer->indent * 4, "");
}

static void consume_token(Printer *printer, unsigned long color)
{
  const Token *token = (Token *) node(printer);
  printf("\033[38;2;%u;%u;%um", (unsigned) (color >> 16) & 0xFF, (unsigned) (color >> 8) & 0xFF, (unsigned) color & 0xFF);
  printf("%s", token->text);
  printf("\033[0m");
  node_next(printer);
}

static void consume_token_foreground(Printer *printer)
{
  consume_token(printer, printer->option.color.foreground);
}

static void consume_token_program(Printer *printer)
{
  consume_token(printer, printer->option.color.program);
}

static void consume_token_keyword(Printer *printer)
{
  consume_token(printer, printer->option.color.keyword);
}

static void consume_token_operator(Printer *printer)
{
  consume_token(printer, printer->option.color.operator);
}

static void consume_token_procedure(Printer *printer)
{
  consume_token(printer, printer->option.color.procedure);
}

static void consume_token_parameter(Printer *printer)
{
  consume_token(printer, printer->option.color.parameter);
}

static void consume_token_string(Printer *printer)
{
  consume_token(printer, printer->option.color.string);
}

static void consume_token_literal(Printer *printer)
{
  consume_token(printer, printer->option.color.literal);
}

static void consume_type(Printer *printer)
{
  if (node(printer)->kind == SYNTAX_KIND_ARRAY_TYPE) {
    tree_start(printer);
    consume_token_keyword(printer);
    consume_token_foreground(printer);
    consume_token_literal(printer);
    consume_token_foreground(printer);
    space();
    consume_token_keyword(printer);
    space();
    consume_token_keyword(printer);
    tree_end(printer);
  } else {
    consume_token_keyword(printer);
  }
}

static void consume_expression(Printer *printer);

static void consume_entire_variable(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_indexed_variable(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  consume_token_foreground(printer);
  consume_expression(printer);
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_variable(Printer *printer)
{
  if (node(printer)->kind == SYNTAX_KIND_ENTIRE_VARIABLE) {
    consume_entire_variable(printer);
  } else {
    consume_indexed_variable(printer);
  }
}

static void consume_binary_expression(Printer *printer)
{
  tree_start(printer);
  if (node(printer)) {
    consume_expression(printer);
    space();
    consume_token_operator(printer);
    space();
    consume_expression(printer);
  } else {
    node_next(printer);
    consume_token_operator(printer);
    consume_expression(printer);
  }
  tree_end(printer);
}

static void consume_parenthesized_expression(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  consume_expression(printer);
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_not_expression(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_expression(printer);
  tree_end(printer);
}

static void consume_cast_expression(Printer *printer)
{
  tree_start(printer);
  consume_type(printer);
  consume_token_foreground(printer);
  consume_expression(printer);
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_expression(Printer *printer)
{
  switch (node(printer)->kind) {
  case SYNTAX_KIND_ENTIRE_VARIABLE:
  case SYNTAX_KIND_INDEXED_VARIABLE:
    consume_variable(printer);
    break;
  case SYNTAX_KIND_BINARY_EXPRESSION:
    consume_binary_expression(printer);
    break;
  case SYNTAX_KIND_PARENTHESIZED_EXPRESSION:
    consume_parenthesized_expression(printer);
    break;
  case SYNTAX_KIND_NOT_EXPRESSION:
    consume_not_expression(printer);
    break;
  case SYNTAX_KIND_CAST_EXPRESSION:
    consume_cast_expression(printer);
    break;
  case SYNTAX_KIND_STRING_LITERAL:
    consume_token_string(printer);
    break;
  case SYNTAX_KIND_INTEGER_LITERAL:
  case SYNTAX_KIND_TRUE_KEYWORD:
  case SYNTAX_KIND_FALSE_KEYWORD:
    consume_token_literal(printer);
    break;
  default:
    /* do nothing */
    break;
  }
}

static void consume_statement(Printer *printer);

static void consume_nested_statement(Printer *printer)
{
  const TokenNode *statement = node(printer);
  if (statement->kind != SYNTAX_KIND_COMPOUND_STATEMENT) {
    ++printer->indent;
    newline();
    indent(printer);
  } else {
    space();
  }
  consume_statement(printer);
  if (statement->kind != SYNTAX_KIND_COMPOUND_STATEMENT) {
    --printer->indent;
  }
}

static void consume_assignment_statement(Printer *printer)
{
  tree_start(printer);
  consume_variable(printer);
  space();
  consume_token_operator(printer);
  space();
  consume_expression(printer);
  tree_end(printer);
}

static void consume_if_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_expression(printer);
  space();
  consume_token_keyword(printer);
  consume_nested_statement(printer);
  if (node(printer)) {
    newline();
    indent(printer);
    consume_token_keyword(printer);
    if (node(printer)->kind == SYNTAX_KIND_IF_STATEMENT) {
      space();
      consume_statement(printer);
    } else {
      consume_nested_statement(printer);
    }
  }
  tree_end(printer);
}

static void consume_while_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_expression(printer);
  space();
  consume_token_keyword(printer);
  consume_nested_statement(printer);
  tree_end(printer);
}

static void consume_actual_parameter_list(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  while (1) {
    consume_expression(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    space();
  }
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_break_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  tree_end(printer);
}

static void consume_call_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_token_procedure(printer);
  if (node(printer)) {
    consume_actual_parameter_list(printer);
  } else {
    node_next(printer);
  }
  tree_end(printer);
}

static void consume_return_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  tree_end(printer);
}

static void consume_input_list(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  while (1) {
    consume_variable(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    space();
  }
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_input_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  if (node(printer)) {
    consume_input_list(printer);
  } else {
    node_next(printer);
  }
  tree_end(printer);
}

static void consume_output_value(Printer *printer)
{
  tree_start(printer);
  consume_expression(printer);
  if (node(printer)) {
    space();
    consume_token_foreground(printer);
    space();
    consume_token_literal(printer);
  }
  tree_end(printer);
}

static void consume_output_list(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  while (1) {
    consume_output_value(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    space();
  }
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_output_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  if (node(printer)) {
    consume_output_list(printer);
  } else {
    node_next(printer);
  }
  tree_end(printer);
}

static void consume_compound_statement(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  newline();
  ++printer->indent;
  while (1) {
    indent(printer);
    consume_statement(printer);
    if (node(printer)->kind != SYNTAX_KIND_SEMICOLON_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    if (!node(printer) && node_index(printer) + 2 == node_count(printer)) {
      node_next(printer);
      break;
    }
    newline();
  }
  --printer->indent;
  newline();
  indent(printer);
  consume_token_keyword(printer);
  tree_end(printer);
}

static void consume_statement(Printer *printer)
{
  switch (node(printer)->kind) {
  case SYNTAX_KIND_ASSIGNMENT_STATEMENT:
    consume_assignment_statement(printer);
    break;
  case SYNTAX_KIND_IF_STATEMENT:
    consume_if_statement(printer);
    break;
  case SYNTAX_KIND_WHILE_STATEMENT:
    consume_while_statement(printer);
    break;
  case SYNTAX_KIND_BREAK_STATEMENT:
    consume_break_statement(printer);
    break;
  case SYNTAX_KIND_CALL_STATEMENT:
    consume_call_statement(printer);
    break;
  case SYNTAX_KIND_RETURN_STATEMENT:
    consume_return_statement(printer);
    break;
  case SYNTAX_KIND_INPUT_STATEMENT:
    consume_input_statement(printer);
    break;
  case SYNTAX_KIND_OUTPUT_STATEMENT:
    consume_output_statement(printer);
    break;
  case SYNTAX_KIND_COMPOUND_STATEMENT:
    consume_compound_statement(printer);
    break;
  default:
    /* do nothing */
    break;
  }
}

static void consume_variable_declaration(Printer *printer)
{
  tree_start(printer);
  while (1) {
    consume_token_foreground(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    space();
  }
  space();
  consume_token_foreground(printer);
  space();
  consume_type(printer);
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_variable_declaration_part(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  newline();
  ++printer->indent;
  while (node_index(printer) < node_count(printer)) {
    indent(printer);
    consume_variable_declaration(printer);
    newline();
  }
  --printer->indent;
  tree_end(printer);
}

static void consume_formal_parameter_section(Printer *printer)
{
  tree_start(printer);
  while (1) {
    consume_token_parameter(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token_foreground(printer);
    space();
  }
  space();
  consume_token_foreground(printer);
  space();
  consume_type(printer);
  tree_end(printer);
}

static void consume_formal_parameter_list(Printer *printer)
{
  tree_start(printer);
  consume_token_foreground(printer);
  while (node(printer)->kind == SYNTAX_KIND_FORMAL_PARAMETER_SECTION) {
    consume_formal_parameter_section(printer);
  }
  consume_token_foreground(printer);
  tree_end(printer);
}

static void consume_procedure_declaration(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_token_procedure(printer);
  if (node(printer)) {
    consume_formal_parameter_list(printer);
  } else {
    node_next(printer);
  }
  consume_token_foreground(printer);
  newline();
  if (node(printer)) {
    indent(printer);
    consume_variable_declaration_part(printer);
  } else {
    node_next(printer);
  }
  indent(printer);
  consume_compound_statement(printer);
  consume_token_foreground(printer);
  newline();
  tree_end(printer);
}

static void consume_program(Printer *printer)
{
  tree_start(printer);
  consume_token_keyword(printer);
  space();
  consume_token_program(printer);
  consume_token_foreground(printer);
  newline();
  ++printer->indent;
  while (1) {
    if (node(printer)->kind == SYNTAX_KIND_VARIABLE_DECLARATION_PART) {
      indent(printer);
      consume_variable_declaration_part(printer);
    } else if (node(printer)->kind == SYNTAX_KIND_PROCEDURE_DECLARATION) {
      indent(printer);
      consume_procedure_declaration(printer);
    } else {
      break;
    }
  }
  --printer->indent;
  indent(printer);
  consume_compound_statement(printer);
  consume_token_foreground(printer);
  newline();
  tree_end(printer);
}

void mppl_pretty_print(const TokenNode *node, const PrinterOption *option)
{
  Printer printer;
  printer.indent = 0;
  array_init(&printer.stack, sizeof(PrinterCursor));
  push_stack(&printer, (TokenNode **) &node, 1);
  if (option) {
    printer.option = *option;
  } else {
    /* clang-format off */
    printer.option.color.foreground = 0xE6EDF3;
    printer.option.color.program    = 0xD2A8FF;
    printer.option.color.keyword    = 0xFF7B72;
    printer.option.color.operator   = 0xFF7B72;
    printer.option.color.procedure  = 0xD2A8FF;
    printer.option.color.parameter  = 0xFFA657;
    printer.option.color.string     = 0xA5D6FF;
    printer.option.color.literal    = 0x79C0FF;
    /* clang-format on */
  }
  consume_program(&printer);
  array_deinit(&printer.stack);
}
