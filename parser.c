#include "parser.h"
#include "lexer.h"
#include "scanner.h"
#include "message.h"
#include "parser-dsl.h"
#include "token-list.h"

MPPL_DEFINE_TERMINAL(name, TNAME)
MPPL_DEFINE_TERMINAL(program, TPROGRAM)
MPPL_DEFINE_TERMINAL(var, TVAR)
MPPL_DEFINE_TERMINAL(array, TARRAY)
MPPL_DEFINE_TERMINAL(of, TOF)
MPPL_DEFINE_TERMINAL(begin, TBEGIN)
MPPL_DEFINE_TERMINAL(end, TEND)
MPPL_DEFINE_TERMINAL(if, TIF)
MPPL_DEFINE_TERMINAL(then, TTHEN)
MPPL_DEFINE_TERMINAL(else, TELSE)
MPPL_DEFINE_TERMINAL(procedure, TPROCEDURE)
MPPL_DEFINE_TERMINAL(return, TRETURN)
MPPL_DEFINE_TERMINAL(call, TCALL)
MPPL_DEFINE_TERMINAL(while, TWHILE)
MPPL_DEFINE_TERMINAL(do, TDO)
MPPL_DEFINE_TERMINAL(not, TNOT)
MPPL_DEFINE_TERMINAL(or, TOR)
MPPL_DEFINE_TERMINAL(div, TDIV)
MPPL_DEFINE_TERMINAL(and, TAND)
MPPL_DEFINE_TERMINAL(char, TCHAR)
MPPL_DEFINE_TERMINAL(integer, TINTEGER)
MPPL_DEFINE_TERMINAL(boolean, TBOOLEAN)
MPPL_DEFINE_TERMINAL(readln, TREADLN)
MPPL_DEFINE_TERMINAL(writeln, TWRITELN)
MPPL_DEFINE_TERMINAL(true, TTRUE)
MPPL_DEFINE_TERMINAL(false, TFALSE)
MPPL_DEFINE_TERMINAL(number, TNUMBER)
MPPL_DEFINE_TERMINAL(string, TSTRING)
MPPL_DEFINE_TERMINAL(plus, TPLUS)
MPPL_DEFINE_TERMINAL(minus, TMINUS)
MPPL_DEFINE_TERMINAL(star, TSTAR)
MPPL_DEFINE_TERMINAL(equal, TEQUAL)
MPPL_DEFINE_TERMINAL(noteq, TNOTEQ)
MPPL_DEFINE_TERMINAL(le, TLE)
MPPL_DEFINE_TERMINAL(leeq, TLEEQ)
MPPL_DEFINE_TERMINAL(gr, TGR)
MPPL_DEFINE_TERMINAL(greq, TGREQ)
MPPL_DEFINE_TERMINAL(lparen, TLPAREN)
MPPL_DEFINE_TERMINAL(rparen, TRPAREN)
MPPL_DEFINE_TERMINAL(lsqparen, TLSQPAREN)
MPPL_DEFINE_TERMINAL(rsqparen, TRSQPAREN)
MPPL_DEFINE_TERMINAL(assign, TASSIGN)
MPPL_DEFINE_TERMINAL(dot, TDOT)
MPPL_DEFINE_TERMINAL(comma, TCOMMA)
MPPL_DEFINE_TERMINAL(colon, TCOLON)
MPPL_DEFINE_TERMINAL(semi, TSEMI)
MPPL_DEFINE_TERMINAL(read, TREAD)
MPPL_DEFINE_TERMINAL(write, TWRITE)
MPPL_DEFINE_TERMINAL(break, TBREAK)

MPPL_DECLARE_RULE(program)
MPPL_DECLARE_RULE(block)
MPPL_DECLARE_RULE(variable_declaration)
MPPL_DECLARE_RULE(variable_names)
MPPL_DECLARE_RULE(variable_name)
MPPL_DECLARE_RULE(type)
MPPL_DECLARE_RULE(standard_type)
MPPL_DECLARE_RULE(array_type)
MPPL_DECLARE_RULE(subprogram_declaration)
MPPL_DECLARE_RULE(procedure_name)
MPPL_DECLARE_RULE(formal_parameters)
MPPL_DECLARE_RULE(compound_statement)
MPPL_DECLARE_RULE(statement)
MPPL_DECLARE_RULE(condition_statement)
MPPL_DECLARE_RULE(iteration_statement)
MPPL_DECLARE_RULE(exit_statement)
MPPL_DECLARE_RULE(call_statement)
MPPL_DECLARE_RULE(expressions)
MPPL_DECLARE_RULE(return_statement)
MPPL_DECLARE_RULE(assignment_statement)
MPPL_DECLARE_RULE(left_part)
MPPL_DECLARE_RULE(variable)
MPPL_DECLARE_RULE(expression)
MPPL_DECLARE_RULE(simple_expression)
MPPL_DECLARE_RULE(term)
MPPL_DECLARE_RULE(factor)
MPPL_DECLARE_RULE(constant)
MPPL_DECLARE_RULE(multiplicative_operator)
MPPL_DECLARE_RULE(additive_operator)
MPPL_DECLARE_RULE(relational_operator)
MPPL_DECLARE_RULE(input_statement)
MPPL_DECLARE_RULE(output_statement)
MPPL_DECLARE_RULE(output_format)
MPPL_DECLARE_RULE(empty_statement)

MPPL_DEFINE_RULE(
    program,
    RULE_PROGRAM,

    (MPPL_TERMINAL(program))
    (MPPL_TERMINAL(name))
    (MPPL_TERMINAL(semi))
    (block)
    (MPPL_TERMINAL(dot))
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
    variable_declaration,
    RULE_VARIABLE_DECLARATION,

    (MPPL_TERMINAL(var))
    (variable_names)
    (MPPL_TERMINAL(colon))
    (type)
    (MPPL_TERMINAL(semi))
    (MPPL_REPEAT(
        (variable_names)
        (MPPL_TERMINAL(colon))
        (type)
        (MPPL_TERMINAL(semi))
    ))
)



MPPL_DEFINE_RULE(
    output_statement,
    RULE_OUTPUT_STATEMENT,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(write))
        (MPPL_TERMINAL(writeln))
    ))
    (MPPL_OPTION(
        (MPPL_TERMINAL(lparen))
        (output_format)
        (MPPL_REPEAT(
            (MPPL_TERMINAL(comma))
            (output_format)
        ))
        (MPPL_TERMINAL(rparen))
    ))
)
int parser_init(parser_t *pa, const char *filename, parser_cb_t *on_success, parser_cb_t *on_failure)
{
    lexer_init(&pa->lexer, filename);
    pa->on_success = on_success;
    pa->on_failure = on_failure;
    pa->expected_terminals = 0;
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
    ret = pa->on_success(pa, args);
    va_end(args);
    return ret;
}

int parser_failure(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    ret = pa->on_failure(pa, args);
    va_end(args);
    return ret;
}
