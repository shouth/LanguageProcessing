#include "lex.h"

int iscrlf(int c)
{
    return c == '\n' || c == '\r';
}

int isgraphical(int c)
{
    return isblank(c) || isgraph(c) || iscrlf(c);
}

int lex_blank(scanner_t *sc)
{
    if (isblank(scanner_top(sc))) {
        scanner_advance(sc);
        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_newline(scanner_t *sc)
{
    if (scanner_top(sc) == '\n') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\r') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return LEX_SUCCESS;
    }

    if (scanner_top(sc) == '\r') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\n') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_comment(scanner_t *sc)
{
    if (scanner_top(sc) == '{') {
        scanner_advance(sc);

        while (1) {
            if (scanner_top(sc) == '}') {
                scanner_advance(sc);
                break;
            }

            if (iscrlf(scanner_top(sc))) {
                lex_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            return LEX_FAILURE;
        }

        return LEX_SUCCESS;
    }

    if (scanner_top(sc) == '/' && scanner_next(sc) == '*') {
        scanner_advance(sc);
        scanner_advance(sc);

        while (1) {
            if (scanner_top(sc) == '*' && scanner_next(sc) == '/') {
                scanner_advance(sc);
                scanner_advance(sc);
                break;
            }

            if (iscrlf(scanner_top(sc))) {
                lex_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            return LEX_FAILURE;
        }

        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_string(scanner_t *sc)
{
    if (scanner_top(sc) == '\'') {
        scanner_advance(sc);

        while (1) {
            if (scanner_top(sc) == '\'' && scanner_next(sc) != '\'') {
                scanner_advance(sc);
                break;
            }

            if (scanner_top(sc) == '\'' && scanner_next(sc) == '\'') {
                scanner_advance(sc);
                scanner_advance(sc);
                continue;
            }
            if (!iscrlf(scanner_top(sc)) && isgraphical(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            return LEX_FAILURE;
        }

        return TSTRING;
    }

    return LEX_FAILURE;
}

int lex_unsigned_number(scanner_t *sc)
{
    if (isdigit(scanner_top(sc))) {
        scanner_advance(sc);

        while (1) {
            if (isdigit(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            break;
        }

        return TNUMBER;
    }

    return LEX_FAILURE;
}

int lex_name_or_keyword(scanner_t *sc)
{
    size_t i;

    if (isalpha(scanner_top(sc))) {
        scanner_advance(sc);

        while (1) {
            if (isalnum(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            break;
        }

        for (i = 0; i < KEYWORDSIZE; i++) {
            if (strcmp(scanner_buf_data(sc), key[i].keyword) == 0) {
                return key[i].keytoken;
            }
        }

        return TNAME;
    }

    return LEX_FAILURE;
}

int lex_symbol(scanner_t *sc)
{
    switch (scanner_top(sc)) {
    case '+':
        scanner_advance(sc);
        return TPLUS;
    case '-':
        scanner_advance(sc);
        return TMINUS;
    case '*':
        scanner_advance(sc);
        return TSTAR;
    case '=':
        scanner_advance(sc);
        return TEQUAL;
    case '(':
        scanner_advance(sc);
        return TLPAREN;
    case ')':
        scanner_advance(sc);
        return TRPAREN;
    case '[':
        scanner_advance(sc);
        return TLSQPAREN;
    case ']':
        scanner_advance(sc);
        return TRSQPAREN;
    case '.':
        scanner_advance(sc);
        return TDOT;
    case ',':
        scanner_advance(sc);
        return TCOMMA;
    case ';':
        scanner_advance(sc);
        return TSEMI;

    case ':':
        scanner_advance(sc);
        switch (scanner_top(sc)) {
        case '=':
            scanner_advance(sc);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        scanner_advance(sc);
        switch (scanner_top(sc)) {
        case '=':
            scanner_advance(sc);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        scanner_advance(sc);
        switch (scanner_top(sc)) {
        case '>':
            scanner_advance(sc);
            return TNOTEQ;
        case '=':
            scanner_advance(sc);
            return TLEEQ;
        default:
            return TLE;
        }

    default:
        return LEX_FAILURE;
    }
}