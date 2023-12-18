#include <stddef.h>
#include <stdio.h>

#include "array.h"
#include "pretty_printer.h"
#include "syntax_kind.h"
#include "token.h"

typedef struct Printer     Printer;

struct Printer {
  unsigned long indent;
  PrinterOption option;
  Array         stack;
};

static TokenNode *node(Printer *printer)
{
  return **((TokenNode ***) array_back(&printer->stack));
}

static TokenTree *tree(Printer *printer)
{
  return (TokenTree *) node(printer);
}

static Token *token(Printer *printer)
{
  return (Token *) node(printer);
}

static void tree_start(Printer *printer)
{
  array_push(&printer->stack, &tree(printer)->children);
}

static void node_next(Printer *printer)
{
  TokenNode ***node = array_back(&printer->stack);
  ++(*node);
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
  printf("\033[38;2;%u;%u;%um", (unsigned) (color >> 16) & 0xFF, (unsigned) (color >> 8) & 0xFF, (unsigned) color & 0xFF);
  printf("%s", token(printer)->text);
  printf("\033[0m");
  node_next(printer);
}

static void consume_type(Printer *printer)
{
  if (node(printer)->kind == SYNTAX_KIND_ARRAY_TYPE) {
    tree_start(printer);
    consume_token(printer, printer->option.color.keyword);
    consume_token(printer, printer->option.color.foreground);
    consume_token(printer, printer->option.color.literal);
    consume_token(printer, printer->option.color.foreground);
    space();
    consume_token(printer, printer->option.color.keyword);
    space();
    consume_token(printer, printer->option.color.keyword);
    tree_end(printer);
  } else {
    consume_token(printer, printer->option.color.keyword);
  }
}

static void consume_expression(Printer *printer);

static void consume_entire_variable(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_indexed_variable(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  consume_token(printer, printer->option.color.foreground);
  consume_expression(printer);
  consume_token(printer, printer->option.color.foreground);
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
    consume_token(printer, printer->option.color.operator);
    space();
    consume_expression(printer);
  } else {
    node_next(printer);
    consume_token(printer, printer->option.color.operator);
    consume_expression(printer);
  }
  tree_end(printer);
}

static void consume_parenthesized_expression(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  consume_expression(printer);
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_not_expression(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_expression(printer);
  tree_end(printer);
}

static void consume_cast_expression(Printer *printer)
{
  tree_start(printer);
  consume_type(printer);
  consume_token(printer, printer->option.color.foreground);
  consume_expression(printer);
  consume_token(printer, printer->option.color.foreground);
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
    consume_token(printer, printer->option.color.string);
    break;
  case SYNTAX_KIND_INTEGER_LITERAL:
  case SYNTAX_KIND_TRUE_KEYWORD:
  case SYNTAX_KIND_FALSE_KEYWORD:
    consume_token(printer, printer->option.color.literal);
    break;
  default:
    /* do nothing */
    break;
  }
}

static void consume_statement(Printer *printer);

static void consume_assignment_statement(Printer *printer)
{
  tree_start(printer);
  consume_variable(printer);
  space();
  consume_token(printer, printer->option.color.operator);
  space();
  consume_expression(printer);
  tree_end(printer);
}

static void consume_if_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_expression(printer);
  space();
  consume_token(printer, printer->option.color.keyword);
  newline();
  ++printer->indent;
  indent(printer);
  consume_statement(printer);
  --printer->indent;
  if (node(printer)) {
    newline();
    indent(printer);
    consume_token(printer, printer->option.color.keyword);
    if (node(printer)->kind == SYNTAX_KIND_IF_STATEMENT) {
      space();
      consume_statement(printer);
    } else {
      newline();
      ++printer->indent;
      indent(printer);
      consume_statement(printer);
      --printer->indent;
    }
  }
  tree_end(printer);
}

static void consume_while_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_expression(printer);
  space();
  consume_token(printer, printer->option.color.keyword);
  newline();
  ++printer->indent;
  indent(printer);
  consume_statement(printer);
  --printer->indent;
  tree_end(printer);
}

static void consume_actual_parameter_list(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  while (1) {
    consume_expression(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    space();
  }
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_break_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  tree_end(printer);
}

static void consume_call_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_token(printer, printer->option.color.procedure);
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
  consume_token(printer, printer->option.color.keyword);
  tree_end(printer);
}

static void consume_input_list(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  while (1) {
    consume_variable(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    space();
  }
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_input_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
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
    consume_token(printer, printer->option.color.foreground);
    space();
    consume_token(printer, printer->option.color.literal);
  }
  tree_end(printer);
}

static void consume_output_list(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  while (1) {
    consume_output_value(printer);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    space();
  }
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_output_statement(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
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
  consume_token(printer, printer->option.color.keyword);
  newline();
  ++printer->indent;
  while (1) {
    indent(printer);
    consume_statement(printer);
    if (node(printer)->kind != SYNTAX_KIND_SEMICOLON_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    newline();
  }
  --printer->indent;
  newline();
  indent(printer);
  consume_token(printer, printer->option.color.keyword);
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
    consume_token(printer, printer->option.color.foreground);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    space();
  }
  space();
  consume_token(printer, printer->option.color.foreground);
  space();
  consume_type(printer);
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_variable_declaration_part(Printer *printer)
{
  unsigned long count = tree(printer)->children_count;
  unsigned long index;
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  newline();
  ++printer->indent;
  for (index = 1; index < count; ++index) {
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
    consume_token(printer, printer->option.color.parameter);
    if (node(printer)->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    consume_token(printer, printer->option.color.foreground);
    space();
  }
  space();
  consume_token(printer, printer->option.color.foreground);
  space();
  consume_type(printer);
  tree_end(printer);
}

static void consume_formal_parameter_list(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.foreground);
  while (node(printer)->kind == SYNTAX_KIND_FORMAL_PARAMETER_SECTION) {
    consume_formal_parameter_section(printer);
  }
  consume_token(printer, printer->option.color.foreground);
  tree_end(printer);
}

static void consume_procedure_declaration(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_token(printer, printer->option.color.procedure);
  if (node(printer)) {
    consume_formal_parameter_list(printer);
  } else {
    node_next(printer);
  }
  consume_token(printer, printer->option.color.foreground);
  newline();
  if (node(printer)) {
    indent(printer);
    consume_variable_declaration_part(printer);
  } else {
    node_next(printer);
  }
  indent(printer);
  consume_compound_statement(printer);
  consume_token(printer, printer->option.color.foreground);
  newline();
  tree_end(printer);
}

static void consume_program(Printer *printer)
{
  tree_start(printer);
  consume_token(printer, printer->option.color.keyword);
  space();
  consume_token(printer, printer->option.color.program);
  consume_token(printer, printer->option.color.foreground);
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
  consume_token(printer, printer->option.color.foreground);
  newline();
  tree_end(printer);
}

void mppl_pretty_print(const TokenNode *node, const PrinterOption *option)
{
  TokenNode **root = (TokenNode **) &node;
  Printer printer;
  printer.indent = 0;
  array_init(&printer.stack, sizeof(TokenNode **));
  array_push(&printer.stack, &root);
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
