#include "parser.h"
#include "lexer.h"
#include "scanner.h"
#include "message.h"
#include "parser-dsl.h"
#include "token-list.h"

MPPL_MAP_TERMINAL(name, TNAME)
MPPL_MAP_TERMINAL(program, TPROGRAM)
MPPL_MAP_TERMINAL(var, TVAR)
MPPL_MAP_TERMINAL(array, TARRAY)
MPPL_MAP_TERMINAL(of, TOF)
MPPL_MAP_TERMINAL(begin, TBEGIN)
MPPL_MAP_TERMINAL(end, TEND)
MPPL_MAP_TERMINAL(if, TIF)
MPPL_MAP_TERMINAL(then, TTHEN)
MPPL_MAP_TERMINAL(else, TELSE)
MPPL_MAP_TERMINAL(procedure, TPROCEDURE)
MPPL_MAP_TERMINAL(return, TRETURN)
MPPL_MAP_TERMINAL(call, TCALL)
MPPL_MAP_TERMINAL(while, TWHILE)
MPPL_MAP_TERMINAL(do, TDO)
MPPL_MAP_TERMINAL(not, TNOT)
MPPL_MAP_TERMINAL(or, TOR)
MPPL_MAP_TERMINAL(div, TDIV)
MPPL_MAP_TERMINAL(and, TAND)
MPPL_MAP_TERMINAL(char, TCHAR)
MPPL_MAP_TERMINAL(integer, TINTEGER)
MPPL_MAP_TERMINAL(boolean, TBOOLEAN)
MPPL_MAP_TERMINAL(readln, TREADLN)
MPPL_MAP_TERMINAL(writeln, TWRITELN)
MPPL_MAP_TERMINAL(true, TTRUE)
MPPL_MAP_TERMINAL(false, TFALSE)
MPPL_MAP_TERMINAL(number, TNUMBER)
MPPL_MAP_TERMINAL(string, TSTRING)
MPPL_MAP_TERMINAL(plus, TPLUS)
MPPL_MAP_TERMINAL(minus, TMINUS)
MPPL_MAP_TERMINAL(star, TSTAR)
MPPL_MAP_TERMINAL(equal, TEQUAL)
MPPL_MAP_TERMINAL(noteq, TNOTEQ)
MPPL_MAP_TERMINAL(le, TLE)
MPPL_MAP_TERMINAL(leeq, TLEEQ)
MPPL_MAP_TERMINAL(gr, TGR)
MPPL_MAP_TERMINAL(greq, TGREQ)
MPPL_MAP_TERMINAL(lparen, TLPAREN)
MPPL_MAP_TERMINAL(rparen, TRPAREN)
MPPL_MAP_TERMINAL(lsqparen, TLSQPAREN)
MPPL_MAP_TERMINAL(rsqparen, TRSQPAREN)
MPPL_MAP_TERMINAL(assign, TASSIGN)
MPPL_MAP_TERMINAL(dot, TDOT)
MPPL_MAP_TERMINAL(comma, TCOMMA)
MPPL_MAP_TERMINAL(colon, TCOLON)
MPPL_MAP_TERMINAL(semi, TSEMI)
MPPL_MAP_TERMINAL(read, TREAD)
MPPL_MAP_TERMINAL(write, TWRITE)
MPPL_MAP_TERMINAL(break, TBREAK)

MPPL_MAP_RULE(root, 0)
MPPL_MAP_RULE(program, RULE_PROGRAM)
MPPL_MAP_RULE(block, RULE_BLOCK)
MPPL_MAP_RULE(variable_declaration, RULE_VARIABLE_DECLARATION)
MPPL_MAP_RULE(variable_names, RULE_VARIABLE_NAMES)
MPPL_MAP_RULE(variable_name, RULE_VARIABLE_NAME)
MPPL_MAP_RULE(type, RULE_TYPE)
MPPL_MAP_RULE(standard_type, RULE_STANDARD_TYPE)
MPPL_MAP_RULE(array_type, RULE_ARRAY_TYPE)
MPPL_MAP_RULE(subprogram_declaration, RULE_SUBPROGRAM_DECLARATION)
MPPL_MAP_RULE(procedure_name, RULE_PROCEDURE_NAME)
MPPL_MAP_RULE(formal_parameters, RULE_FORMAL_PARAMETERS)
MPPL_MAP_RULE(compound_statement, RULE_COMPOUND_STATEMENT)
MPPL_MAP_RULE(statement, RULE_STATEMENT)
MPPL_MAP_RULE(condition_statement, RULE_CONDITION_STATEMENT)
MPPL_MAP_RULE(iteration_statement, RULE_ITERATION_STATEMENT)
MPPL_MAP_RULE(exit_statement, RULE_EXIT_STATEMENT)
MPPL_MAP_RULE(call_statement, RULE_CALL_STATEMENT)
MPPL_MAP_RULE(expressions, RULE_EXPRESSIONS)
MPPL_MAP_RULE(return_statement, RULE_RETURN_STATEMENT)
MPPL_MAP_RULE(assignment_statement, RULE_ASSIGNMENT_STATEMENT)
MPPL_MAP_RULE(left_part, RULE_LEFT_PART)
MPPL_MAP_RULE(variable, RULE_VARIABLE)
MPPL_MAP_RULE(expression, RULE_EXPRESSION)
MPPL_MAP_RULE(simple_expression, RULE_SIMPLE_EXPRESSION)
MPPL_MAP_RULE(term, RULE_TERM)
MPPL_MAP_RULE(factor, RULE_FACTOR)
MPPL_MAP_RULE(constant, RULE_CONSTANT)
MPPL_MAP_RULE(multiplicative_operator, RULE_MULTIPLICATIVE_OPERATOR)
MPPL_MAP_RULE(additive_operator, RULE_ADDITIVE_OPERATOR)
MPPL_MAP_RULE(relational_operator, RULE_RELATIONAL_OPERATOR)
MPPL_MAP_RULE(input_statement, RULE_INPUT_STATEMENT)
MPPL_MAP_RULE(output_statement, RULE_OUTPUT_STATEMENT)
MPPL_MAP_RULE(output_format, RULE_OUTPUT_FORMAT)
MPPL_MAP_RULE(empty_statement, RULE_EMPTY_STATEMENT)

