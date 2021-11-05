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
    variable_names,
    RULE_VARIABLE_NAMES,

    (variable_name)
    (MPPL_REPEAT(
        (MPPL_TERMINAL(comma))
        (variable_name)
    ))
)

MPPL_DEFINE_RULE(
    variable_name,
    RULE_VARIABLE_NAME,

    (MPPL_TERMINAL(name))
)

MPPL_DEFINE_RULE(
    type,
    RULE_TYPE,

    (MPPL_ALTERNATE(
        (standard_type)
        (array_type)
    ))
)

MPPL_DEFINE_RULE(
    standard_type,
    RULE_STANDARD_TYPE,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(integer))
        (MPPL_TERMINAL(boolean))
        (MPPL_TERMINAL(char))
    ))
)

MPPL_DEFINE_RULE(
    array_type,
    RULE_ARRAY_TYPE,

    (MPPL_TERMINAL(array))
    (MPPL_TERMINAL(lsqparen))
    (MPPL_TERMINAL(number))
    (MPPL_TERMINAL(rsqparen))
    (MPPL_TERMINAL(of))
    (standard_type)
)

MPPL_DEFINE_RULE(
    subprogram_declaration,
    RULE_SUBPROGRAM_DECLARATION,

    (MPPL_TERMINAL(procedure))
    (procedure_name)
    (MPPL_OPTION(
        (formal_parameters)
    ))
    (MPPL_TERMINAL(semi))
    (MPPL_OPTION(
        (variable_declaration)
    ))
    (compound_statement)
    (MPPL_TERMINAL(semi))
)

MPPL_DEFINE_RULE(
    procedure_name,
    RULE_PROCEDURE_NAME,

    (MPPL_TERMINAL(name))
)

MPPL_DEFINE_RULE(
    formal_parameters,
    RULE_FORMAL_PARAMETERS,

    (MPPL_TERMINAL(lparen))
    (variable_names)
    (MPPL_TERMINAL(colon))
    (type)
    (MPPL_REPEAT(
        (MPPL_TERMINAL(semi))
        (variable_names)
        (MPPL_TERMINAL(colon))
        (type)
    ))
    (MPPL_TERMINAL(rparen))
)

MPPL_DEFINE_RULE(
    compound_statement,
    RULE_COMPOUND_STATEMENT,

    (MPPL_TERMINAL(begin))
    (statement)
    (MPPL_REPEAT(
        (MPPL_TERMINAL(semi))
        (statement)
    ))
    (MPPL_TERMINAL(end))
)

MPPL_DEFINE_RULE(
    statement,
    RULE_STATEMENT,

    (MPPL_ALTERNATE(
        (assignment_statement)
        (condition_statement)
        (iteration_statement)
        (exit_statement)
        (call_statement)
        (return_statement)
        (input_statement)
        (output_statement)
        (compound_statement)
        (empty_statement)
    ))
)

MPPL_DEFINE_RULE(
    condition_statement,
    RULE_CONDITION_STATEMENT,

    (MPPL_TERMINAL(if))
    (expression)
    (MPPL_TERMINAL(then))
    (statement)
    (MPPL_OPTION(
        (MPPL_TERMINAL(else))
        (statement)
    ))
)

MPPL_DEFINE_RULE(
    exit_statement,
    RULE_EXIT_STATEMENT,

    (MPPL_TERMINAL(break))
)

MPPL_DEFINE_RULE(
    call_statement,
    RULE_CALL_STATEMENT,

    (MPPL_TERMINAL(call))
    (procedure_name)
    (MPPL_OPTION(
        (MPPL_TERMINAL(lparen))
        (expressions)
        (MPPL_TERMINAL(rparen))
    ))
)

MPPL_DEFINE_RULE(
    expressions,
    RULE_EXPRESSIONS,

    (expression)
    (MPPL_REPEAT(
        (MPPL_TERMINAL(comma))
        (expression)
    ))
)

MPPL_DEFINE_RULE(
    return_statement,
    RULE_RETURN_STATEMENT,

    (MPPL_TERMINAL(return))
)

