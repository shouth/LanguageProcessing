#include "parser.h"
#include "lexer.h"
#include "scanner.h"
#include "message.h"
#include "macro.h"

#define MPPL_DSL_FAILURE PARSE_FAILURE
#define MPPL_DSL_SUCCESS PARSE_SUCCESS

#define MPPL_DSL_CALLBACK_ON_SUCCESS() \
    parser_success(pa, parsing_rule);

#define MPPL_DSL_CALLBACK_ON_FAILURE() \
    parser_failure(pa, parsing_rule);

#define MPPL_DSL_RETURN(x) \
    parse_status = x; \
    break;

#define MPPL_DSL_IS_STATUS_SUCCESS() \
    parse_status != MPPL_DSL_FAILURE

#define MPPL_DSL_IS_STATUS_FAILURE() \
    parse_status == MPPL_DSL_FAILURE

#define MPPL_DSL_IS_RULE(x) \
    BOOL(MPPL_IS_RULE_##x)

#define MPPL_DSL_EXPAND(x) \
    IF(MPPL_DSL_IS_RULE(x))( \
        parse_status = mppl_parse_##x(pa);, \
        MPPL_EXPAND_##x \
    )

#define MPPL_DSL_EARLY_RETURN(rule, cond, ret) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS_##cond()) { \
        MPPL_DSL_RETURN(ret) \
    }

#define MPPL_DSL_SUCCESS_ON_SUCCESS(rule) \
    MPPL_DSL_EARLY_RETURN(rule, SUCCESS, parsing_rule)

#define MPPL_DSL_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EARLY_RETURN(rule, FAILURE, MPPL_DSL_FAILURE)

#define IMPL_MPPL_EXPAND_MPPL_ALTERNATE(rules) \
    do { \
        INVOKE_ALL(MPPL_DSL_SUCCESS_ON_SUCCESS, rules) \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    } while (0);
#define IMPL_MPPL_EXPAND_MPPL_ALTERNATE_INDIRECT() IMPL_MPPL_EXPAND_MPPL_ALTERNATE
#define MPPL_EXPAND_MPPL_ALTERNATE(rules) OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_ALTERNATE_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_MPPL_REPEAT(rules) \
    while (1) { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), FAILURE, parsing_rule) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
    }
#define IMPL_MPPL_EXPAND_MPPL_REPEAT_INDIRECT() IMPL_MPPL_EXPAND_MPPL_REPEAT
#define MPPL_EXPAND_MPPL_REPEAT(rules) OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_REPEAT_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_MPPL_OPTION(rules) \
    do { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), FAILURE, parsing_rule) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
    } while (0);
#define IMPL_MPPL_EXPAND_MPPL_OPTION_INDIRECT() IMPL_MPPL_EXPAND_MPPL_OPTION
#define MPPL_EXPAND_MPPL_OPTION(rules) OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_OPTION_INDIRECT) () (rules)

#define MPPL_DSL_RULE_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS_FAILURE()) { \
        MPPL_DSL_CALLBACK_ON_FAILURE() \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    }

#define MPPL_DEFINE_RULE(name, val, rules) \
    int mppl_parse_##name(parser_t *pa) { \
        const int parsing_rule = val; \
        int parse_status; \
        do { \
            EVAL(MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), FAILURE, MPPL_DSL_FAILURE)) \
            EVAL(INVOKE_ALL(MPPL_DSL_RULE_FAILURE_ON_FAILURE, LIST_TAIL(rules))) \
        } while (0); \
        if (MPPL_DSL_IS_STATUS_SUCCESS()) { \
            MPPL_DSL_CALLBACK_ON_SUCCESS() \
        } \
        return parse_status; \
    }

#define MPPL_EXPAND_MPPL_TERMINAL(name) \
    parse_status = mppl_parse_terminal_##name(pa);

#define MPPL_DEFINE_TERMINAL(name, val) \
    int mppl_parse_terminal_##name(parser_t *pa) { \
        const int parsing_rule = val; \
        if (lexer_lookahead(&pa->lexer) == parsing_rule) { \
            return PARSE_FAILURE; \
        } \
        MPPL_DSL_CALLBACK_ON_SUCCESS() \
        lexer_next(&pa->lexer); \
        return PARSE_SUCCESS; \
    }

#define MPPL_EXPAND_MPPL_MANUAL(code) \
    do { \
        code \
    } while (0); \

#define MPPL_IS_RULE_MPPL_ALTERNATE(_) 0
#define MPPL_IS_RULE_MPPL_REPEAT(_) 0
#define MPPL_IS_RULE_MPPL_OPTION(_) 0
#define MPPL_IS_RULE_MPPL_TERMINAL(_) 0
#define MPPL_IS_RULE_MPPL_MANUAL(_) 0

#define MPPL_ALTERNATE(_) MPPL_ALTERNATE(_)
#define MPPL_REPEAT(_) MPPL_REPEAT(_)
#define MPPL_OPTION(_) MPPL_OPTION(_)
#define MPPL_TERMINAL(_) MPPL_TERMINAL(_)
#define MPPL_MANUAL(_) MPPL_MANUAL(_)

MPPL_DEFINE_TERMINAL(program, TPROGRAM)
MPPL_DEFINE_TERMINAL(name, TNAME)
MPPL_DEFINE_TERMINAL(semicolon, TSEMI)
MPPL_DEFINE_TERMINAL(dot, TDOT)

MPPL_DEFINE_RULE(
    output_statement,
    RULE_OUTPUT_STATEMENT,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(write))
        (MPPL_TERMINAL(writeln))
    ))
    (MPPL_OPTION(
        (MPPL_TERMINAL(left_parenthesis))
        (output_format)
        (MPPL_REPEAT(
            (MPPL_TERMINAL(comma))
            (output_format)
        ))
        (MPPL_TERMINAL(right_parenthesis))
    ))
)

MPPL_DEFINE_RULE(
    block,
    RULE_BLOCK,

    (MPPL_REPEAT(
        (MPPL_ALTERNATE(
            (variable_declaration)
            (subprogram_declaration)
        ))
    ))
    (compound_statement)
)

MPPL_DEFINE_RULE(
    program,
    RULE_PROGRAM,

    (MPPL_TERMINAL(program))
    (MPPL_TERMINAL(name))
    (MPPL_TERMINAL(semicolon))
    (block)
    (MPPL_TERMINAL(dot))
)

int parser_init(parser_t *pa, const char *filename, parser_cb_t *on_success, parser_cb_t *on_failure)
{
    lexer_init(&pa->lexer, filename);
    pa->on_success = on_success;
    pa->on_failure = on_failure;
}

void parser_free(parser_t *pa)
{
    lexer_free(&pa->lexer);
}

int parser_success(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    ret = pa->on_success(pa->lexer, args);
    va_end(args);
    return ret;
}

int parser_failure(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    ret = pa->on_failure(pa->lexer, args);
    va_end(args);
    return ret;
}
