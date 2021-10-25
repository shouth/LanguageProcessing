#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "token-list.h"

typedef struct {
    FILE *file;
    int c0, c1;
    int line_number;
} scan_info_t;

int init_scan_info(scan_info_t *si, char *filename)
{
    si->file = fopen(filename, "r");
    if (si->file == NULL) {
        return -1;
    }

    si->c0 = fgetc(si->file);
    si->c1 = fgetc(si->file);
    si->line_number = 1;
    return 0;
}

int free_scan_info(scan_info_t *si)
{
    return fclose(si->file);
}

void scan_info_advance(scan_info_t *si)
{
    si->c0 = si->c1;
    si->c1 = fgetc(si->file);
}

void scan_info_advance_line(scan_info_t *si)
{
    si->line_number++;
}

int scan_info_top(scan_info_t *si)
{
    return si->c0;
}

int scan_info_next(scan_info_t *si)
{
    return si->c1;
}

int scan_info_line_number(scan_info_t *si)
{
    return si->line_number;
}

typedef struct {
    char buffer[MAXSTRSIZE];
    size_t end;
} str_buf_t;

void init_str_buf(str_buf_t *sb)
{
    sb->buffer[0] = '\0';
    sb->end = 0;
}

int str_buf_push(str_buf_t *sb, char c)
{
    if (sb->end + 1 >= MAXSTRSIZE) {
        return -1;
    }
    sb->buffer[sb->end] = c;
    sb->end++;
    sb->buffer[sb->end] = '\0';
    return 0;
}

char str_buf_pop(str_buf_t *sb)
{
    sb->end--;
    char ret = sb->buffer[sb->end];
    sb->buffer[sb->end] = '\0';
    return ret;
}

const char *str_buf_data(str_buf_t *sb)
{
    return sb->buffer;
}

int initialized = 0;

/**
 * This variable is a part of the specification of Task 1.
 */
int num_attr;

/**
 * This variable is a part of the specification of Task 1.
 */
char string_attr[MAXSTRSIZE];

static scan_info_t scan_info;

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
    if (init_scan_info(&scan_info, filename) < 0) {
        return -1;
    }
    initialized = 1;
    return 1;
}

int read_symbol(scan_info_t *si)
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
        return -1;
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
    scan_info_t *si = &scan_info;
    static str_buf_t str_buf;
    str_buf_t *sb = &str_buf;

    long num;
    int code;
    size_t i;

    init_str_buf(sb);

    while (1) {
        /* return on EOF */
        if (scan_info_top(si) == EOF) {
            return -1;
        }

        /* skip space and tab */
        if (isblank(scan_info_top(si))) {
            scan_info_advance(si);
            continue;
        }

        /* skip new line with \n */
        if (scan_info_top(si) == '\n') {
            scan_info_advance(si);
            if (scan_info_top(si) == '\r') {
                scan_info_advance(si);
            }
            scan_info_advance_line(si);
            continue;
        }

        /* skip new line with \r */
        if (scan_info_top(si) == '\r') {
            scan_info_advance(si);
            if (scan_info_top(si) == '\n') {
                scan_info_advance(si);
            }
            scan_info_advance_line(si);
            continue;
        }

        /* skip braces comment */
        if (scan_info_top(si) == '{') {
            scan_info_advance(si);

            while (1) {
                if (scan_info_top(si) == EOF) {
                    return -1;
                }
                if (scan_info_top(si) == '}') {
                    scan_info_advance(si);
                    break;
                }
                scan_info_advance(si);
            }

            continue;
        }

        /* skip c-style comment */
        if (scan_info_top(si) == '/' && scan_info_next(si) == '*') {
            scan_info_advance(si);
            scan_info_advance(si);

            while (1) {
                if (scan_info_top(si) == EOF) {
                    return -1;
                }
                if (scan_info_top(si) == '*' && scan_info_next(si) == '/') {
                    scan_info_advance(si);
                    scan_info_advance(si);
                    break;
                }
                scan_info_advance(si);
            }

            continue;
        }

        /* read string */
        if (scan_info_top(si) == '\'') {
            scan_info_advance(si);

            while (1) {
                if (scan_info_top(si) == EOF) {
                    return -1;
                }
                if (scan_info_top(si) == '\n' || scan_info_top(si) == '\r') {
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

        /* read unsigned number */
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

        /* read name or keyword */
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

        /* read symbol */
        code = read_symbol(si);
        if (code > 0) {
            return code;
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
    if (!initialized) {
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
        free_scan_info(&scan_info);
    }
}
