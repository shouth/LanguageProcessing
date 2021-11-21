#include <assert.h>
#include <stdint.h>

#include "cursol.h"
#include "lexer.h"
#include "parser.h"
#include "rule_stream.h"
#include "terminal.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    terminal_type_t last_terminal;
    uint64_t expected_terminals;
} parser_t;

rule_stream_t *parse_terminal(parser_t *parser, terminal_type_t type);
rule_stream_t *parse_program(parser_t *parser);
rule_stream_t *parse_block(parser_t *parser);
rule_stream_t *parse_variable_declaration(parser_t *parser);
rule_stream_t *parse_variable_names(parser_t *parser);
rule_stream_t *parse_variable_name(parser_t *parser);
rule_stream_t *parse_type(parser_t *parser);
rule_stream_t *parse_standard_type(parser_t *parser);
rule_stream_t *parse_array_type(parser_t *parser);
rule_stream_t *parse_subprogram_declaration(parser_t *parser);
rule_stream_t *parse_procedure_name(parser_t *parser);
rule_stream_t *parse_formal_parameters(parser_t *parser);
rule_stream_t *parse_compound_statement(parser_t *parser);
rule_stream_t *parse_statement(parser_t *parser);
rule_stream_t *parse_condition_statement(parser_t *parser);
rule_stream_t *parse_iteration_statement(parser_t *parser);
rule_stream_t *parse_exit_statement(parser_t *parser);
rule_stream_t *parse_call_statement(parser_t *parser);
rule_stream_t *parse_expressions(parser_t *parser);
rule_stream_t *parse_return_statement(parser_t *parser);
rule_stream_t *parse_assignment_statement(parser_t *parser);
rule_stream_t *parse_left_part(parser_t *parser);
rule_stream_t *parse_variable(parser_t *parser);
rule_stream_t *parse_expression(parser_t *parser);
rule_stream_t *parse_simple_expression(parser_t *parser);
rule_stream_t *parse_term(parser_t *parser);
rule_stream_t *parse_factor(parser_t *parser);
rule_stream_t *parse_constant(parser_t *parser);
rule_stream_t *parse_multiplicative_operator(parser_t *parser);
rule_stream_t *parse_additive_operator(parser_t *parser);
rule_stream_t *parse_relational_operator(parser_t *parser);
rule_stream_t *parse_input_statement(parser_t *parser);
rule_stream_t *parse_output_statement(parser_t *parser);
rule_stream_t *parse_output_format(parser_t *parser);
rule_stream_t *parse_empty_statement(parser_t *parser);

void next_terminal(cursol_t *cursol, terminal_t *terminal)
{
    token_t token;

    assert(cursol != NULL && terminal != NULL);
    while (1) {
        lex(cursol, &token);
        terminal_from_token(&token, terminal);
        if (terminal->data.type != TERMINAL_NONE) {
            return;
        }
    }
}

rule_stream_t *parse_terminal(parser_t *parser, terminal_type_t type)
{
    terminal_t terminal;

    assert(parser != NULL);
    next_terminal(&parser->cursol, &terminal);
    parser->last_terminal = terminal.data.type;
    if (terminal.data.type != type) {
        parser->expected_terminals |= 1ull << type;
        return NULL;
    }
    return rule_stream_new_terminal(&terminal);
}

