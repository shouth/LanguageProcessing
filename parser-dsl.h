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

#define MPPL_DSL_ENTER   PARSE_ENTER
#define MPPL_DSL_FAILURE PARSE_FAILURE
#define MPPL_DSL_SUCCESS PARSE_SUCCESS

#define LL1_TERMINAL_TO_VALUE(prefix, name) \
    _##prefix##_map_terminal_##name()

#define LL1_RULE_TO_VALUE(prefix, name) \
    _##prefix##_map_rule_##name()

#define LL1_MAP_TERMINAL(prefix, name, value) \
    static inline int LL1_TERMINAL_TO_VALUE(prefix, name) { return value; }

#define LL1_MAP_RULE(prefix, name, value) \
    static inline int LL1_RULE_TO_VALUE(prefix, name) { return value; }

#define MPPL_TERMINAL_TO_VALUE(name) \
    LL1_TERMINAL_TO_VALUE(mppl, name)

#define MPPL_RULE_TO_VALUE(name) \
    LL1_RULE_TO_VALUE(mppl, name)

#define MPPL_MAP_TERMINAL(name, value) \
    LL1_MAP_TERMINAL(mppl, name, value)

#define MPPL_MAP_RULE(name, value) \
    LL1_MAP_RULE(mppl, name, value)

#define MPPL_DSL_CALLBACK(event) \
    parser_callback(pa, event, parsing_rule);

#define MPPL_DSL_RETURN(x) \
    parse_status = x; \
    break;

#define MPPL_DSL_IS_STATUS(x) \
    parse_status == x

#define MPPL_DSL_EXPAND_DIRECTIVE(x) \
    EXPAND(CONCAT(MPPL_EXPAND_, LIST_HEAD(x)) (LIST_TAIL(x)))

#define MPPL_DSL_EXPAND_RULE(rule) \
    parse_status = mppl_rule_##rule(pa);

#define MPPL_NORMALIZE_DIRECTIVE(x) \
    EXPAND(PREMITIVE_CONCAT(MPPL_GET_DIRECTIVE_, x))

#define MPPL_DSL_IS_DIRECTIVE_RAW(x) \
    HEAD_IS_CELL(MPPL_NORMALIZE_DIRECTIVE(x))

#define MPPL_DSL_IS_DIRECTIVE_CELL(x) \
    MPPL_DSL_IS_DIRECTIVE_RAW(LIST_HEAD(x) LIST_TAIL(x))

#define IMPL_MPPL_DSL_EXPAND(x) \
    IF(HEAD_IS_CELL(x))( \
        IF(MPPL_DSL_IS_DIRECTIVE_CELL(x))( \
            OBSTRUCT(MPPL_DSL_EXPAND_DIRECTIVE)(x), \
            OBSTRUCT(MPPL_DSL_EXPAND_RULE)(EXPAND x) \
        ), \
        IF(MPPL_DSL_IS_DIRECTIVE_RAW(x))( \
            OBSTRUCT(MPPL_DSL_EXPAND_DIRECTIVE)(MPPL_NORMALIZE_DIRECTIVE(x)), \
            OBSTRUCT(MPPL_DSL_EXPAND_RULE)(x) \
        ) \
    )
#define IMPL_MPPL_DSL_EXPAND_INDIRECT() \
    IMPL_MPPL_DSL_EXPAND
#define MPPL_DSL_EXPAND(x) \
    OBSTRUCT(IMPL_MPPL_DSL_EXPAND_INDIRECT) () (x)

#define MPPL_DSL_EARLY_RETURN(rule, cond, ret) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS(cond)) { \
        MPPL_DSL_RETURN(ret) \
    }

#define MPPL_DSL_SUCCESS_ON_SUCCESS(rule) \
    MPPL_DSL_EARLY_RETURN(rule, MPPL_DSL_SUCCESS, MPPL_DSL_SUCCESS)

#define MPPL_DSL_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EARLY_RETURN(rule, MPPL_DSL_FAILURE, MPPL_DSL_FAILURE)

#define IMPL_MPPL_EXPAND_seq(rules) \
    do { \
        INVOKE_ALL(MPPL_DSL_RULE_FAILURE_ON_FAILURE, rules) \
    } while (0);
