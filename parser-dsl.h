#ifndef PARSER_DSL_H
#define PARSER_DSL_H

#include "parser.h"
#include "macro.h"

/*
    These macros assume that the constructing grammer is LL(1).
    Rules have two types of failure: local failure and global failure.
    If the rule is the first one of its parent rule and fails, it is a local failure.
    A local failure is not fatal: if other candidates remain, the parser automatically
    switches to them. Otherwise, local failure propagates to parent rules until examining
    rule is not the first one of its parent rule, and then it becomes a global failure.
    When a global failure happens, the parser raises an error and stops processing.
 */

#define MPPL_DSL_FAILURE -1
#define MPPL_DSL_SUCCESS 0

#define MPPL_DSL_CALLBACK_ON_SUCCESS() \
    parser_success(pa, parsing_rule);

#define MPPL_DSL_CALLBACK_ON_FAILURE() \
    parser_failure(pa, parsing_rule);

#define MPPL_DSL_RETURN(x) \
    parse_status = x; \
    break;

#define MPPL_DSL_IS_STATUS(x) \
    parse_status == x

#define MPPL_DSL_IS_RULE(x) \
    BOOL(MPPL_IS_RULE_##x)

#define MPPL_DSL_EXPAND(x) \
    IF(MPPL_DSL_IS_RULE(x))( \
        parse_status = mppl_rule_##x(pa);, \
        MPPL_EXPAND_##x \
    )

#define MPPL_DSL_EARLY_RETURN(rule, cond, ret) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS(cond)) { \
        MPPL_DSL_RETURN(ret) \
    }

#define MPPL_DSL_SUCCESS_ON_SUCCESS(rule) \
    MPPL_DSL_EARLY_RETURN(rule, MPPL_DSL_SUCCESS, MPPL_DSL_SUCCESS)

#define MPPL_DSL_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EARLY_RETURN(rule, MPPL_DSL_FAILURE, MPPL_DSL_FAILURE)

#define IMPL_MPPL_EXPAND_MPPL_ALTERNATE(rules) \
    do { \
        INVOKE_ALL(MPPL_DSL_SUCCESS_ON_SUCCESS, rules) \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    } while (0);
#define IMPL_MPPL_EXPAND_MPPL_ALTERNATE_INDIRECT() \
    IMPL_MPPL_EXPAND_MPPL_ALTERNATE
#define MPPL_EXPAND_MPPL_ALTERNATE(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_ALTERNATE_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_MPPL_REPEAT(rules) \
    while (1) { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), MPPL_DSL_FAILURE, MPPL_DSL_SUCCESS) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
    }
#define IMPL_MPPL_EXPAND_MPPL_REPEAT_INDIRECT() \
    IMPL_MPPL_EXPAND_MPPL_REPEAT
#define MPPL_EXPAND_MPPL_REPEAT(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_REPEAT_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_MPPL_OPTION(rules) \
    do { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), MPPL_DSL_FAILURE, MPPL_DSL_SUCCESS) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
    } while (0);
#define IMPL_MPPL_EXPAND_MPPL_OPTION_INDIRECT() \
    IMPL_MPPL_EXPAND_MPPL_OPTION
#define MPPL_EXPAND_MPPL_OPTION(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_MPPL_OPTION_INDIRECT) () (rules)

#define MPPL_DSL_RULE_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS(MPPL_DSL_FAILURE)) { \
        MPPL_DSL_CALLBACK_ON_FAILURE() \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    }

#define MPPL_DECLARE_RULE(name) \
    int mppl_rule_##name(parser_t *pa);

#define MPPL_DEFINE_RULE(name, val, rules) \
    int mppl_rule_##name(parser_t *pa) { \
        const int parsing_rule = val; \
        int parse_status; \
        do { \
            EVAL(MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), MPPL_DSL_FAILURE, MPPL_DSL_FAILURE)) \
            EVAL(INVOKE_ALL(MPPL_DSL_RULE_FAILURE_ON_FAILURE, LIST_TAIL(rules))) \
        } while (0); \
        if (MPPL_DSL_IS_STATUS(MPPL_DSL_SUCCESS)) { \
            MPPL_DSL_CALLBACK_ON_SUCCESS() \
            pa->expected_terminals = 0; \
        } \
        return parse_status; \
    }

#define MPPL_EXPAND_MPPL_TERMINAL(name) \
    parse_status = mppl_terminal_##name(pa);

#define MPPL_DECLARE_TERMINAL(name) \
    int mppl_terminal_##name(parser_t *pa);

#define MPPL_DEFINE_TERMINAL(name, val) \
    int mppl_terminal_##name(parser_t *pa) { \
        const int parsing_rule = val; \
        if (lexer_lookahead(&pa->lexer) != parsing_rule) { \
            pa->expected_terminals |= 1ull << val; \
            return MPPL_DSL_FAILURE; \
        } \
        MPPL_DSL_CALLBACK_ON_SUCCESS() \
        lexer_next(&pa->lexer); \
        pa->expected_terminals = 0; \
        return MPPL_DSL_SUCCESS; \
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

#endif /* PARSER_DSL_H */