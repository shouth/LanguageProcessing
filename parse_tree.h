#ifndef RULE_H
#define RULE_H

#include "terminal.h"

typedef enum {
    RULE_PROGRAM,
    RULE_BLOCK,
    RULE_VARIABLE_DECLARATION,
    RULE_VARIABLE_NAMES,
    RULE_VARIABLE_NAME,
    RULE_TYPE,
    RULE_STANDARD_TYPE,
    RULE_ARRAY_TYPE,
    RULE_SUBPROGRAM_DECLARATION,
    RULE_PROCEDURE_NAME,
    RULE_FORMAL_PARAMETERS,
    RULE_COMPOUND_STATEMENT,
    RULE_STATEMENT,
    RULE_CONDITION_STATEMENT,
    RULE_ITERATION_STATEMENT,
    RULE_EXIT_STATEMENT,
    RULE_CALL_STATEMENT,
    RULE_EXPRESSIONS,
    RULE_RETURN_STATEMENT,
    RULE_ASSIGNMENT_STATEMENT,
    RULE_LEFT_PART,
    RULE_VARIABLE,
    RULE_EXPRESSION,
    RULE_SIMPLE_EXPRESSION,
    RULE_TERM,
    RULE_FACTOR,
    RULE_CONSTANT,
    RULE_MULTIPLICATIVE_OPERATOR,
    RULE_ADDITIVE_OPERATOR,
    RULE_RELATIONAL_OPERATOR,
    RULE_INPUT_STATEMENT,
    RULE_OUTPUT_STATEMENT,
    RULE_OUTPUT_FORMAT,
    RULE_EMPTY_STATEMENT,

    RULE_TERMINAL,
} rule_type_t;

typedef struct parse_tree parse_tree_t;
struct parse_tree {
    parse_tree_t *parent;
    rule_type_t type;

    union {
        struct {
            parse_tree_t *next;
            struct {
                parse_tree_t *front;
                parse_tree_t **back;
            } child;
        } stream;

        terminal_t terminal;
    };
};

parse_tree_t *parse_tree_new(rule_type_t type);

parse_tree_t *parse_tree_new_terminal(const terminal_t *terminal);

void parse_tree_free(parse_tree_t *stream);

void parse_tree_push(parse_tree_t *stream, parse_tree_t *child);

#endif /* TERMINAL_STREAM_H */