MPPL_DEFINE_RULE(
    assignment_statement,
    RULE_ASSIGNMENT_STATEMENT,

    (left_part)
    (MPPL_TERMINAL(assign))
    (expression)
)

MPPL_DEFINE_RULE(
    left_part,
    RULE_LEFT_PART,

    (variable)
)

MPPL_DEFINE_RULE(
    variable,
    RULE_VARIABLE,

    (variable_name)
    (MPPL_OPTION(
        (MPPL_TERMINAL(lsqparen))
        (expression)
        (MPPL_TERMINAL(rsqparen))
    ))
)

MPPL_DEFINE_RULE(
    expression,
    RULE_EXPRESSION,

    (simple_expression)
    (MPPL_REPEAT(
        (relational_operator)
        (simple_expression)
    ))
)

MPPL_DEFINE_RULE(
    simple_expression,
    RULE_SIMPLE_EXPRESSION,

    (MPPL_OPTION(
        (MPPL_ALTERNATE(
            (MPPL_TERMINAL(plus))
            (MPPL_TERMINAL(minus))
        ))
    ))
    (term)
    (MPPL_REPEAT(
        (additive_operator)
        (term)
    ))
)

MPPL_DEFINE_RULE(
    term,
    RULE_TERM,

    (factor)
    (MPPL_REPEAT(
        (multiplicative_operator)
        (factor)
    ))
)

MPPL_DEFINE_RULE(
    factor,
    RULE_FACTOR,

    (MPPL_ALTERNATE(
        (variable)
        (constant)
        (MPPL_SEQUENCE(
            (MPPL_TERMINAL(lparen))
            (expression)
            (MPPL_TERMINAL(rparen))
        ))
        (MPPL_SEQUENCE(
            (MPPL_TERMINAL(not))
            (factor)
        ))
        (MPPL_SEQUENCE(
            (standard_type)
            (MPPL_TERMINAL(lparen))
            (expression)
            (MPPL_TERMINAL(rparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    constant,
    RULE_CONSTANT,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(number))
        (MPPL_TERMINAL(false))
        (MPPL_TERMINAL(false))
        (MPPL_TERMINAL(string))
    ))
)

MPPL_DEFINE_RULE(
    multiplicative_operator,
    RULE_MULTIPLICATIVE_OPERATOR,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(star))
        (MPPL_TERMINAL(div))
        (MPPL_TERMINAL(and))
    ))
)

MPPL_DEFINE_RULE(
    additive_operator,
    RULE_ADDITIVE_OPERATOR,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(plus))
        (MPPL_TERMINAL(minus))
        (MPPL_TERMINAL(or))
    ))
)

MPPL_DEFINE_RULE(
    relational_operator,
    RULE_RELATIONAL_OPERATOR,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(equal))
        (MPPL_TERMINAL(noteq))
        (MPPL_TERMINAL(le))
        (MPPL_TERMINAL(leeq))
        (MPPL_TERMINAL(gr))
        (MPPL_TERMINAL(greq))
    ))
)

MPPL_DEFINE_RULE(
    input_statement,
    RULE_INPUT_STATEMENT,

    (MPPL_ALTERNATE(
        (MPPL_TERMINAL(read))
        (MPPL_TERMINAL(readln))
    ))
    (MPPL_OPTION(
        (MPPL_TERMINAL(lparen))
        (variable)
        (MPPL_REPEAT(
            (MPPL_TERMINAL(comma))
            (variable)
        ))
        (MPPL_TERMINAL(rparen))
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

MPPL_DEFINE_RULE(
    output_format,
    RULE_OUTPUT_FORMAT,

    (MPPL_ALTERNATE(
        (MPPL_SEQUENCE(
            (expression)
            (MPPL_OPTION(
                (MPPL_TERMINAL(comma))
                (MPPL_TERMINAL(number))
            ))
        ))
        (MPPL_TERMINAL(string))
    ))
)

MPPL_DEFINE_RULE(
    empty_statement,
    RULE_EMPTY_STATEMENT,
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
