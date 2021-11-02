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

static lexer_t lexer;

int init_scan(char *filename)
{
    if (initialized) {
        fprintf(stderr, "Already initialized\n");
        return -1;
    }
    if (lexer_init(&lexer, filename) < 0) {
        fprintf(stderr, "Cannot initialize. Maybe `filename` is wrong.\n");
        return -1;
    }
    initialized = 1;
    return 1;
}

int get_linenum(void)
{
    location_t loc;
    if (!scanning) {
        return 0;
    }
    scanner_location(lexer_scanner(&lexer), &loc);
    return loc.line;
}

int scan(void)
{
    int code;
    lexer_t *le = &lexer;

    if (!scanning) {
        scanning = 1;
    }

    code = lexer_top(le);
    lexer_next(le);

    if (code == TNUMBER) {
        num_attr = lexer_num_attr(le);
    }
    if (code == TSTRING || code == TNAME) {
        strcpy(string_attr, lexer_string_attr(le));
    }

    return code;
}

void end_scan(void)
{
    if (!initialized) {
        initialized = 0;
        scanning = 0;
        lexer_free(&lexer);
    }
}
