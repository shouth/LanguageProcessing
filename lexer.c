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

int lex_blank(scanner_t *sc)
{
    if (isblank(scanner_top(sc))) {
        scanner_consume(sc);
        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_newline(scanner_t *sc)
{
    if (scanner_top(sc) == '\n') {
        scanner_consume(sc);
        if (scanner_top(sc) == '\r') {
            scanner_consume(sc);
        }
        scanner_newline(sc);
        return LEX_SUCCESS;
    }

    if (scanner_top(sc) == '\r') {
        scanner_consume(sc);
        if (scanner_top(sc) == '\n') {
            scanner_consume(sc);
        }
        scanner_newline(sc);
        return LEX_SUCCESS;
    }

    return LEX_FAILURE;
}

int lex_comment(scanner_t *sc)
{
    if (scanner_top(sc) == '{') {
        scanner_consume(sc);

        while (1) {
            if (scanner_top(sc) == '}') {
                scanner_consume(sc);
                break;
            }

            if (iscrlf(scanner_top(sc))) {
                lex_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_consume(sc);
                continue;
            }

            return LEX_FAILURE;
        }

        return LEX_SUCCESS;
    }

    if (scanner_top(sc) == '/' && scanner_next(sc) == '*') {
        scanner_consume(sc);
        scanner_consume(sc);

        while (1) {
            if (scanner_top(sc) == '*' && scanner_next(sc) == '/') {
                scanner_consume(sc);
                scanner_consume(sc);
                break;
            }

            if (iscrlf(scanner_top(sc))) {
                lex_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_consume(sc);
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
        scanner_consume(sc);

        while (1) {
            if (scanner_top(sc) == '\'' && scanner_next(sc) != '\'') {
                scanner_consume(sc);
                break;
            }

            if (scanner_top(sc) == '\'' && scanner_next(sc) == '\'') {
                scanner_consume(sc);
                scanner_consume(sc);
                continue;
            }
            if (!iscrlf(scanner_top(sc)) && isgraphical(scanner_top(sc))) {
                scanner_consume(sc);
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
        scanner_consume(sc);

        while (1) {
            if (isdigit(scanner_top(sc))) {
                scanner_consume(sc);
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
        scanner_consume(sc);

        while (1) {
            if (isalnum(scanner_top(sc))) {
                scanner_consume(sc);
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
        scanner_consume(sc);
        return TPLUS;
    case '-':
        scanner_consume(sc);
        return TMINUS;
    case '*':
        scanner_consume(sc);
        return TSTAR;
    case '=':
        scanner_consume(sc);
        return TEQUAL;
    case '(':
        scanner_consume(sc);
        return TLPAREN;
    case ')':
        scanner_consume(sc);
        return TRPAREN;
    case '[':
        scanner_consume(sc);
        return TLSQPAREN;
    case ']':
        scanner_consume(sc);
        return TRSQPAREN;
    case '.':
        scanner_consume(sc);
        return TDOT;
    case ',':
        scanner_consume(sc);
        return TCOMMA;
    case ';':
        scanner_consume(sc);
        return TSEMI;

    case ':':
        scanner_consume(sc);
        switch (scanner_top(sc)) {
        case '=':
            scanner_consume(sc);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        scanner_consume(sc);
        switch (scanner_top(sc)) {
        case '=':
            scanner_consume(sc);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        scanner_consume(sc);
        switch (scanner_top(sc)) {
        case '>':
            scanner_consume(sc);
            return TNOTEQ;
        case '=':
            scanner_consume(sc);
            return TLEEQ;
        default:
            return TLE;
        }

    default:
        return LEX_FAILURE;
    }
}

int lex_token(scanner_t *sc)
{
    int code;
    long num;

    while (1) {
        scanner_clear_buf(sc);

        /* return on EOF */
        if (scanner_top(sc) == EOF) {
            return LEX_FAILURE;
        }

        /* skip space and tab */
        if (isblank(scanner_top(sc))) {
            lex_blank(sc);
            continue;
        }

        /* skip new line */
        if (iscrlf(scanner_top(sc))) {
            lex_newline(sc);
            continue;
        }

        /* skip comment */
        if (scanner_top(sc) == '{' || (scanner_top(sc) == '/' && scanner_next(sc) == '*')) {
            code = lex_comment(sc);
            if (code == LEX_FAILURE) {
                if (scanner_top(sc) == EOF) {
                    message_error(sc, scanner_pre_location(sc), "comment is unterminated");
                } else {
                    message_error(sc, scanner_location(sc), "invalid character is detected");
                }
                return LEX_FAILURE;
            }
            continue;
        }

        /* read string */
        if (scanner_top(sc) == '\'') {
            code = lex_string(sc);
            if (code == LEX_FAILURE) {
                if (scanner_top(sc) == EOF || iscrlf(scanner_top(sc))) {
                    message_error(sc, scanner_pre_location(sc), "string is unterminated");
                } else {
                    message_error(sc, scanner_location(sc), "invalid character is detected");
                }
                return LEX_FAILURE;
            }
            return code;
        }

        /* read unsigned number */
        if (isdigit(scanner_top(sc))) {
            code = lex_unsigned_number(sc);
            return code;
        }

        /* read name or keyword */
        if (isalpha(scanner_top(sc))) {
            code = lex_name_or_keyword(sc);
            return code;
        }

        /* read symbol */
        if ((code = lex_symbol(sc)) > 0) {
            return code;
        }

        message_error(sc, scanner_location(sc), "invalid character is detected");
        return LEX_FAILURE;
    }
}

int lexer_read(lexer_t *le)
{
    scanner_t *sc = &le->scanner;
    le->last_token = lex_token(sc);

    if (le->last_token == TSTRING) {
        if (scanner_buf_overflow(sc)) {
            message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "string needs to be shorter than %d", MAXSTRSIZE);
            return LEX_FAILURE;
        }
        strncpy(le->string_attr, scanner_buf_data(sc) + 1, scanner_buf_size(sc) - 2);
        le->string_attr[scanner_buf_size(sc) - 2] = '\0';
        return LEX_SUCCESS;
    }

    if (le->last_token == TNUMBER) {
        errno = 0;
        le->num_attr = strtol(scanner_buf_data(sc), NULL, 10);
        if (errno == ERANGE || le->num_attr > 32767) {
            message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "number needs to be less than 32768", MAXSTRSIZE);
            return LEX_FAILURE;
        }
        return LEX_SUCCESS;
    }

    if (le->last_token == TNAME) {
        if (scanner_buf_overflow(sc)) {
            message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "name needs to be shorter than %d", MAXSTRSIZE);
            return LEX_FAILURE;
        }
        strcpy(le->string_attr, scanner_buf_data(sc));
        return LEX_SUCCESS;
    }

    return LEX_SUCCESS;
}

int lexer_init(lexer_t *le, const char *filename)
{
    if (le == NULL || filename == NULL) {
        return -1;
    }
    scanner_init(&le->scanner, filename);
    lexer_read(le);
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

void lexer_consume(lexer_t *le)
{
    if (le == NULL) {
        return;
    }
    lexer_read(le);
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