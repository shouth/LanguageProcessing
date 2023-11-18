#include <stdio.h>

#include "pretty_printer.h"
#include "syntax_kind.h"
#include "token.h"

typedef struct Printer Printer;

struct Printer {
  unsigned long indent;
  PrinterOption option;
};

static void print_space(void)
{
  printf(" ");
}

static void print_newline(void)
{
  printf("\n");
}

static void print_indent(Printer *printer)
{
  printf("%*.s", (int) printer->indent * 4, "");
}

static void print_token(Printer *printer, const TokenNode *node, unsigned long color)
{
  const Token *token = (Token *) node;
  printf("\033[38;2;%u;%u;%um", (unsigned) (color >> 16) & 0xFF, (unsigned) (color >> 8) & 0xFF, (unsigned) color & 0xFF);
  printf("%s", token->text);
  printf("\033[0m");
}

static void print_type(Printer *printer, const TokenNode *node)
{
  if (node->kind == SYNTAX_KIND_ARRAY_TYPE) {
    const TokenTree *tree  = (TokenTree *) node;
    unsigned long    index = 0;

    print_token(printer, tree->children[index++], printer->option.color.keyword);
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_token(printer, tree->children[index++], printer->option.color.literal);
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
    print_token(printer, tree->children[index++], printer->option.color.keyword);
    print_space();
    print_token(printer, tree->children[index++], printer->option.color.keyword);
  } else {
    print_token(printer, node, printer->option.color.keyword);
  }
}

static void print_expression(Printer *printer, const TokenNode *node);

