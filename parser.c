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

MPPL_DEFINE_RULE(root, 0, (program))

MPPL_DEFINE_RULE(
    program,
    RULE_PROGRAM,

    (seq (
        (term (program))
        (term (name))
        (term (semi))
        (block)
        (term (dot))
    ))
)

MPPL_DEFINE_RULE(
    block,
    RULE_BLOCK,

    (seq (
        (rep (
            (alt (
                (variable_declaration)
                (subprogram_declaration)
            ))
        ))
        (compound_statement)
    ))
)

MPPL_DEFINE_RULE(
    variable_declaration,
    RULE_VARIABLE_DECLARATION,

    (seq (
        (term (var))
        (variable_names)
        (term (colon))
        (type)
        (term (semi))
        (rep (
            (variable_names)
            (term (colon))
            (type)
            (term (semi))
        ))
    ))
)

MPPL_DEFINE_RULE(
    variable_names,
    RULE_VARIABLE_NAMES,

    (seq (
        (variable_name)
        (rep (
            (term (comma))
            (variable_name)
        ))
    ))
)

MPPL_DEFINE_RULE(
    variable_name,
    RULE_VARIABLE_NAME,

    (term (name))
)

MPPL_DEFINE_RULE(
    type,
    RULE_TYPE,

    (alt (
        (standard_type)
        (array_type)
    ))
)

MPPL_DEFINE_RULE(
    standard_type,
    RULE_STANDARD_TYPE,

    (alt (
        (term (integer))
        (term (boolean))
        (term (char))
    ))
)

MPPL_DEFINE_RULE(
    array_type,
    RULE_ARRAY_TYPE,

    (seq (
        (term (array))
        (term (lsqparen))
        (term (number))
        (term (rsqparen))
        (term (of))
        (standard_type)
    ))
)

MPPL_DEFINE_RULE(
    subprogram_declaration,
    RULE_SUBPROGRAM_DECLARATION,

    (seq (
        (term (procedure))
        (procedure_name)
        (opt (
            (formal_parameters)
        ))
        (term (semi))
        (opt (
            (variable_declaration)
        ))
        (compound_statement)
        (term (semi))
    ))
)

MPPL_DEFINE_RULE(
    procedure_name,
    RULE_PROCEDURE_NAME,

    (term (name))
)

MPPL_DEFINE_RULE(
    formal_parameters,
    RULE_FORMAL_PARAMETERS,

    (seq (
        (term (lparen))
        (variable_names)
        (term (colon))
        (type)
        (rep (
            (term (semi))
            (variable_names)
            (term (colon))
            (type)
        ))
        (term (rparen))
    ))
)

MPPL_DEFINE_RULE(
    compound_statement,
    RULE_COMPOUND_STATEMENT,

    (seq (
        (term (begin))
        (statement)
        (rep (
            (term (semi))
            (statement)
        ))
        (term (end))
    ))
)

