#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "token-list.h"
#include "scan-info.h"
#include "str-buf.h"

static int initialized = 0;
static int scanning = 0;

/**
 * This variable is a part of the specification of Task 1.
 */
int num_attr;

/**
 * This variable is a part of the specification of Task 1.
 */
char string_attr[MAXSTRSIZE];

static scan_info_t scan_info;
static str_buf_t str_buf;

/**
 * Open the file and initiate the scanner.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return 0 on successful initiation or a negative number on failure.
 */
int init_scan(char *filename)
{
    if (initialized) {
        return -1;
    }
    if (scan_info_init(&scan_info, filename) < 0) {
        return -1;
    }
    initialized = 1;
    return 1;
}

int isgraphical(int c)
{
    return isblank(c) || isgraph(c) || c == '\n' || c == '\r';
}

static int scan_blank(scan_info_t *si)
{
    if (isblank(scan_info_top(si))) {
        scan_info_advance(si);
        return 1;
    }

    return 0;
}

static int scan_newline(scan_info_t *si)
{
    if (scan_info_top(si) == '\n') {
        scan_info_advance(si);
        if (scan_info_top(si) == '\r') {
            scan_info_advance(si);
        }
        scan_info_advance_line(si);
        return 1;
    }

    if (scan_info_top(si) == '\r') {
        scan_info_advance(si);
        if (scan_info_top(si) == '\n') {
            scan_info_advance(si);
        }
        scan_info_advance_line(si);
        return 1;
    }

    return 0;
}

static int scan_comment(scan_info_t *si)
{
    if (scan_info_top(si) == '{') {
        scan_info_advance(si);

        while (1) {
            if (scan_info_top(si) == EOF) {
                return -1;
            }
            if (!isgraphical(scan_info_top(si))) {
                return -1;
            }
            if (scan_info_top(si) == '}') {
                scan_info_advance(si);
                break;
            }
            scan_info_advance(si);
        }

        return 1;
    }

    if (scan_info_top(si) == '/' && scan_info_next(si) == '*') {
        scan_info_advance(si);
        scan_info_advance(si);

        while (1) {
            if (scan_info_top(si) == EOF) {
                return -1;
            }
            if (!isgraphical(scan_info_top(si))) {
                return -1;
            }
            if (scan_info_top(si) == '*' && scan_info_next(si) == '/') {
                scan_info_advance(si);
                scan_info_advance(si);
                break;
            }
            scan_info_advance(si);
        }

        return 1;
    }

    return 0;
}

static int scan_string(scan_info_t *si)
{
    str_buf_t *sb = &str_buf;

    str_buf_init(sb);

    if (scan_info_top(si) == '\'') {
        scan_info_advance(si);

        while (1) {
            if (scan_info_top(si) == EOF) {
                return -1;
            }
            if (scan_info_top(si) == '\n' || scan_info_top(si) == '\r') {
                return -1;
            }
            if (!isgraphical(scan_info_top(si))) {
                return -1;
            }

            if (scan_info_top(si) == '\'' && scan_info_next(si) == '\'') {
                if (str_buf_push(sb, '\'') < 0) {
                    return -1;
                }
                if (str_buf_push(sb, '\'') < 0) {
                    return -1;
                }
                scan_info_advance(si);
                scan_info_advance(si);
            } else {
                if (str_buf_push(sb, scan_info_top(si)) < 0) {
                    return -1;
                }
                scan_info_advance(si);
            }

            if (scan_info_top(si) == '\'' && scan_info_next(si) != '\'') {
                scan_info_advance(si);
                break;
            }
        }

        strcpy(string_attr, str_buf_data(sb));
        return TSTRING;
    }

    return 0;
}

static int scan_unsigned_number(scan_info_t *si)
{
    str_buf_t *sb = &str_buf;
    long num;

    str_buf_init(sb);

    if (isdigit(scan_info_top(si))) {
        str_buf_push(sb, scan_info_top(si));
        scan_info_advance(si);

        while (isdigit(scan_info_top(si))) {
            if (str_buf_push(sb, scan_info_top(si)) < 0) {
                return -1;
            }
            scan_info_advance(si);
        }

        errno = 0;
        num = strtol(str_buf_data(sb), NULL, 10);
        if (errno == ERANGE || num > 32767) {
            return -1;
        }
        num_attr = (int) num;
        return TNUMBER;
    }

    return 0;
}

