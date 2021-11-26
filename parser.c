#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "cursol.h"
#include "lexer.h"
#include "parser.h"
#include "parse_tree.h"
#include "terminal.h"
#include "message.h"
#include "util.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    terminal_t last_terminal, current_terminal;
    uint64_t expected_terminals;
} parser_t;

parse_tree_t *parse_terminal(parser_t *parser, terminal_type_t type);
parse_tree_t *parse_program(parser_t *parser);
parse_tree_t *parse_block(parser_t *parser);
parse_tree_t *parse_variable_declaration(parser_t *parser);
parse_tree_t *parse_variable_names(parser_t *parser);
parse_tree_t *parse_variable_name(parser_t *parser);
parse_tree_t *parse_type(parser_t *parser);
parse_tree_t *parse_standard_type(parser_t *parser);
parse_tree_t *parse_array_type(parser_t *parser);
parse_tree_t *parse_subprogram_declaration(parser_t *parser);
parse_tree_t *parse_procedure_name(parser_t *parser);
parse_tree_t *parse_formal_parameters(parser_t *parser);
parse_tree_t *parse_compound_statement(parser_t *parser);
parse_tree_t *parse_statement(parser_t *parser);
parse_tree_t *parse_condition_statement(parser_t *parser);
parse_tree_t *parse_iteration_statement(parser_t *parser);
parse_tree_t *parse_exit_statement(parser_t *parser);
parse_tree_t *parse_call_statement(parser_t *parser);
parse_tree_t *parse_expressions(parser_t *parser);
parse_tree_t *parse_return_statement(parser_t *parser);
parse_tree_t *parse_assignment_statement(parser_t *parser);
parse_tree_t *parse_left_part(parser_t *parser);
parse_tree_t *parse_variable(parser_t *parser);
parse_tree_t *parse_expression(parser_t *parser);
parse_tree_t *parse_simple_expression(parser_t *parser);
parse_tree_t *parse_term(parser_t *parser);
parse_tree_t *parse_factor(parser_t *parser);
parse_tree_t *parse_constant(parser_t *parser);
parse_tree_t *parse_multiplicative_operator(parser_t *parser);
parse_tree_t *parse_additive_operator(parser_t *parser);
parse_tree_t *parse_relational_operator(parser_t *parser);
parse_tree_t *parse_input_statement(parser_t *parser);
parse_tree_t *parse_output_statement(parser_t *parser);
parse_tree_t *parse_output_format(parser_t *parser);
parse_tree_t *parse_empty_statement(parser_t *parser);

void next_terminal(parser_t *parser)
{
    token_t token;

    assert(parser != NULL);
    parser->last_terminal = parser->current_terminal;
    parser->expected_terminals = 0;
    while (1) {
        lex(&parser->cursol, &token);
        terminal_from_token(&parser->current_terminal, &token);
        if (parser->current_terminal.type != TERMINAL_NONE) {
            return;
        }
    }
}

void error_unexpected_terminal(parser_t *parser)
{
    msg_t *msg;
    size_t pos, len;
    terminal_type_t i;
    char buf[256], *ptr;
    uint64_t msb, bit;

    assert(parser != NULL);
    assert(parser->expected_terminals != 0);

    if (parser->expected_terminals == (uint64_t) 1 << TERMINAL_SEMI) {
        pos = parser->last_terminal.pos + parser->last_terminal.len;
        len = 1;
    } else {
        pos = parser->current_terminal.pos;
        len = parser->current_terminal.len;
    }

    ptr = buf;
    msb = msb64(parser->expected_terminals);
    for (i = 0; i <= TERMINAL_EOF; i++) {
        if (bit = ((uint64_t) 1 << i) & parser->expected_terminals) {
            if (ptr != buf) {
                ptr += sprintf(ptr, bit != msb ? ", " : " or ");
            }
            ptr += sprintf(ptr, "`%s`", terminal_to_str(i));
        }
    }

    msg = msg_new(parser->src, pos, len,
        MSG_ERROR, "expected %s, got `%.*s`", buf,
        (int) parser->current_terminal.len, parser->current_terminal.ptr);
    if (parser->expected_terminals == (uint64_t) 1 << TERMINAL_SEMI) {
        msg_add_inline_entry(msg, pos, len, "insert `;` here");
        msg_add_inline_entry(msg, parser->current_terminal.pos, parser->current_terminal.len, "unexpected token");
    }
    msg_emit(msg);
    exit(1);
}

