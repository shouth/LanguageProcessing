#ifndef PARSER_H
#define PARSER_H

#include <stdarg.h>
#include <stdint.h>

#include "lexer.h"
#include "parser-dsl.h"

#define PARSE_SUCCESS 0
#define PARSE_FAILURE -1

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

    SIZE_OF_RULE
} rule_id_t;

typedef struct parser parser_t;

typedef int (parser_cb_t)(const parser_t *pa, va_list args);

struct parser {
    lexer_t lexer;
    parser_cb_t *on_success;
    parser_cb_t *on_failure;

    uint64_t expected_terminals;
};

int parser_init(parser_t *pa, const char *filename, parser_cb_t *on_success, parser_cb_t *on_failure);

void parser_free(parser_t *pa);

int parser_failure(parser_t *pa, ...);

int parser_success(parser_t *pa, ...);

MPPL_DECLARE_RULE(root)

#endif /* PARSER_H */