static int scan_name_or_keyword(scan_info_t *si)
{
    str_buf_t *sb = &str_buf;
    size_t i;

    str_buf_init(sb);

    if (isalpha(scan_info_top(si))) {
        str_buf_push(sb, scan_info_top(si));
        scan_info_advance(si);

        while (isalnum(scan_info_top(si))) {
            if (str_buf_push(sb, scan_info_top(si)) < 0) {
                return -1;
            }
            scan_info_advance(si);
        }

        for (i = 0; i < KEYWORDSIZE; i++) {
            if (strcmp(str_buf_data(sb), key[i].keyword) == 0) {
                return key[i].keytoken;
            }
        }

        strcpy(string_attr, str_buf_data(sb));
        return TNAME;
    }

    return 0;
}

static int scan_symbol(scan_info_t *si)
{
    switch (scan_info_top(si)) {
    case '+':
        scan_info_advance(si);
        return TPLUS;
    case '-':
        scan_info_advance(si);
        return TMINUS;
    case '*':
        scan_info_advance(si);
        return TSTAR;
    case '=':
        scan_info_advance(si);
        return TEQUAL;
    case '(':
        scan_info_advance(si);
        return TLPAREN;
    case ')':
        scan_info_advance(si);
        return TRPAREN;
    case '[':
        scan_info_advance(si);
        return TLSQPAREN;
    case ']':
        scan_info_advance(si);
        return TRSQPAREN;
    case '.':
        scan_info_advance(si);
        return TDOT;
    case ',':
        scan_info_advance(si);
        return TCOMMA;
    case ';':
        scan_info_advance(si);
        return TSEMI;

    case ':':
        scan_info_advance(si);
        switch (scan_info_top(si)) {
        case '=':
            scan_info_advance(si);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        scan_info_advance(si);
        switch (scan_info_top(si)) {
        case '=':
            scan_info_advance(si);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        scan_info_advance(si);
        switch (scan_info_top(si)) {
        case '>':
            scan_info_advance(si);
            return TNOTEQ;
        case '=':
            scan_info_advance(si);
            return TLEEQ;
        default:
            return TLE;
        }

    default:
        return 0;
    }
}

/**
 * Reads the current file and gets the code of next token.
 * Refer to token-list.h to see which value will be returned.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return The code of the next token or -1 when failed to scan.
 */
int scan(void)
{
    int code;

    if (!scanning) {
        scanning = 1;
    }

    while (1) {
        /* return on EOF */
        if (scan_info_top(&scan_info) == EOF) {
            return -1;
        }

        /* skip space and tab */
        if ((code = scan_blank(&scan_info)) > 0) {
            continue;
        } else if (code < 0) {
            return -1;
        }

        /* skip new line */
        if ((code = scan_newline(&scan_info)) > 0) {
            continue;
        } else if (code < 0) {
            return -1;
        }

        /* skip comment */
        if ((code = scan_comment(&scan_info)) > 0) {
            continue;
        } else if (code < 0) {
            return -1;
        }

        /* read string */
        if ((code = scan_string(&scan_info)) > 0) {
            return code;
        } else if (code < 0) {
            return -1;
        }

        /* read unsigned number */
        if ((code = scan_unsigned_number(&scan_info)) > 0) {
            return code;
        } else if (code < 0) {
            return -1;
        }

        /* read name or keyword */
        if ((code = scan_name_or_keyword(&scan_info)) > 0) {
            return code;
        } else if (code < 0) {
            return -1;
        }

        /* read symbol */
        if ((code = scan_symbol(&scan_info)) > 0) {
            return code;
        } else if (code < 0) {
            return -1;
        }

        return -1;
    }
}

/**
 * Gets the number of lines that the last read token lies on.
 *
 * This function is a part of the specification of Task 1.
 *
 * @return The number of lines that the last read token lies on
 * or 0 if scan() is not called before.
 */
int get_linenum(void)
{
    if (!scanning) {
        return 0;
    }
    return scan_info_line_number(&scan_info);
}

/**
 * Close the current file and terminate the scaner.
 *
 * This function is a part of the specification of Task 1.
 */
void end_scan(void)
{
    if (!initialized) {
        initialized = 0;
        scan_info_free(&scan_info);
    }
}