MPPL_DEFINE_RULE(
    statement,
    RULE_STATEMENT,

    (alt (
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

    (seq (
        (term (if))
        (expression)
        (term (then))
        (statement)
        (opt (
            (term (else))
            (statement)
        ))
    ))
)

MPPL_DEFINE_RULE(
    iteration_statement,
    RULE_ITERATION_STATEMENT,

    (seq (
        (term (while))
        (expression)
        (term (do))
        (statement)
    ))
)

MPPL_DEFINE_RULE(
    exit_statement,
    RULE_EXIT_STATEMENT,

    (term (break))
)

MPPL_DEFINE_RULE(
    call_statement,
    RULE_CALL_STATEMENT,

    (seq (
        (term (call))
        (procedure_name)
        (opt (
            (term (lparen))
            (expressions)
            (term (rparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    expressions,
    RULE_EXPRESSIONS,

    (seq (
        (expression)
        (rep (
            (term (comma))
            (expression)
        ))
    ))
)

MPPL_DEFINE_RULE(
    return_statement,
    RULE_RETURN_STATEMENT,

    (term (return))
)

MPPL_DEFINE_RULE(
    assignment_statement,
    RULE_ASSIGNMENT_STATEMENT,

    (seq (
        (left_part)
        (term (assign))
        (expression)
    ))
)

MPPL_DEFINE_RULE(
    left_part,
    RULE_LEFT_PART,

    (variable)
)

MPPL_DEFINE_RULE(
    variable,
    RULE_VARIABLE,

    (seq (
        (variable_name)
        (opt (
            (term (lsqparen))
            (expression)
            (term (rsqparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    expression,
    RULE_EXPRESSION,

    (seq (
        (simple_expression)
        (rep (
            (relational_operator)
            (simple_expression)
        ))
    ))
)

MPPL_DEFINE_RULE(
    simple_expression,
    RULE_SIMPLE_EXPRESSION,

    (seq (
        (opt (
            (alt (
                (term (plus))
                (term (minus))
            ))
        ))
        (term)
        (rep (
            (additive_operator)
            (term)
        ))
    ))
)

MPPL_DEFINE_RULE(
    term,
    RULE_TERM,

    (seq (
        (factor)
        (rep (
            (multiplicative_operator)
            (factor)
        ))
    ))
)

MPPL_DEFINE_RULE(
    factor,
    RULE_FACTOR,

    (alt (
        (variable)
        (constant)
        (seq (
            (term (lparen))
            (expression)
            (term (rparen))
        ))
        (seq (
            (term (not))
            (factor)
        ))
        (seq (
            (standard_type)
            (term (lparen))
            (expression)
            (term (rparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    constant,
    RULE_CONSTANT,

    (alt (
        (term (number))
        (term (true))
        (term (false))
        (term (string))
    ))
)

MPPL_DEFINE_RULE(
    multiplicative_operator,
    RULE_MULTIPLICATIVE_OPERATOR,

    (alt (
        (term (star))
        (term (div))
        (term (and))
    ))
)

MPPL_DEFINE_RULE(
    additive_operator,
    RULE_ADDITIVE_OPERATOR,

    (alt (
        (term (plus))
        (term (minus))
        (term (or))
    ))
)

MPPL_DEFINE_RULE(
    relational_operator,
    RULE_RELATIONAL_OPERATOR,

    (alt (
        (term (equal))
        (term (noteq))
        (term (le))
        (term (leeq))
        (term (gr))
        (term (greq))
    ))
)

MPPL_DEFINE_RULE(
    input_statement,
    RULE_INPUT_STATEMENT,

    (seq (
        (alt (
            (term (read))
            (term (readln))
        ))
        (opt (
            (term (lparen))
            (variable)
            (rep (
                (term (comma))
                (variable)
            ))
            (term (rparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    output_statement,
    RULE_OUTPUT_STATEMENT,

    (seq (
        (alt (
            (term (write))
            (term (writeln))
        ))
        (opt (
            (term (lparen))
            (output_format)
            (rep (
                (term (comma))
                (output_format)
            ))
            (term (rparen))
        ))
    ))
)

MPPL_DEFINE_RULE(
    output_format,
    RULE_OUTPUT_FORMAT,

    (alt (
        (seq (
            (expression)
            (opt (
                (term (colon))
                (term (number))
            ))
        ))
        (term (string))
    ))
)

MPPL_DEFINE_RULE(
    empty_statement,
    RULE_EMPTY_STATEMENT,
)

int parser_init(parser_t *pa, const char *filename, parser_cb_t *cb)
{
    lexer_init(&pa->lexer, filename);
    pa->cb = cb;
    pa->expected_terminals = 0;
}

void parser_free(parser_t *pa)
{
    lexer_free(&pa->lexer);
}

int parser_callback(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    if (pa->cb != NULL) {
        ret = pa->cb(pa, args);
    }
    va_end(args);
    return ret;
}
