#include <assert.h>
#include <stdio.h>

#include "parse_tree.h"
#include "pretty_printer.h"

typedef struct {
    const parse_tree_t *top, *current;
    size_t indentation_level;
} pretty_printer_t;

void next_rule(pretty_printer_t *printer)
{
    assert(printer != NULL);
    assert(printer->current != NULL);
    printer->current = printer->current->next;
}

void enter_rule(pretty_printer_t *printer)
{
    assert(printer != NULL);
    assert(printer->current != NULL);
    printer->current = printer->current->data.stream.child.front;
}

void exit_rule(pretty_printer_t *printer)
{
    assert(printer != NULL);
    assert(printer->current != NULL);
    printer->current = printer->current->parent;
}

void pretty_print_terminal(pretty_printer_t *printer)
{
    assert(printer != NULL);
    assert(printer->current != NULL);
    fflush(stdout);
    assert(printer->current->type == RULE_TERMINAL);
    printf("%.*s", (int) printer->current->data.terminal.len, printer->current->data.terminal.ptr);
    next_rule(printer);
}

void print_space()
{
    printf(" ");
}

void pretty_print_program(pretty_printer_t *printer);
void pretty_print_block(pretty_printer_t *printer);
void pretty_print_variable_declaration(pretty_printer_t *printer);
void pretty_print_variable_names(pretty_printer_t *printer);
void pretty_print_variable_name(pretty_printer_t *printer);
void pretty_print_type(pretty_printer_t *printer);
void pretty_print_standard_type(pretty_printer_t *printer);
void pretty_print_array_type(pretty_printer_t *printer);
void pretty_print_subprogram_declaration(pretty_printer_t *printer);
void pretty_print_procedure_name(pretty_printer_t *printer);
void pretty_print_formal_parameters(pretty_printer_t *printer);
void pretty_print_compound_statement(pretty_printer_t *printer);
void pretty_print_statement(pretty_printer_t *printer);
void pretty_print_condition_statement(pretty_printer_t *printer);
void pretty_print_iteration_statement(pretty_printer_t *printer);
void pretty_print_exit_statement(pretty_printer_t *printer);
void pretty_print_call_statement(pretty_printer_t *printer);
void pretty_print_expressions(pretty_printer_t *printer);
void pretty_print_return_statement(pretty_printer_t *printer);
void pretty_print_assignment_statement(pretty_printer_t *printer);
void pretty_print_left_part(pretty_printer_t *printer);
void pretty_print_variable(pretty_printer_t *printer);
void pretty_print_expression(pretty_printer_t *printer);
void pretty_print_simple_expression(pretty_printer_t *printer);
void pretty_print_term(pretty_printer_t *printer);
void pretty_print_factor(pretty_printer_t *printer);
void pretty_print_constant(pretty_printer_t *printer);
void pretty_print_multiplicative_operator(pretty_printer_t *printer);
void pretty_print_additive_operator(pretty_printer_t *printer);
void pretty_print_relational_operator(pretty_printer_t *printer);
void pretty_print_input_statement(pretty_printer_t *printer);
void pretty_print_output_statement(pretty_printer_t *printer);
void pretty_print_output_format(pretty_printer_t *printer);
void pretty_print_empty_statement(pretty_printer_t *printer);

void pretty_print_program(pretty_printer_t *printer)
{
    assert(printer->current->type == RULE_PROGRAM);
    enter_rule(printer);
    pretty_print_terminal(printer);
    print_space();
    pretty_print_terminal(printer);
    pretty_print_terminal(printer);
    pretty_print_block(printer);
    pretty_print_terminal(printer);
    exit_rule(printer);
}

void pretty_print_block(pretty_printer_t *printer)
{
    assert(printer->current->type == RULE_BLOCK);
    enter_rule(printer);
    while (1) {
        if (printer->current->type == RULE_VARIABLE_DECLARATION) {
            pretty_print_variable_declaration(printer);
            continue;
        }
        if (printer->current->type == RULE_SUBPROGRAM_DECLARATION) {
            pretty_print_subprogram_declaration(printer);
            continue;
        }
        break;
    }
    pretty_print_compound_statement(printer);
    exit_rule(printer);
}

void pretty_print_variable_declaration(pretty_printer_t *printer)
{
    assert(printer->current->type == RULE_VARIABLE_DECLARATION);
    next_rule(printer);
}

void pretty_print_variable_names(pretty_printer_t *printer);
void pretty_print_variable_name(pretty_printer_t *printer);
void pretty_print_type(pretty_printer_t *printer);
void pretty_print_standard_type(pretty_printer_t *printer);
void pretty_print_array_type(pretty_printer_t *printer);

void pretty_print_subprogram_declaration(pretty_printer_t *printer)
{
    assert(printer->current->type == RULE_SUBPROGRAM_DECLARATION);
    next_rule(printer);
}

void pretty_print_procedure_name(pretty_printer_t *printer);
void pretty_print_formal_parameters(pretty_printer_t *printer);

void pretty_print_compound_statement(pretty_printer_t *printer)
{
    assert(printer->current->type == RULE_COMPOUND_STATEMENT);
    enter_rule(printer);
    pretty_print_terminal(printer);

    /*  */

    pretty_print_terminal(printer);
    exit_rule(printer);
}

void pretty_print_statement(pretty_printer_t *printer);
void pretty_print_condition_statement(pretty_printer_t *printer);
void pretty_print_iteration_statement(pretty_printer_t *printer);
void pretty_print_exit_statement(pretty_printer_t *printer);
void pretty_print_call_statement(pretty_printer_t *printer);
void pretty_print_expressions(pretty_printer_t *printer);
void pretty_print_return_statement(pretty_printer_t *printer);
void pretty_print_assignment_statement(pretty_printer_t *printer);
void pretty_print_left_part(pretty_printer_t *printer);
void pretty_print_variable(pretty_printer_t *printer);
void pretty_print_expression(pretty_printer_t *printer);
void pretty_print_simple_expression(pretty_printer_t *printer);
void pretty_print_term(pretty_printer_t *printer);
void pretty_print_factor(pretty_printer_t *printer);
void pretty_print_constant(pretty_printer_t *printer);
void pretty_print_multiplicative_operator(pretty_printer_t *printer);
void pretty_print_additive_operator(pretty_printer_t *printer);
void pretty_print_relational_operator(pretty_printer_t *printer);
void pretty_print_input_statement(pretty_printer_t *printer);
void pretty_print_output_statement(pretty_printer_t *printer);
void pretty_print_output_format(pretty_printer_t *printer);
void pretty_print_empty_statement(pretty_printer_t *printer);

void pretty_print(const parse_tree_t *tree)
{
    pretty_printer_t printer;
    printer.top = printer.current = tree;
    printer.indentation_level = 0;
    pretty_print_program(&printer);
}