rule_stream_t *parse_program(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_PROGRAM);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_PROGRAM);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_block(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_DOT);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_block(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_BLOCK);

    assert(parser != NULL);

    while (1) {
        stream = parse_variable_declaration(parser);
        if (stream != NULL) {
            rule_stream_push(ret, stream);
            continue;
        }

        stream = parse_subprogram_declaration(parser);
        if (stream != NULL) {
            rule_stream_push(ret, stream);
            continue;
        }

        break;
    }

    stream = parse_compound_statement(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_variable_declaration(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_VARIABLE_DECLARATION);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_VAR);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_variable_names(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_COLON);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_type(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_variable_names(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_COLON);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_type(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_SEMI);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_variable_names(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_VARIABLE_NAMES);

    assert(parser != NULL);

    stream = parse_variable_name(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_COMMA);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_variable_name(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_variable_name(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_VARIABLE_NAME);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_type(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_TYPE);

    assert(parser != NULL);

    stream = parse_standard_type(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_array_type(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_standard_type(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_STANDARD_TYPE);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_INTEGER);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_BOOLEAN);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_CHAR);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_array_type(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_ARRAY_TYPE);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_ARRAY);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_LSQPAREN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_NUMBER);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_RSQPAREN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_OF);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_standard_type(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_subprogram_declaration(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_SUBPROGRAM_DECLARATION);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_PROCEDURE);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_procedure_name(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_formal_parameters(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_variable_declaration(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
    }

    stream = parse_compound_statement(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_procedure_name(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_PROCEDURE_NAME);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_formal_parameters(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_FORMAL_PARAMETERS);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_variable_names(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_COLON);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_type(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_variable_names(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_COLON);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_type(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_RPAREN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_compound_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_COMPOUND_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_BEGIN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_SEMI);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_statement(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_END);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_STATEMENT);

    assert(parser != NULL);

    stream = parse_assignment_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_condition_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_iteration_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_exit_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_call_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_return_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_input_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_output_statement(parser);
    if (stream != NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_condition_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_CONDITION_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_IF);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_THEN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    do {
        stream = parse_terminal(parser, TERMINAL_ELSE);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_statement(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    } while (0);

    return ret;
}

rule_stream_t *parse_iteration_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_ITERATION_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_WHILE);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_DO);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_exit_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_EXIT_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_BREAK);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_call_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_CALL_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_CALL);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_procedure_name(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    do {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_expressions(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    } while (0);

    return ret;
}

rule_stream_t *parse_expressions(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_EXPRESSIONS);

    assert(parser != NULL);

    stream = parse_expression(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_COMMA);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_return_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_RETURN_STATEMENT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_RETURN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_assignment_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_ASSIGNMENT_STATEMENT);

    assert(parser != NULL);

    stream = parse_left_part(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_ASSIGN);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_left_part(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_LEFT_PART);

    assert(parser != NULL);

    stream = parse_variable(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    return ret;
}

rule_stream_t *parse_variable(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_VARIABLE);

    assert(parser != NULL);

    stream = parse_variable_name(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    do {
        stream = parse_terminal(parser, TERMINAL_LSQPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RSQPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    } while (0);

    return ret;
}

rule_stream_t *parse_expression(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_EXPRESSION);

    assert(parser != NULL);

    stream = parse_simple_expression(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_relational_operator(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_simple_expression(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_simple_expression(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_SIMPLE_EXPRESSION);

    assert(parser != NULL);

    do {
        stream = parse_terminal(parser, TERMINAL_PLUS);
        if (stream != NULL) {
            rule_stream_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_MINUS);
        if (stream != NULL) {
            rule_stream_push(ret, stream);
            break;
        }
    } while (0);

    stream = parse_term(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_additive_operator(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_term(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_term(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_TERM);

    assert(parser != NULL);

    stream = parse_factor(parser);
    if (stream == NULL) {
        rule_stream_free(ret);
        return NULL; /* error */
    }
    rule_stream_push(ret, stream);

    while (1) {
        stream = parse_multiplicative_operator(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_factor(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    }

    return ret;
}

rule_stream_t *parse_factor(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_FACTOR);

    assert(parser != NULL);

    stream = parse_variable(parser);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_constant(parser);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    do {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        return ret;
    } while (0);

    do {
        stream = parse_terminal(parser, TERMINAL_NOT);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_factor(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        return ret;
    } while (0);

    do {
        stream = parse_standard_type(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL;
        }
        rule_stream_push(ret, stream);

        return ret;
    } while (0);

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_constant(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_CONSTANT);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_NUMBER);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_FALSE);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_TRUE);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_STRING);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_multiplicative_operator(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_MULTIPLICATIVE_OPERATOR);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_STAR);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_DIV);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_AND);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_additive_operator(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_ADDITIVE_OPERATOR);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_PLUS);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_MINUS);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_OR);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_relational_operator(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_RELATIONAL_OPERATOR);

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_EQUAL);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_NOTEQ);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_LE);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_LEEQ);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_GR);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_GREQ);
    if (stream == NULL) {
        rule_stream_push(ret, stream);
        return ret;
    }

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_input_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_INPUT_STATEMENT);

    assert(parser != NULL);

    do {
        stream = parse_terminal(parser, TERMINAL_READ);
        if (stream == NULL) {
            rule_stream_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_READLN);
        if (stream == NULL) {
            rule_stream_push(ret, stream);
            break;
        }

        rule_stream_free(ret);
        return NULL;
    } while (0);

    do {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_variable(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        while (1) {
            stream = parse_terminal(parser, TERMINAL_COMMA);
            if (stream == NULL) {
                break;
            }
            rule_stream_push(ret, stream);

            stream = parse_variable(parser);
            if (stream == NULL) {
                rule_stream_free(ret);
                return NULL; /* error */
            }
            rule_stream_push(ret, stream);
        }

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    } while (0);

    return ret;
}