parse_tree_t *parse_terminal(parser_t *parser, terminal_type_t type)
{
    assert(parser != NULL);
    if (parser->current_terminal.type != type) {
        parser->expected_terminals |= (uint64_t) 1 << type;
        return NULL;
    }
    next_terminal(parser);
    return parse_tree_new_terminal(&parser->last_terminal);
}

parse_tree_t *parse_program(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_PROGRAM);

    stream = parse_terminal(parser, TERMINAL_PROGRAM);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_block(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_DOT);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_block(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_BLOCK);

    while (1) {
        stream = parse_variable_declaration(parser);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            continue;
        }

        stream = parse_subprogram_declaration(parser);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            continue;
        }

        break;
    }

    stream = parse_compound_statement(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_variable_declaration(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_VARIABLE_DECLARATION);

    stream = parse_terminal(parser, TERMINAL_VAR);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_variable_names(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_COLON);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_type(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_variable_names(parser);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_COLON);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_type(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_SEMI);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_variable_names(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_VARIABLE_NAMES);

    stream = parse_variable_name(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_COMMA);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_variable_name(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_variable_name(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_VARIABLE_NAME);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_type(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_TYPE);

    stream = parse_standard_type(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_array_type(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_standard_type(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_STANDARD_TYPE);

    stream = parse_terminal(parser, TERMINAL_INTEGER);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_BOOLEAN);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_CHAR);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_array_type(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_ARRAY_TYPE);

    stream = parse_terminal(parser, TERMINAL_ARRAY);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_LSQPAREN);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_NUMBER);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_RSQPAREN);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_OF);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_standard_type(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_subprogram_declaration(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_SUBPROGRAM_DECLARATION);

    stream = parse_terminal(parser, TERMINAL_PROCEDURE);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_procedure_name(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_formal_parameters(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_variable_declaration(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
    }

    stream = parse_compound_statement(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_SEMI);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_procedure_name(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_PROCEDURE_NAME);

    stream = parse_terminal(parser, TERMINAL_NAME);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_formal_parameters(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_FORMAL_PARAMETERS);

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_variable_names(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_COLON);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_type(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_variable_names(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_COLON);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_type(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_RPAREN);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_compound_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_COMPOUND_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_BEGIN);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_SEMI);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_statement(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    stream = parse_terminal(parser, TERMINAL_END);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_STATEMENT);

    stream = parse_assignment_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_condition_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_iteration_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_exit_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_call_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_return_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_input_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_output_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_compound_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_empty_statement(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_condition_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_CONDITION_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_IF);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_THEN);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_ELSE);
    if (stream == NULL) {
        parse_tree_push(ret, stream);

        stream = parse_statement(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_iteration_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_ITERATION_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_WHILE);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_DO);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_statement(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_exit_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_EXIT_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_BREAK);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_call_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_CALL_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_CALL);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_procedure_name(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream == NULL) {
        parse_tree_push(ret, stream);

        stream = parse_expressions(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_expressions(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_EXPRESSIONS);

    stream = parse_expression(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_terminal(parser, TERMINAL_COMMA);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_return_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_RETURN_STATEMENT);

    stream = parse_terminal(parser, TERMINAL_RETURN);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_assignment_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_ASSIGNMENT_STATEMENT);

    stream = parse_left_part(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_ASSIGN);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_expression(parser);
    if (stream == NULL) {
        error_unexpected_terminal(parser);
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_left_part(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_LEFT_PART);

    stream = parse_variable(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    return ret;
}

parse_tree_t *parse_variable(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_VARIABLE);

    stream = parse_variable_name(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    stream = parse_terminal(parser, TERMINAL_LSQPAREN);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RSQPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_expression(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_EXPRESSION);

    stream = parse_simple_expression(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_relational_operator(parser);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_simple_expression(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_simple_expression(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_SIMPLE_EXPRESSION);

    do {
        stream = parse_terminal(parser, TERMINAL_PLUS);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_MINUS);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }
    } while (0);

    stream = parse_term(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_additive_operator(parser);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_term(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_term(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_TERM);

    stream = parse_factor(parser);
    if (stream == NULL) {
        parse_tree_free(ret);
        return NULL; /* error */
    }
    parse_tree_push(ret, stream);

    while (1) {
        stream = parse_multiplicative_operator(parser);
        if (stream == NULL) {
            break;
        }
        parse_tree_push(ret, stream);

        stream = parse_factor(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_factor(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_FACTOR);

    assert(parser != NULL);

    stream = parse_variable(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_constant(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_NOT);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_factor(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        return ret;
    }

    stream = parse_standard_type(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_LPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        stream = parse_expression(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL;
        }
        parse_tree_push(ret, stream);

        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_constant(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_CONSTANT);

    stream = parse_terminal(parser, TERMINAL_NUMBER);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_FALSE);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_TRUE);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_STRING);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_multiplicative_operator(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_MULTIPLICATIVE_OPERATOR);

    stream = parse_terminal(parser, TERMINAL_STAR);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_DIV);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_AND);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_additive_operator(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_ADDITIVE_OPERATOR);

    stream = parse_terminal(parser, TERMINAL_PLUS);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_MINUS);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_OR);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_relational_operator(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_RELATIONAL_OPERATOR);

    stream = parse_terminal(parser, TERMINAL_EQUAL);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_NOTEQ);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_LE);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_LEEQ);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_GR);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_terminal(parser, TERMINAL_GREQ);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_input_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_INPUT_STATEMENT);

    do {
        stream = parse_terminal(parser, TERMINAL_READ);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_READLN);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }

        parse_tree_free(ret);
        return NULL;
    } while (0);

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_variable(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        while (1) {
            stream = parse_terminal(parser, TERMINAL_COMMA);
            if (stream == NULL) {
                break;
            }
            parse_tree_push(ret, stream);

            stream = parse_variable(parser);
            if (stream == NULL) {
                error_unexpected_terminal(parser);
                parse_tree_free(ret);
                return NULL; /* error */
            }
            parse_tree_push(ret, stream);
        }

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_output_statement(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    assert(parser != NULL);

    ret = parse_tree_new(RULE_OUTPUT_STATEMENT);

    do {
        stream = parse_terminal(parser, TERMINAL_WRITE);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }

        stream = parse_terminal(parser, TERMINAL_WRITELN);
        if (stream != NULL) {
            parse_tree_push(ret, stream);
            break;
        }

        parse_tree_free(ret);
        return NULL;
    } while (0);

    stream = parse_terminal(parser, TERMINAL_LPAREN);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_output_format(parser);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);

        while (1) {
            stream = parse_terminal(parser, TERMINAL_COMMA);
            if (stream == NULL) {
                break;
            }
            parse_tree_push(ret, stream);

            stream = parse_output_format(parser);
            if (stream == NULL) {
                error_unexpected_terminal(parser);
                parse_tree_free(ret);
                return NULL; /* error */
            }
            parse_tree_push(ret, stream);
        }

        stream = parse_terminal(parser, TERMINAL_RPAREN);
        if (stream == NULL) {
            error_unexpected_terminal(parser);
            parse_tree_free(ret);
            return NULL; /* error */
        }
        parse_tree_push(ret, stream);
    }

    return ret;
}