#define IMPL_MPPL_EXPAND_seq_INDIRECT() \
    IMPL_MPPL_EXPAND_seq
#define MPPL_EXPAND_seq(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_seq_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_alt(rules) \
    do { \
        INVOKE_ALL(MPPL_DSL_SUCCESS_ON_SUCCESS, rules) \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    } while (0);
#define IMPL_MPPL_EXPAND_alt_INDIRECT() \
    IMPL_MPPL_EXPAND_alt
#define MPPL_EXPAND_alt(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_alt_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_rep(rules) \
    while (1) { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), MPPL_DSL_FAILURE, MPPL_DSL_SUCCESS) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
    }
#define IMPL_MPPL_EXPAND_rep_INDIRECT() \
    IMPL_MPPL_EXPAND_rep
#define MPPL_EXPAND_rep(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_rep_INDIRECT) () (rules)

#define IMPL_MPPL_EXPAND_opt(rules) \
    do { \
        MPPL_DSL_EARLY_RETURN(LIST_HEAD(rules), MPPL_DSL_FAILURE, MPPL_DSL_SUCCESS) \
        INVOKE_ALL(MPPL_DSL_FAILURE_ON_FAILURE, LIST_TAIL(rules)) \
        MPPL_DSL_RETURN(MPPL_DSL_SUCCESS) \
    } while (0);
#define IMPL_MPPL_EXPAND_opt_INDIRECT() \
    IMPL_MPPL_EXPAND_opt
#define MPPL_EXPAND_opt(rules) \
    OBSTRUCT(IMPL_MPPL_EXPAND_opt_INDIRECT) () (rules)

#define MPPL_DSL_RULE_FAILURE_ON_FAILURE(rule) \
    MPPL_DSL_EXPAND(rule) \
    if (MPPL_DSL_IS_STATUS(MPPL_DSL_FAILURE)) { \
        MPPL_DSL_RETURN(MPPL_DSL_FAILURE) \
    }
#define MPPL_DECLARE_RULE(name) \
    int mppl_rule_##name(parser_t *pa);

#define MPPL_DEFINE_RULE(name, rule) \
    int mppl_rule_##name(parser_t *pa) { \
        const int parsing_rule = MPPL_RULE_TO_VALUE(name); \
        int parse_status = MPPL_DSL_SUCCESS; \
        MPPL_DSL_CALLBACK(MPPL_DSL_ENTER) \
        WHEN(NOT(INVOKE(IS_EMPTY, rule))) ( \
            EVAL(INVOKE(MPPL_DSL_EXPAND, rule)) \
        ) \
        MPPL_DSL_CALLBACK(parse_status) \
        return parse_status; \
    }

#define MPPL_EXPAND_term(name) \
    parse_status = CONCAT(mppl_terminal_, LIST_HEAD(name))(pa);

#define MPPL_DECLARE_TERMINAL(name) \
    int mppl_terminal_##name(parser_t *pa);

#define MPPL_DEFINE_TERMINAL(name) \
    int mppl_terminal_##name(parser_t *pa) { \
        const int parsing_rule = MPPL_TERMINAL_TO_VALUE(name); \
        MPPL_DSL_CALLBACK(MPPL_DSL_ENTER) \
        if (lexer_lookahead(&pa->lexer) != parsing_rule) { \
            MPPL_DSL_CALLBACK(MPPL_DSL_FAILURE) \
            return MPPL_DSL_FAILURE; \
        } \
        MPPL_DSL_CALLBACK(MPPL_DSL_SUCCESS) \
        lexer_next(&pa->lexer); \
        return MPPL_DSL_SUCCESS; \
    }

#define MPPL_EXPAND_manual(code) \
    do { \
        code \
    } while (0); \

#define MPPL_GET_DIRECTIVE_seq(arg) (seq) (arg)
#define MPPL_GET_DIRECTIVE_alt(arg) (alt) (arg)
#define MPPL_GET_DIRECTIVE_rep(arg) (rep) (arg)
#define MPPL_GET_DIRECTIVE_opt(arg) (opt) (arg)
#define MPPL_GET_DIRECTIVE_term(arg) (term) (arg)
#define MPPL_GET_DIRECTIVE_manual(arg) (manual) (arg)

#endif /* PARSER_DSL_H */