rule_stream_t *parse_output_statement(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_OUTPUT_STATEMENT);

    assert(parser != NULL);

    do {
        stream = parse_terminal(parser, TERMINAL_WRITE);
        if (stream == NULL) {
            rule_stream_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_WRITELN);
        if (stream == NULL) {
            rule_stream_push(ret, stream);
            break;
        }

        rule_stream_free(ret);
        return NULL;
    } while (0);

    do {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        stream = parse_output_format(parser);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);

        while (1) {
            stream = parse_terminal(parser, TERMINAL_COMMA);
            if (stream == NULL) {
                break;
            }
            rule_stream_push(ret, stream);

            stream = parse_output_format(parser);
            if (stream == NULL) {
                rule_stream_free(ret);
                return NULL; /* error */
            }
            rule_stream_push(ret, stream);
        }

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            rule_stream_free(ret);
            return NULL; /* error */
        }
        rule_stream_push(ret, stream);
    } while (0);

    return ret;
}

rule_stream_t *parse_output_format(parser_t *parser)
{
    rule_stream_t *stream = NULL;
    rule_stream_t *ret = rule_stream_new(RULE_STREAM_OUTPUT_FORMAT);

    cursol_t cursol = parser->cursol;
    size_t len;
    const char *ptr;

    assert(parser != NULL);

    stream = parse_terminal(parser, TERMINAL_STRING);
    if (stream != NULL) {
        len = stream->terminal.data.string.len;
        ptr = stream->terminal.data.string.ptr;
        if (len == 1 || (len == 2 && ptr[0] == '\'' && ptr[1] == '\'')) {
            parser->cursol = cursol;
        } else {
            rule_stream_push(ret, stream);
            return ret;
        }
    }

    do {
        stream = parse_expression(parser);
        if (stream == NULL) {
            break;
        }
        rule_stream_push(ret, stream);

        do {
            stream = parse_terminal(parser, TERMINAL_COLON);
            if (stream == NULL) {
                break;
            }
            rule_stream_push(ret, stream);

            stream = parse_terminal(parser, TERMINAL_NUMBER);
            if (stream == NULL) {
                rule_stream_free(ret);
                return NULL; /* error */
            }
            rule_stream_push(ret, stream);
        } while (0);

        return ret;
    } while (0);

    rule_stream_free(ret);
    return NULL;
}

rule_stream_t *parse_empty_statement(parser_t *parser)
{
    assert(parser != NULL);
    return rule_stream_new(RULE_STREAM_EMPTY_STATEMENT);
}

rule_stream_t *parse(const source_t *src)
{
    parser_t parser;

    assert(src != NULL);
    parser.src = src;
    parser.expected_terminals = 0;
    cursol_init(&parser.cursol, src->src_ptr, src->src_size);
    return parse_program(&parser);
}