MPPL_DEFINE_TERMINAL(name)
MPPL_DEFINE_TERMINAL(program)
MPPL_DEFINE_TERMINAL(var)
MPPL_DEFINE_TERMINAL(array)
MPPL_DEFINE_TERMINAL(of)
MPPL_DEFINE_TERMINAL(begin)
MPPL_DEFINE_TERMINAL(end)
MPPL_DEFINE_TERMINAL(if)
MPPL_DEFINE_TERMINAL(then)
MPPL_DEFINE_TERMINAL(else)
MPPL_DEFINE_TERMINAL(procedure)
MPPL_DEFINE_TERMINAL(return)
MPPL_DEFINE_TERMINAL(call)
MPPL_DEFINE_TERMINAL(while)
MPPL_DEFINE_TERMINAL(do)
MPPL_DEFINE_TERMINAL(not)
MPPL_DEFINE_TERMINAL(or)
MPPL_DEFINE_TERMINAL(div)
MPPL_DEFINE_TERMINAL(and)
MPPL_DEFINE_TERMINAL(char)
MPPL_DEFINE_TERMINAL(integer)
MPPL_DEFINE_TERMINAL(boolean)
MPPL_DEFINE_TERMINAL(readln)
MPPL_DEFINE_TERMINAL(writeln)
MPPL_DEFINE_TERMINAL(true)
MPPL_DEFINE_TERMINAL(false)
MPPL_DEFINE_TERMINAL(number)
MPPL_DEFINE_TERMINAL(string)
MPPL_DEFINE_TERMINAL(plus)
MPPL_DEFINE_TERMINAL(minus)
MPPL_DEFINE_TERMINAL(star)
MPPL_DEFINE_TERMINAL(equal)
MPPL_DEFINE_TERMINAL(noteq)
MPPL_DEFINE_TERMINAL(le)
MPPL_DEFINE_TERMINAL(leeq)
MPPL_DEFINE_TERMINAL(gr)
MPPL_DEFINE_TERMINAL(greq)
MPPL_DEFINE_TERMINAL(lparen)
MPPL_DEFINE_TERMINAL(rparen)
MPPL_DEFINE_TERMINAL(lsqparen)
MPPL_DEFINE_TERMINAL(rsqparen)
MPPL_DEFINE_TERMINAL(assign)
MPPL_DEFINE_TERMINAL(dot)
MPPL_DEFINE_TERMINAL(comma)
MPPL_DEFINE_TERMINAL(colon)
MPPL_DEFINE_TERMINAL(semi)
MPPL_DEFINE_TERMINAL(read)
MPPL_DEFINE_TERMINAL(write)
MPPL_DEFINE_TERMINAL(break)

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

MPPL_DEFINE_RULE(root, (program))

MPPL_DEFINE_RULE(program,
    (seq
        (term (program))
        (term (name))
        (term (semi))
        (block)
        (term (dot))
    )
)

MPPL_DEFINE_RULE(block,
    (seq
        (rep
            (alt
                (variable_declaration)
                (subprogram_declaration)
            )
        )
        (compound_statement)
    )
)

MPPL_DEFINE_RULE(variable_declaration,
    (seq
        (term (var))
        (variable_names)
        (term (colon))
        (type)
        (term (semi))
        (rep
            (variable_names)
            (term (colon))
            (type)
            (term (semi))
        )
    )
)

MPPL_DEFINE_RULE(variable_names,
    (seq
        (variable_name)
        (rep
            (term (comma))
            (variable_name)
        )
    )
)

MPPL_DEFINE_RULE(variable_name,
    (term (name))
)

MPPL_DEFINE_RULE(type,
    (alt
        (standard_type)
        (array_type)
    )
)

