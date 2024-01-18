#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct Symbol        Symbol;
typedef struct SymbolContext SymbolContext;

struct Symbol {
  long        length;
  const char *string;
};

#define SYM_LIST(F)            \
  F(PLUS_PUN, "+")             \
  F(MINUS_PUN, "-")            \
  F(STAR_PUN, "*")             \
  F(EQUAL_PUN, "=")            \
  F(NOTEQ_PUN, "<>")           \
  F(LESS_PUN, "<")             \
  F(LESSEQ_PUN, "<=")          \
  F(GREATER_PUN, ">")          \
  F(GREATEREQ_PUN, ">=")       \
  F(LPAREN_PUN, "(")           \
  F(RPAREN_PUN, ")")           \
  F(LBRACKET_PUN, "[")         \
  F(RBRACKET_PUN, "]")         \
  F(ASSIGN_PUN, ":=")          \
  F(DOT_PUN, ".")              \
  F(COMMA_PUN, ",")            \
  F(COLON_PUN, ":")            \
  F(SEMI_PUN, ";")             \
  F(PROGRAM_KW, "program")     \
  F(VAR_KW, "var")             \
  F(ARRAY_KW, "array")         \
  F(OF_KW, "of")               \
  F(BEGIN_KW, "begin")         \
  F(END_KW, "end")             \
  F(IF_KW, "if")               \
  F(THEN_KW, "then")           \
  F(ELSE_KW, "else")           \
  F(PROCEDURE_KW, "procedure") \
  F(RETURN_KW, "return")       \
  F(CALL_KW, "call")           \
  F(WHILE_KW, "while")         \
  F(DO_KW, "do")               \
  F(NOT_KW, "not")             \
  F(OR_KW, "or")               \
  F(DIV_KW, "div")             \
  F(AND_KW, "and")             \
  F(CHAR_KW, "char")           \
  F(INTEGER_KW, "integer")     \
  F(BOOLEAN_KW, "boolean")     \
  F(READ_KW, "read")           \
  F(WRITE_KW, "write")         \
  F(READLN_KW, "readln")       \
  F(WRITELN_KW, "writeln")     \
  F(TRUE_KW, "true")           \
  F(FALSE_KW, "false")         \
  F(BREAK_KW, "break")

#define F(name, string) extern const Symbol *SYM_##name;
SYM_LIST(F)
#undef F

const Symbol  *symbol(const char *string, unsigned long length, SymbolContext *context);
SymbolContext *symbol_context_new(void);
void           symbol_context_free(SymbolContext *context);

#endif
