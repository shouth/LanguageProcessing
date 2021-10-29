#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "token-list.h"
#include "scanner.h"
#include "scan.h"

static int initialized = 0;
static int scanning = 0;

int num_attr;
char string_attr[MAXSTRSIZE];

static scanner_t scanner;

int init_scan(char *filename)
{
    if (initialized) {
        fprintf(stderr, "Already initialized\n");
        return -1;
    }
    if (scanner_init(&scanner, filename) < 0) {
        fprintf(stderr, "Cannot initialize. Maybe `filename` is wrong.\n");
        return -1;
    }
    initialized = 1;
    return 1;
}

int get_linenum(void)
{
    if (!scanning) {
        return 0;
    }
    return scanner_line_number(&scanner);
}

int iscrlf(int c)
{
    return c == '\n' || c == '\r';
}

int isgraphical(int c)
{
    return isblank(c) || isgraph(c) || iscrlf(c);
}

static int scan_blank(scanner_t *sc)
{
    if (isblank(scanner_top(sc))) {
        scanner_advance(sc);
        return 1;
    }

    return -1;
}

static int scan_newline(scanner_t *sc)
{
    if (scanner_top(sc) == '\n') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\r') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return 1;
    }

    if (scanner_top(sc) == '\r') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\n') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return 1;
    }

    return -1;
}

static int scan_comment(scanner_t *sc)
{
    if (scanner_top(sc) == '{') {
        scanner_advance(sc);

        while (1) {
            if (scanner_top(sc) == '}') {
                scanner_advance(sc);
                break;
            }

            if (iscrlf(scanner_top(sc))) {
                scan_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            if (scanner_top(sc) == EOF) {
                fprintf(stderr, "Error on line %d: Reached EOF before closing comment\n", get_linenum());
            } else {
                fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
            }
            return -1;
        }

        return 1;
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
                scan_newline(sc);
                continue;
            }
            if (isgraphical(scanner_top(sc))) {
                scanner_advance(sc);
                continue;
            }

            if (scanner_top(sc) == EOF) {
                fprintf(stderr, "Error on line %d: Reached EOF before closing comment\n", get_linenum());
            } else {
                fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
            }
            return -1;
        }

        return 1;
    }

    return -1;
}

static int scan_string(scanner_t *sc)
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

            if (scanner_top(sc) == EOF) {
                fprintf(stderr, "Error on line %d: Reached EOF before end of string\n", get_linenum());
            } else {
                fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
            }
            return -1;
        }

        return TSTRING;
    }

    return -1;
}

static int scan_unsigned_number(scanner_t *sc)
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

    return -1;
}

static int scan_name_or_keyword(scanner_t *sc)
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

    return -1;
}

static int scan_symbol(scanner_t *sc)
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
        return -1;
    }
}

int scan(void)
{
    scanner_t *sc = &scanner;
    int code;
    long num;

    if (!scanning) {
        scanning = 1;
    }

    while (1) {
        scanner_clear_buf(sc);

        /* return on EOF */
        if (scanner_top(sc) == EOF) {
            return -1;
        }

        /* skip space and tab */
        if (isblank(scanner_top(sc))) {
            scan_blank(sc);
            continue;
        }

        /* skip new line */
        if (iscrlf(scanner_top(sc))) {
            scan_newline(sc);
            continue;
        }

        /* skip comment */
        if (scanner_top(sc) == '{' || (scanner_top(sc) == '/' && scanner_next(sc) == '*')) {
            scan_comment(sc);
            continue;
        }

        /* read string */
        if (scanner_top(sc) == '\'') {
            code = scan_string(sc);
            if (scanner_buf_overflow(sc)) {
                fprintf(stderr, "Error on line %d: String needs to be shorter than %d\n", get_linenum(), MAXSTRSIZE);
                return -1;
            }
            strcpy(string_attr, scanner_buf_data(sc));
            return code;
        }

        /* read unsigned number */
        if (isdigit(scanner_top(sc))) {
            code = scan_unsigned_number(sc);
            errno = 0;
            num = strtol(scanner_buf_data(sc), NULL, 10);
            if (errno == ERANGE || num > 32767) {
                fprintf(stderr, "Error on line %d: Number needs to be less than 32768\n", get_linenum());
                return -1;
            }
            num_attr = (int) num;
            return code;
        }

        /* read name or keyword */
        if (isalpha(scanner_top(sc))) {
            code = scan_name_or_keyword(sc);
            if (scanner_buf_overflow(sc)) {
                fprintf(stderr, "Error on line %d: Name needs to be shorter than %d\n", get_linenum(), MAXSTRSIZE);
                return -1;
            }
            strcpy(string_attr, scanner_buf_data(sc));
            return code;
        }

        /* read symbol */
        if ((code = scan_symbol(sc)) > 0) {
            return code;
        }

        fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
        return -1;
    }
}

void end_scan(void)
{
    if (!initialized) {
        initialized = 0;
        scanning = 0;
        scanner_free(&scanner);
    }
}
