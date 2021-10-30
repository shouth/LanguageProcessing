#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "token-list.h"
#include "scanner.h"
#include "scan.h"
#include "lexer.h"

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
    return scanner_location(&scanner)->line;
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
            return SCAN_FAILURE;
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
                return SCAN_FAILURE;
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
                return SCAN_FAILURE;
            }

            if (scanner_buf_overflow(sc)) {
                message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "string needs to be shorter than %d", MAXSTRSIZE);
                return SCAN_FAILURE;
            }
            strcpy(string_attr, scanner_buf_data(sc));
            return code;
        }

        /* read unsigned number */
        if (isdigit(scanner_top(sc))) {
            code = lex_unsigned_number(sc);
            errno = 0;
            num = strtol(scanner_buf_data(sc), NULL, 10);
            if (errno == ERANGE || num > 32767) {
                message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "number needs to be less than 32768", MAXSTRSIZE);
                return SCAN_FAILURE;
            }
            num_attr = (int) num;
            return code;
        }

        /* read name or keyword */
        if (isalpha(scanner_top(sc))) {
            code = lex_name_or_keyword(sc);
            if (scanner_buf_overflow(sc)) {
                message_token_error(sc, scanner_pre_location(sc), scanner_location(sc), "name needs to be shorter than %d", MAXSTRSIZE);
                return SCAN_FAILURE;
            }
            strcpy(string_attr, scanner_buf_data(sc));
            return code;
        }

        /* read symbol */
        if ((code = lex_symbol(sc)) > 0) {
            return code;
        }

        message_error(sc, scanner_location(sc), "invalid character is detected");
        return SCAN_FAILURE;
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

const scanner_loc_t *get_location()
{
    return scanner_location(&scanner);
}
