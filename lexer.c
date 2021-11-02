#include <errno.h>

#include "lexer.h"
#include "scanner.h"

int iscrlf(int c)
{
    return c == '\n' || c == '\r';
}

int isgraphical(int c)
{
    return isblank(c) || isgraph(c) || iscrlf(c);
}

int lexer_consume(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (le->buf_size + 1 < le->buf_capacity) {
        le->buf[le->buf_size] = scanner_lookahead_1(sc);
        le->buf_size++;
        le->buf[le->buf_size] = '\0';
    }
    scanner_next(sc);
}

int lex_blank(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (isblank(scanner_lookahead_1(sc))) {
        lexer_consume(le);
        return LEX_SUCCESS;
    }
    return LEX_FAILURE;
}

int lex_newline(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (scanner_lookahead_1(sc) == '\n') {
        lexer_consume(le);
        if (scanner_lookahead_1(sc) == '\r') {
            lexer_consume(le);
        }
        scanner_next_line(sc);
        return LEX_SUCCESS;
    }

    if (scanner_lookahead_1(sc) == '\r') {
        lexer_consume(le);
        if (scanner_lookahead_1(sc) == '\n') {
            lexer_consume(le);
        }
        scanner_next_line(sc);
        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_comment(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (scanner_lookahead_1(sc) == '{') {
        lexer_consume(le);

        while (1) {
            if (scanner_lookahead_1(sc) == '}') {
                lexer_consume(le);
                break;
            }

            if (iscrlf(scanner_lookahead_1(sc))) {
                lex_newline(le);
                continue;
            }
            if (isgraphical(scanner_lookahead_1(sc))) {
                lexer_consume(le);
                continue;
            }

            return LEX_FAILURE;
        }

        return LEX_SUCCESS;
    }

    if (scanner_lookahead_1(sc) == '/' && scanner_lookahead_2(sc) == '*') {
        lexer_consume(le);
        lexer_consume(le);

        while (1) {
            if (scanner_lookahead_1(sc) == '*' && scanner_lookahead_2(sc) == '/') {
                lexer_consume(le);
                lexer_consume(le);
                break;
            }

            if (iscrlf(scanner_lookahead_1(sc))) {
                lex_newline(le);
                continue;
            }
            if (isgraphical(scanner_lookahead_1(sc))) {
                lexer_consume(le);
                continue;
            }

            return LEX_FAILURE;
        }

        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_string(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (scanner_lookahead_1(sc) == '\'') {
        lexer_consume(le);

        while (1) {
            if (scanner_lookahead_1(sc) == '\'' && scanner_lookahead_2(sc) != '\'') {
                lexer_consume(le);
                break;
            }

            if (scanner_lookahead_1(sc) == '\'' && scanner_lookahead_2(sc) == '\'') {
                lexer_consume(le);
                lexer_consume(le);
                continue;
            }
            if (!iscrlf(scanner_lookahead_1(sc)) && isgraphical(scanner_lookahead_1(sc))) {
                lexer_consume(le);
                continue;
            }

            return LEX_FAILURE;
        }

        return TSTRING;
    }

    return LEX_FAILURE;
}

int lex_unsigned_number(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    if (isdigit(scanner_lookahead_1(sc))) {
        lexer_consume(le);

        while (1) {
            if (isdigit(scanner_lookahead_1(sc))) {
                lexer_consume(le);
                continue;
            }

            break;
        }

        return TNUMBER;
    }

    return LEX_FAILURE;
}

int lex_name_or_keyword(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    size_t i;

    if (isalpha(scanner_lookahead_1(sc))) {
        lexer_consume(le);

        while (1) {
            if (isalnum(scanner_lookahead_1(sc))) {
                lexer_consume(le);
                continue;
            }

            break;
        }

        for (i = 0; i < KEYWORDSIZE; i++) {
            if (strcmp(le->buf, key[i].keyword) == 0) {
                return key[i].keytoken;
            }
        }

        return TNAME;
    }

    return LEX_FAILURE;
}

int lex_symbol(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    switch (scanner_lookahead_1(sc)) {
    case '+':
        lexer_consume(le);
        return TPLUS;
    case '-':
        lexer_consume(le);
        return TMINUS;
    case '*':
        lexer_consume(le);
        return TSTAR;
    case '=':
        lexer_consume(le);
        return TEQUAL;
    case '(':
        lexer_consume(le);
        return TLPAREN;
    case ')':
        lexer_consume(le);
        return TRPAREN;
    case '[':
        lexer_consume(le);
        return TLSQPAREN;
    case ']':
        lexer_consume(le);
        return TRSQPAREN;
    case '.':
        lexer_consume(le);
        return TDOT;
    case ',':
        lexer_consume(le);
        return TCOMMA;
    case ';':
        lexer_consume(le);
        return TSEMI;

    case ':':
        lexer_consume(le);
        switch (scanner_lookahead_1(sc)) {
        case '=':
            lexer_consume(le);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        lexer_consume(le);
        switch (scanner_lookahead_1(sc)) {
        case '=':
            lexer_consume(le);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        lexer_consume(le);
        switch (scanner_lookahead_1(sc)) {
        case '>':
            lexer_consume(le);
            return TNOTEQ;
        case '=':
            lexer_consume(le);
            return TLEEQ;
        default:
            return TLE;
        }

    default:
        return LEX_FAILURE;
    }
}

int lex_token(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    location_t pre_loc, loc;

    int code;
    long num;

    scanner_location(sc, &pre_loc);
    while (1) {
        le->buf[0] = '\0';
        le->buf_size = 0;

        /* return on EOF */
        if (scanner_lookahead_1(sc) == EOF) {
            return LEX_FAILURE;
        }

        /* skip space and tab */
        if (isblank(scanner_lookahead_1(sc))) {
            lex_blank(le);
            continue;
        }

        /* skip new line */
        if (iscrlf(scanner_lookahead_1(sc))) {
            lex_newline(le);
            continue;
        }

        /* skip comment */
        if (scanner_lookahead_1(sc) == '{' || (scanner_lookahead_1(sc) == '/' && scanner_lookahead_2(sc) == '*')) {
            code = lex_comment(le);
            if (code == LEX_FAILURE) {
                if (scanner_lookahead_1(sc) == EOF) {
                    message_error(sc, &pre_loc, "comment is unterminated");
                } else {
                    scanner_location(sc, &loc);
                    message_error(sc, &loc, "invalid character is detected");
                }
                return LEX_FAILURE;
            }
            continue;
        }

        /* read string */
        if (scanner_lookahead_1(sc) == '\'') {
            code = lex_string(le);
            if (code == LEX_FAILURE) {
                if (scanner_lookahead_1(sc) == EOF || iscrlf(scanner_lookahead_1(sc))) {
                    message_error(sc, &pre_loc, "string is unterminated");
                } else {
                    scanner_location(sc, &loc);
                    message_error(sc, &loc, "invalid character is detected");
                }
                return LEX_FAILURE;
            }

            if (le->buf_size - 2 > MAXSTRSIZE) {
                scanner_location(sc, &loc);
                message_token_error(sc, &pre_loc, &loc, "string needs to be shorter than %d", MAXSTRSIZE);
                return LEX_FAILURE;
            }
            strncpy(le->string_attr, le->buf + 1, le->buf_size - 2);
            le->string_attr[le->buf_size - 2] = '\0';
            return code;
        }

        /* read unsigned number */
        if (isdigit(scanner_lookahead_1(sc))) {
            code = lex_unsigned_number(le);
            errno = 0;
            le->num_attr = strtol(le->buf, NULL, 10);
            if (errno == ERANGE || le->num_attr > 32767) {
                scanner_location(sc, &loc);
                message_token_error(sc, &pre_loc, &loc, "number needs to be less than 32768", MAXSTRSIZE);
                return LEX_FAILURE;
            }
            return code;
        }

        /* read name or keyword */
        if (isalpha(scanner_lookahead_1(sc))) {
            code = lex_name_or_keyword(le);
            if (le->buf_size > MAXSTRSIZE) {
                scanner_location(sc, &loc);
                message_token_error(sc, &pre_loc, &loc, "name needs to be shorter than %d", MAXSTRSIZE);
                return LEX_FAILURE;
            }
            strcpy(le->string_attr, le->buf);
            return code;
        }

        /* read symbol */
        if ((code = lex_symbol(le)) > 0) {
            return code;
        }

        scanner_location(sc, &loc);
        message_error(sc, &loc, "invalid character is detected");
        return LEX_FAILURE;
    }
}

int lexer_init(lexer_t *le, const char *filename)
{
    if (le == NULL || filename == NULL) {
        return -1;
    }
    scanner_init(&le->scanner, filename);
    le->buf[0] = '\0';
    le->buf_capacity = sizeof(le->buf) / sizeof(le->buf[0]);
    le->buf_size = 0;
    le->last_token = lex_token(le);
    return 0;
}

int lexer_free(lexer_t *le)
{
    scanner_free(&le->scanner);
}

int lexer_top(const lexer_t *le)
{
    if (le == NULL) {
        return -1;
    }
    return le->last_token;
}

void lexer_next(lexer_t *le)
{
    if (le == NULL) {
        return;
    }
    le->last_token = lex_token(le);
}

const scanner_t *lexer_scanner(const lexer_t *le)
{
    if (le == NULL) {
        return NULL;
    }
    return &le->scanner;
}

int lexer_num_attr(const lexer_t *le)
{
    if (le == NULL) {
        return -1;
    }
    return le->num_attr;
}

const char *lexer_string_attr(const lexer_t *le)
{
    if (le == NULL) {
        return NULL;
    }
    return le->string_attr;
}