MPPL_DEFINE_RULE(standard_type,
    (alt
        (term (integer))
        (term (boolean))
        (term (char))
    )
)

MPPL_DEFINE_RULE(array_type,
    (seq
        (term (array))
        (term (lsqparen))
        (term (number))
        (term (rsqparen))
        (term (of))
        (standard_type)
    )
)

MPPL_DEFINE_RULE(subprogram_declaration,
    (seq
        (term (procedure))
        (procedure_name)
        (opt
            (formal_parameters)
        )
        (term (semi))
        (opt
            (variable_declaration)
        )
        (compound_statement)
        (term (semi))
    )
)

MPPL_DEFINE_RULE(procedure_name,
    (term (name))
)

MPPL_DEFINE_RULE(formal_parameters,
    (seq
        (term (lparen))
        (variable_names)
        (term (colon))
        (type)
        (rep
            (term (semi))
            (variable_names)
            (term (colon))
            (type)
        )
        (term (rparen))
    )
)

MPPL_DEFINE_RULE(compound_statement,
    (seq
        (term (begin))
        (statement)
        (rep
            (term (semi))
            (statement)
        )
        (term (end))
    )
)

MPPL_DEFINE_RULE(statement,
    (alt
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
    )
)

MPPL_DEFINE_RULE(condition_statement,
    (seq
        (term (if))
        (expression)
        (term (then))
        (statement)
        (opt
            (term (else))
            (statement)
        )
    )
)

MPPL_DEFINE_RULE(iteration_statement,
    (seq
        (term (while))
        (expression)
        (term (do))
        (statement)
    )
)

MPPL_DEFINE_RULE(exit_statement,
    (term (break))
)

MPPL_DEFINE_RULE(call_statement,
    (seq
        (term (call))
        (procedure_name)
        (opt
            (term (lparen))
            (expressions)
            (term (rparen))
        )
    )
)

MPPL_DEFINE_RULE(expressions,
    (seq
        (expression)
        (rep
            (term (comma))
            (expression)
        )
    )
)

MPPL_DEFINE_RULE(return_statement,
    (term (return))
)

MPPL_DEFINE_RULE(assignment_statement,
    (seq
        (left_part)
        (term (assign))
        (expression)
    )
)

MPPL_DEFINE_RULE(left_part,
    (variable)
)

MPPL_DEFINE_RULE(variable,
    (seq
        (variable_name)
        (opt
            (term (lsqparen))
            (expression)
            (term (rsqparen))
        )
    )
)

MPPL_DEFINE_RULE(expression,
    (seq
        (simple_expression)
        (rep
            (relational_operator)
            (simple_expression)
        )
    )
)

MPPL_DEFINE_RULE(simple_expression,
    (seq
        (opt
            (alt
                (term (plus))
                (term (minus))
            )
        )
        (term)
        (rep
            (additive_operator)
            (term)
        )
    )
)

MPPL_DEFINE_RULE(term,
    (seq
        (factor)
        (rep
            (multiplicative_operator)
            (factor)
        )
    )
)

MPPL_DEFINE_RULE(factor,
    (alt
        (variable)
        (constant)
        (seq
            (term (lparen))
            (expression)
            (term (rparen))
        )
        (seq
            (term (not))
            (factor)
        )
        (seq
            (standard_type)
            (term (lparen))
            (expression)
            (term (rparen))
        )
    )
)

MPPL_DEFINE_RULE(constant,
    (alt
        (term (number))
        (term (true))
        (term (false))
        (term (string))
    )
)

MPPL_DEFINE_RULE(multiplicative_operator,
    (alt
        (term (star))
        (term (div))
        (term (and))
    )
)

MPPL_DEFINE_RULE(additive_operator,
    (alt
        (term (plus))
        (term (minus))
        (term (or))
    )
)

MPPL_DEFINE_RULE(relational_operator,
    (alt
        (term (equal))
        (term (noteq))
        (term (le))
        (term (leeq))
        (term (gr))
        (term (greq))
    )
)

MPPL_DEFINE_RULE(input_statement,
    (seq
        (alt
            (term (read))
            (term (readln))
        )
        (opt
            (term (lparen))
            (variable)
            (rep
                (term (comma))
                (variable)
            )
            (term (rparen))
        )
    )
)

MPPL_DEFINE_RULE(output_statement,
    (seq
        (alt
            (term (write))
            (term (writeln))
        )
        (opt
            (term (lparen))
            (output_format)
            (rep
                (term (comma))
                (output_format)
            )
            (term (rparen))
        )
    )
)

MPPL_DEFINE_RULE(output_format,
    (alt
        (seq
            (expression)
            (opt
                (term (colon))
                (term (number))
            )
        )
        (term (string))
    )
)

MPPL_DEFINE_RULE(empty_statement,)

int parser_init(parser_t *pa, const char *filename, parser_cb_t *cb)
{
    lexer_init(&pa->lexer, filename);
    pa->cb = cb;
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
