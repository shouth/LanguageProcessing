#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "token-list.h"

typedef struct {
    FILE *file;
    int c0, c1;
} scan_info_t;

int init_scan_info(scan_info_t *si, char *filename)
{
    si->file = fopen(filename, "r");
    if (si->file == NULL) {
        return -1;
    }

    si->c0 = fgetc(si->file);
    si->c1 = fgetc(si->file);
    return 0;
}

int free_scan_info(scan_info_t *si)
{
    return fclose(si->file);
}

void scan_info_proceed(scan_info_t *si)
{
    si->c0 = si->c1;
    si->c1 = fgetc(si->file);
}

int scan_info_next(scan_info_t *si)
{
    return si->c0;
}

int scan_info_ahead(scan_info_t *si)
{
    return si->c1;
}

int scan_info_eof(scan_info_t *si)
{
    return si->c0 == EOF;
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

scan_info_t scan_info;

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
    switch (scan_info_next(si)) {
    case '+':
        scan_info_proceed(si);
        return TPLUS;
    case '-':
        scan_info_proceed(si);
        return TMINUS;
    case '*':
        scan_info_proceed(si);
        return TSTAR;
    case '=':
        scan_info_proceed(si);
        return TEQUAL;
    case '(':
        scan_info_proceed(si);
        return TLPAREN;
    case ')':
        scan_info_proceed(si);
        return TRPAREN;
    case '[':
        scan_info_proceed(si);
        return TLSQPAREN;
    case ']':
        scan_info_proceed(si);
        return TRSQPAREN;
    case '.':
        scan_info_proceed(si);
        return TDOT;
    case ',':
        scan_info_proceed(si);
        return TCOMMA;
    case ';':
        scan_info_proceed(si);
        return TSEMI;

    case ':':
        scan_info_proceed(si);
        switch (scan_info_next(si)) {
        case '=':
            scan_info_proceed(si);
            return TASSIGN;
        default:
            return TCOLON;
        }

    case '>':
        scan_info_proceed(si);
        switch (scan_info_next(si)) {
        case '=':
            scan_info_proceed(si);
            return TGREQ;
        default:
            return TGR;
        }

    case '<':
        scan_info_proceed(si);
        switch (scan_info_next(si)) {
        case '>':
            scan_info_proceed(si);
            return TNOTEQ;
        case '=':
            scan_info_proceed(si);
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
    static char buffer[MAXSTRSIZE];
    size_t buffer_end = 0;
    long num;
    int code;
    size_t i;

    while (1) {
        /* return on EOF */
        if (scan_info_eof(si)) {
            return -1;
        }

        /* skip space */
        if (scan_info_next(si) == ' ') {
            scan_info_proceed(si);
            continue;
        }

        /* skip tab */
        if (scan_info_next(si) == '\t') {
            scan_info_proceed(si);
            continue;
        }

        /* skip new line with \n */
        if (scan_info_next(si) == '\n') {
            scan_info_proceed(si);
            if (scan_info_next(si) == '\r') {
                scan_info_proceed(si);
            }
            continue;
        }

        /* skip new line with \r */
        if (scan_info_next(si) == '\r') {
            scan_info_proceed(si);
            if (scan_info_next(si) == '\n') {
                scan_info_proceed(si);
            }
            continue;
        }

        /* skip braces comment */
        if (scan_info_next(si) == '{') {
            while (!scan_info_eof(si)) {
                scan_info_proceed(si);
                if (scan_info_next(si) == '}') {
                    break;
                }
            }

            if (scan_info_eof(si)) {
                return -1;
            }

            scan_info_proceed(si);
            continue;
        }

        /* skip c-style comment */
        if (scan_info_next(si) == '/' && scan_info_ahead(si) == '*') {
            scan_info_proceed(si);
            while (!scan_info_eof(si)) {
                scan_info_proceed(si);
                if (scan_info_next(si) == '*' && scan_info_ahead(si) == '/') {
                    break;
                }
            }

            if (scan_info_eof(si)) {
                return -1;
            }

            scan_info_proceed(si);
            scan_info_proceed(si);
            continue;
        }

        /* read string */
        if (scan_info_next(si) == '\'') {
            scan_info_proceed(si);

            if (scan_info_next(si) == '\n' || scan_info_next(si) == '\r') {
                return -1;
            }

            if (scan_info_next(si) == '\'' && scan_info_ahead(si) == '\'') {
                if (buffer_end + 2 >= MAXSTRSIZE) {
                    return -1;
                }
                string_attr[buffer_end++] = '\'';
                string_attr[buffer_end++] = '\'';
                scan_info_proceed(si);
                scan_info_proceed(si);
            } else {
                if (buffer_end + 1 >= MAXSTRSIZE) {
                    return -1;
                }
                buffer[buffer_end++] = scan_info_next(si);
                scan_info_proceed(si);
            }

            buffer[buffer_end++] = '\0';
            strcpy(string_attr, buffer);
            return TSTRING;
        }

        /* read unsigned number */
        if (isdigit(scan_info_next(si))) {
            buffer[buffer_end++] = scan_info_next(si);
            scan_info_proceed(si);

            while (isdigit(scan_info_next(si))) {
                if (buffer_end + 1 >= MAXSTRSIZE) {
                    return -1;
                }
                buffer[buffer_end++] = scan_info_next(si);
                scan_info_proceed(si);
            }
            buffer[buffer_end++] = '\0';

            errno = 0;
            num = strtol(buffer, NULL, 10);
            if (errno == ERANGE || num > 32767) {
                return -1;
            }
            num_attr = (int) num;
            return TNUMBER;
        }

        /* read name or keyword */
        if (isalpha(scan_info_next(si))) {
            buffer[buffer_end++] = scan_info_next(si);
            scan_info_proceed(si);

            while (isalnum(scan_info_next(si))) {
                if (buffer_end + 1 >= MAXSTRSIZE) {
                    return -1;
                }
                buffer[buffer_end++] = scan_info_next(si);
                scan_info_proceed(si);
            }
            buffer[buffer_end++] = '\0';

            for (i = 0; i < KEYWORDSIZE; i++) {
                if (strcmp(buffer, key[i].keyword) == 0) {
                    return key[i].keytoken;
                }
            }

            strcpy(string_attr, buffer);
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
 * or 0 if scan() isn't called before.
 */
int get_linenum(void)
{
}

/**
 * Close the current file and terminate the scaner.
 *
 * This function is a part of the specification of Task 1.
 */
void end_scan(void)
{
    initialized = 0;
    free_scan_info(&scan_info);
}
