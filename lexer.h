#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include "scanner.h"

#define LEX_SUCCESS 0
#define LEX_FAILURE -1

int iscrlf(int c);

int isgraphical(int c);

int lex_blank(scanner_t *sc);

int lex_newline(scanner_t *sc);

int lex_comment(scanner_t *sc);

int lex_string(scanner_t *sc);

int lex_unsigned_number(scanner_t *sc);

int lex_name_or_keyword(scanner_t *sc);

int lex_symbol(scanner_t *sc);

#endif /* LEXER_H */