parse_tree_t *parse_output_format(parser_t *parser)
{
    parse_tree_t *stream;
    parse_tree_t *ret;

    cursol_t cursol;
    size_t len;
    const char *ptr;

    assert(parser != NULL);

    cursol = parser->cursol;
    ret = parse_tree_new(RULE_OUTPUT_FORMAT);

    stream = parse_terminal(parser, TERMINAL_STRING);
    if (stream != NULL) {
        parse_tree_push(ret, stream);
        return ret;
    }

    stream = parse_expression(parser);
    if (stream != NULL) {
        parse_tree_push(ret, stream);

        stream = parse_terminal(parser, TERMINAL_COLON);
        if (stream != NULL) {
            parse_tree_push(ret, stream);

            stream = parse_terminal(parser, TERMINAL_NUMBER);
            if (stream == NULL) {
                error_unexpected_terminal(parser);
                parse_tree_free(ret);
                return NULL; /* error */
            }
            parse_tree_push(ret, stream);
        }

        return ret;
    }

    parse_tree_free(ret);
    return NULL;
}

parse_tree_t *parse_empty_statement(parser_t *parser)
{
    assert(parser != NULL);
    return parse_tree_new(RULE_EMPTY_STATEMENT);
}

parse_tree_t *parse(const source_t *src)
{
    parser_t parser;

    assert(src != NULL);
    parser.src = src;
    cursol_init(&parser.cursol, src, src->src_ptr, src->src_size);
    next_terminal(&parser);
    return parse_program(&parser);
}
