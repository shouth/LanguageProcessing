#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "token-list.h"
#include "scan-info.h"
#include "str-buf.h"
#include "scan.h"

static int initialized = 0;
static int scanning = 0;

int num_attr;
char string_attr[MAXSTRSIZE];

static scanner_t scanner;
static str_buf_t str_buf;

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

static int scan_blank(scanner_t *si)
{
    if (isblank(scanner_top(si))) {
        scanner_advance(si);
        return 1;
    }

    return -1;
}

static int scan_newline(scanner_t *si)
{
    if (scanner_top(si) == '\n') {
        scanner_advance(si);
        if (scanner_top(si) == '\r') {
            scanner_advance(si);
        }
        scanner_advance_line(si);
        return 1;
    }

    if (scanner_top(si) == '\r') {
        scanner_advance(si);
        if (scanner_top(si) == '\n') {
            scanner_advance(si);
        }
        scanner_advance_line(si);
        return 1;
    }

    return -1;
}

static int scan_comment(scanner_t *si)
{
    if (scanner_top(si) == '{') {
        scanner_advance(si);

        while (1) {
            if (scanner_top(si) == '}') {
                scanner_advance(si);
                break;
            }

            if (iscrlf(scanner_top(si))) {
                scan_newline(si);
                continue;
            }
            if (isgraphical(scanner_top(si))) {
                scanner_advance(si);
                continue;
            }

            if (scanner_top(si) == EOF) {
                fprintf(stderr, "Error on line %d: Reached EOF before closing comment\n", get_linenum());
            } else {
                fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
            }
            return -1;
        }

        return 1;
    }

    if (scanner_top(si) == '/' && scanner_next(si) == '*') {
        scanner_advance(si);
        scanner_advance(si);

        while (1) {
            if (scanner_top(si) == '*' && scanner_next(si) == '/') {
                scanner_advance(si);
                scanner_advance(si);
                break;
            }

            if (iscrlf(scanner_top(si))) {
                scan_newline(si);
                continue;
            }
            if (isgraphical(scanner_top(si))) {
                scanner_advance(si);
                continue;
            }

            if (scanner_top(si) == EOF) {
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

static int scan_string(scanner_t *si)
{
    str_buf_t *sb = &str_buf;

    str_buf_init(sb);

    if (scanner_top(si) == '\'') {
        scanner_advance(si);

        while (1) {
            if (scanner_top(si) == '\'' && scanner_next(si) != '\'') {
                scanner_advance(si);
                break;
            }

            if (scanner_top(si) == '\'' && scanner_next(si) == '\'') {
                if (str_buf_push(sb, '\'') < 0) {
                    fprintf(stderr, "Error on line %d: String is too long\n", get_linenum());
                    return -1;
                }
                if (str_buf_push(sb, '\'') < 0) {
                    fprintf(stderr, "Error on line %d: String is too long\n", get_linenum());
                    return -1;
                }
                scanner_advance(si);
                scanner_advance(si);
                continue;
            }
            if (!iscrlf(scanner_top(si)) && isgraphical(scanner_top(si))) {
                if (str_buf_push(sb, scanner_top(si)) < 0) {
                    fprintf(stderr, "Error on line %d: String is too long\n", get_linenum());
                    return -1;
                }
                scanner_advance(si);
                continue;
            }

            if (scanner_top(si) == EOF) {
                fprintf(stderr, "Error on line %d: Reached EOF before end of string\n", get_linenum());
            } else {
                fprintf(stderr, "Error on line %d: Invalid character is detected\n", get_linenum());
            }
            return -1;
        }

        strcpy(string_attr, str_buf_data(sb));
        return TSTRING;
    }

    return -1;
}

static int scan_unsigned_number(scanner_t *si)
{
    str_buf_t *sb = &str_buf;
    long num;

    str_buf_init(sb);

    if (isdigit(scanner_top(si))) {
        str_buf_push(sb, scanner_top(si));
        scanner_advance(si);

        while (1) {
            if (isdigit(scanner_top(si))) {
                if (str_buf_push(sb, scanner_top(si)) < 0) {
                    fprintf(stderr, "Error on line %d: Number is too long\n", get_linenum());
                    return -1;
                }
                scanner_advance(si);
                continue;
            }

            break;
        }

        errno = 0;
        num = strtol(str_buf_data(sb), NULL, 10);
        if (errno == ERANGE || num > 32767) {
            fprintf(stderr, "Error on line %d: Number needs to be less than 32768\n", get_linenum());
            return -1;
        }
        num_attr = (int) num;
        return TNUMBER;
    }

    return -1;
}

static int scan_name_or_keyword(scanner_t *si)
{
    str_buf_t *sb = &str_buf;
    size_t i;

    str_buf_init(sb);

    if (isalpha(scanner_top(si))) {
        str_buf_push(sb, scanner_top(si));
        scanner_advance(si);

        while (1) {
            if (isalnum(scanner_top(si))) {
                if (str_buf_push(sb, scanner_top(si)) < 0) {
                    fprintf(stderr, "Error on line %d: Name is too long\n", get_linenum());
                    return -1;
                }
                scanner_advance(si);
                continue;
            }

            break;
        }

        for (i = 0; i < KEYWORDSIZE; i++) {
            if (strcmp(str_buf_data(sb), key[i].keyword) == 0) {
                return key[i].keytoken;
            }
        }

        strcpy(string_attr, str_buf_data(sb));
        return TNAME;
    }

    return -1;
}

static int scan_symbol(scanner_t *si)
{
    switch (scanner_top(si)) {
    case '+':
        scanner_advance(si);
        return TPLUS;
    case '-':
        scanner_advance(si);
        return TMINUS;
    case '*':
        scanner_advance(si);
        return TSTAR;
    case '=':
        scanner_advance(si);
        return TEQUAL;
    case '(':
        scanner_advance(si);
        return TLPAREN;
    case ')':
        scanner_advance(si);
        return TRPAREN;
    case '[':
        scanner_advance(si);
        return TLSQPAREN;
    case ']':
        scanner_advance(si);
        return TRSQPAREN;
    case '.':
        scanner_advance(si);
        return TDOT;
    case ',':
        scanner_advance(si);
        return TCOMMA;
    case ';':
        scanner_advance(si);
        return TSEMI;

    case ':':
        scanner_advance(si);
        switch (scanner_top(si)) {
        case '=':
            scanner_advance(si);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        scanner_advance(si);
        switch (scanner_top(si)) {
        case '=':
            scanner_advance(si);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        scanner_advance(si);
        switch (scanner_top(si)) {
        case '>':
            scanner_advance(si);
            return TNOTEQ;
        case '=':
            scanner_advance(si);
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
    scanner_t *si = &scanner;
    int code;

    if (!scanning) {
        scanning = 1;
    }

    while (1) {
        /* return on EOF */
        if (scanner_top(&scanner) == EOF) {
            return -1;
        }

        /* skip space and tab */
        if (isblank(scanner_top(si))) {
            scan_blank(si);
            continue;
        }

        /* skip new line */
        if (iscrlf(scanner_top(si))) {
            scan_newline(si);
            continue;
        }

        /* skip comment */
        if (scanner_top(si) == '{' || (scanner_top(si) == '/' && scanner_next(si) == '*')) {
            scan_comment(si);
            continue;
        }

        /* read string */
        if (scanner_top(si) == '\'') {
            return scan_string(si);
        }

        /* read unsigned number */
        if (isdigit(scanner_top(si))) {
            return scan_unsigned_number(si);
        }

        /* read name or keyword */
        if (isalpha(scanner_top(si))) {
            return scan_name_or_keyword(si);
        }

        /* read symbol */
        if ((code = scan_symbol(&scanner)) > 0) {
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
