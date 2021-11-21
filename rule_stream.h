#ifndef RULE_STREAM_H
#define RULE_STREAM_H

#include "terminal.h"

typedef enum {
    RULE_STREAM_PROGRAM,
    RULE_STREAM_BLOCK,
    RULE_STREAM_VARIABLE_DECLARATION,
    RULE_STREAM_VARIABLE_NAMES,
    RULE_STREAM_VARIABLE_NAME,
    RULE_STREAM_TYPE,
    RULE_STREAM_STANDARD_TYPE,
    RULE_STREAM_SUBPROGRAM_DECLARATION,
    RULE_STREAM_PROCEDURE_NAME,
    RULE_STREAM_FORMAL_PARAMETERS,
    RULE_STREAM_COMPOUND_STATEMENT,
    RULE_STREAM_STATEMENT,
    RULE_STREAM_CONDITION_STATEMENT,
    RULE_STREAM_ITERATION_STATEMENT,
    RULE_STREAM_EXIT_STATEMENT,
    RULE_STREAM_CALL_STATEMENT,
    RULE_STREAM_EXPRESSIONS,
    RULE_STREAM_RETURN_STATEMENT,
    RULE_STREAM_ASSIGNMENT_STATEMENT,
    RULE_STREAM_LEFT_PART,
    RULE_STREAM_VARIABLE,
    RULE_STREAM_EXPRESSION,
    RULE_STREAM_SIMPLE_EXPRESSION,
    RULE_STREAM_TERM,
    RULE_STREAM_FACTOR,
    RULE_STREAM_CONSTANT,
    RULE_STREAM_MULTIPLICATIVE_OPERATOR,
    RULE_STREAM_ADDITIVE_OPERATOR,
    RULE_STREAM_RELATIONAL_OPERATOR,
    RULE_STREAM_INPUT_STATEMENT,
    RULE_STREAM_OUTPUT_STATEMENT,
    RULE_STREAM_OUPUT_FORMAT,
    RULE_STREAM_EMPTY_STATEMENT,

    RULE_STREAM_TERMINAL,
} rule_stream_type_t;

typedef struct rule_stream rule_stream_t;
struct rule_stream {
    rule_stream_t *parent;
    rule_stream_type_t type;

    union {
        struct {
            rule_stream_t *next;
            struct {
                rule_stream_t *front;
                rule_stream_t **back;
            } child;
        } stream;

        terminal_t terminal;
    };
};

rule_stream_t *rule_stream_new(rule_stream_type_t type);

rule_stream_t *rule_stream_new_terminal(const terminal_t *terminal);

void rule_stream_free(rule_stream_t *stream);

void rule_stream_push(rule_stream_t *stream, rule_stream_t *child);

#endif /* TERMINAL_STREAM_H */