static void print_entire_variable(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_indexed_variable(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_expression(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_variable(Printer *printer, const TokenNode *node)
{
  if (node->kind == SYNTAX_KIND_ENTIRE_VARIABLE) {
    print_entire_variable(printer, node);
  } else {
    print_indexed_variable(printer, node);
  }
}

static void print_binary_expression(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  if (tree->children[index]) {
    print_expression(printer, tree->children[index++]);
    print_space();
    print_token(printer, tree->children[index++], printer->option.color.operator);
    print_space();
    print_expression(printer, tree->children[index++]);
  } else {
    index++;
    print_token(printer, tree->children[index++], printer->option.color.operator);
    print_expression(printer, tree->children[index++]);
  }
}

static void print_parenthesized_expression(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_expression(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_not_expression(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_expression(printer, tree->children[index++]);
}

static void print_cast_expression(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_type(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_expression(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_expression(Printer *printer, const TokenNode *node)
{
  switch (node->kind) {
  case SYNTAX_KIND_ENTIRE_VARIABLE:
  case SYNTAX_KIND_INDEXED_VARIABLE:
    print_variable(printer, node);
    break;
  case SYNTAX_KIND_BINARY_EXPRESSION:
    print_binary_expression(printer, node);
    break;
  case SYNTAX_KIND_PARENTHESIZED_EXPRESSION:
    print_parenthesized_expression(printer, node);
    break;
  case SYNTAX_KIND_NOT_EXPRESSION:
    print_not_expression(printer, node);
    break;
  case SYNTAX_KIND_CAST_EXPRESSION:
    print_cast_expression(printer, node);
    break;
  case SYNTAX_KIND_STRING_LITERAL:
    print_token(printer, node, printer->option.color.string);
    break;
  case SYNTAX_KIND_INTEGER_LITERAL:
  case SYNTAX_KIND_TRUE_KEYWORD:
  case SYNTAX_KIND_FALSE_KEYWORD:
    print_token(printer, node, printer->option.color.literal);
    break;
  default:
    /* do nothing */
    break;
  }
}

static void print_statement(Printer *printer, const TokenNode *node);

static void print_assignment_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_variable(printer, tree->children[index++]);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.operator);
  print_space();
  print_expression(printer, tree->children[index++]);
}

static void print_if_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_expression(printer, tree->children[index++]);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_newline();
  ++printer->indent;
  print_indent(printer);
  print_statement(printer, tree->children[index++]);
  --printer->indent;
  if (tree->children[index]) {
    print_newline();
    print_indent(printer);
    print_token(printer, tree->children[index++], printer->option.color.keyword);
    if (tree->children[index]->kind == SYNTAX_KIND_IF_STATEMENT) {
      print_space();
      print_statement(printer, tree->children[index++]);
    } else {
      print_newline();
      ++printer->indent;
      print_indent(printer);
      print_statement(printer, tree->children[index++]);
      --printer->indent;
    }
  }
}

static void print_while_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_expression(printer, tree->children[index++]);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_newline();
  ++printer->indent;
  print_indent(printer);
  print_statement(printer, tree->children[index++]);
  --printer->indent;
}

static void print_actual_parameter_list(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
  while (1) {
    print_expression(printer, tree->children[index++]);
    if (tree->children[index]->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
  }
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_break_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
}

static void print_call_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.procedure);
  if (tree->children[index]) {
    print_actual_parameter_list(printer, tree->children[index++]);
  } else {
    index++;
  }
}

static void print_return_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
}

static void print_input_list(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
  while (1) {
    print_variable(printer, tree->children[index++]);
    if (tree->children[index]->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
  }
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_input_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  if (tree->children[index]) {
    print_input_list(printer, tree->children[index++]);
  } else {
    index++;
  }
}

static void print_output_value(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_expression(printer, tree->children[index++]);
  if (tree->children[index]) {
    print_space();
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
    print_token(printer, tree->children[index++], printer->option.color.literal);
  }
}

static void print_output_list(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
  while (1) {
    print_output_value(printer, tree->children[index++]);
    if (tree->children[index]->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
  }
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_output_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  if (tree->children[index]) {
    print_output_list(printer, tree->children[index++]);
  } else {
    index++;
  }
}

static void print_compound_statement(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_newline();
  ++printer->indent;
  while (1) {
    print_indent(printer);
    print_statement(printer, tree->children[index++]);
    if (tree->children[index]->kind != SYNTAX_KIND_SEMICOLON_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_newline();
  }
  --printer->indent;
  print_newline();
  print_indent(printer);
  print_token(printer, tree->children[index++], printer->option.color.keyword);
}

static void print_statement(Printer *printer, const TokenNode *node)
{
  switch (node->kind) {
  case SYNTAX_KIND_ASSIGNMENT_STATEMENT:
    print_assignment_statement(printer, node);
    break;
  case SYNTAX_KIND_IF_STATEMENT:
    print_if_statement(printer, node);
    break;
  case SYNTAX_KIND_WHILE_STATEMENT:
    print_while_statement(printer, node);
    break;
  case SYNTAX_KIND_BREAK_STATEMENT:
    print_break_statement(printer, node);
    break;
  case SYNTAX_KIND_CALL_STATEMENT:
    print_call_statement(printer, node);
    break;
  case SYNTAX_KIND_RETURN_STATEMENT:
    print_return_statement(printer, node);
    break;
  case SYNTAX_KIND_INPUT_STATEMENT:
    print_input_statement(printer, node);
    break;
  case SYNTAX_KIND_OUTPUT_STATEMENT:
    print_output_statement(printer, node);
    break;
  case SYNTAX_KIND_COMPOUND_STATEMENT:
    print_compound_statement(printer, node);
    break;
  default:
    /* do nothing */
    break;
  }
}

static void print_variable_declaration(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  while (1) {
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    if (tree->children[index]->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
  }
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_space();
  print_type(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_variable_declaration_part(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_newline();
  ++printer->indent;
  while (index < tree->children_count) {
    print_indent(printer);
    print_variable_declaration(printer, tree->children[index++]);
    print_newline();
  }
  --printer->indent;
}

static void print_formal_parameter_section(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  while (1) {
    print_token(printer, tree->children[index++], printer->option.color.parameter);
    if (tree->children[index]->kind != SYNTAX_KIND_COMMA_TOKEN) {
      break;
    }
    print_token(printer, tree->children[index++], printer->option.color.foreground);
    print_space();
  }
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_space();
  print_type(printer, tree->children[index++]);
}

static void print_formal_parameter_list(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.foreground);
  while (tree->children[index]->kind == SYNTAX_KIND_FORMAL_PARAMETER_SECTION) {
    print_formal_parameter_section(printer, tree->children[index++]);
  }
  print_token(printer, tree->children[index++], printer->option.color.foreground);
}

static void print_procedure_declaration(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.procedure);
  if (tree->children[index]) {
    print_formal_parameter_list(printer, tree->children[index++]);
  } else {
    index++;
  }
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_newline();
  if (tree->children[index]) {
    print_indent(printer);
    print_variable_declaration_part(printer, tree->children[index++]);
  } else {
    index++;
  }
  print_indent(printer);
  print_compound_statement(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_newline();
}

static void print_program(Printer *printer, const TokenNode *node)
{
  const TokenTree *tree  = (TokenTree *) node;
  unsigned long    index = 0;

  print_token(printer, tree->children[index++], printer->option.color.keyword);
  print_space();
  print_token(printer, tree->children[index++], printer->option.color.program);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_newline();
  ++printer->indent;
  while (1) {
    if (tree->children[index]->kind == SYNTAX_KIND_VARIABLE_DECLARATION_PART) {
      print_indent(printer);
      print_variable_declaration_part(printer, tree->children[index++]);
    } else if (tree->children[index]->kind == SYNTAX_KIND_PROCEDURE_DECLARATION) {
      print_indent(printer);
      print_procedure_declaration(printer, tree->children[index++]);
    } else {
      break;
    }
  }
  --printer->indent;
  print_indent(printer);
  print_compound_statement(printer, tree->children[index++]);
  print_token(printer, tree->children[index++], printer->option.color.foreground);
  print_newline();
}

void mppl_pretty_print(const TokenNode *node, const PrinterOption *option)
{
  Printer printer;
  printer.indent = 0;
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
  print_program(&printer, node);
}
