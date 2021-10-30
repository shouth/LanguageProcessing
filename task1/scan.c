#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

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
    return scanner_location(&scanner)->line;
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
        return SCAN_SUCCESS;
    }

    return SCAN_FAILURE;
}

static int scan_newline(scanner_t *sc)
{
    if (scanner_top(sc) == '\n') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\r') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return SCAN_SUCCESS;
    }

    if (scanner_top(sc) == '\r') {
        scanner_advance(sc);
        if (scanner_top(sc) == '\n') {
            scanner_advance(sc);
        }
        scanner_advance_line(sc);
        return SCAN_SUCCESS;
    }

    return SCAN_FAILURE;
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
                print_error(scanner_pre_location(sc), "comment is unterminated");
            } else {
                print_error(scanner_location(sc), "invalid character is detected");
            }
            return SCAN_FAILURE;
        }

        return SCAN_SUCCESS;
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
                print_error(scanner_pre_location(sc), "comment is unterminated");
            } else {
                print_error(scanner_location(sc), "invalid character is detected");
            }
            return SCAN_FAILURE;
        }

        return SCAN_SUCCESS;
    }

    return SCAN_FAILURE;
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
                print_error(scanner_pre_location(sc), "string is unterminated");
            } else {
                print_error(scanner_location(sc), "invalid character is detected");
            }
            return SCAN_FAILURE;
        }

        return TSTRING;
    }

    return SCAN_FAILURE;
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

    return SCAN_FAILURE;
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

    return SCAN_FAILURE;
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
        return SCAN_FAILURE;
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
            return SCAN_FAILURE;
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
                print_token_error(scanner_pre_location(sc), scanner_location(sc), "string needs to be shorter than %d", MAXSTRSIZE);
                return SCAN_FAILURE;
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
                print_token_error(scanner_pre_location(sc), scanner_location(sc), "number needs to be less than 32768", MAXSTRSIZE);
                return SCAN_FAILURE;
            }
            num_attr = (int) num;
            return code;
        }

        /* read name or keyword */
        if (isalpha(scanner_top(sc))) {
            code = scan_name_or_keyword(sc);
            if (scanner_buf_overflow(sc)) {
                print_token_error(scanner_pre_location(sc), scanner_location(sc), "name needs to be shorter than %d", MAXSTRSIZE);
                return SCAN_FAILURE;
            }
            strcpy(string_attr, scanner_buf_data(sc));
            return code;
        }

        /* read symbol */
        if ((code = scan_symbol(sc)) > 0) {
            return code;
        }

        print_error(scanner_location(sc), "invalid character is detected");
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

static void print_message_color(const scan_message_t type)
{
    printf("\033[1m");
    switch (type) {
    case SCAN_WARNING:
        printf("\033[95m");
        break;
    case SCAN_ERROR:
        printf("\033[91m");
        break;
    }
}

static void print_message_impl(const scanner_loc_t *begin, const scanner_loc_t *end, const scan_message_t type, const char *format, va_list args)
{
    fpos_t fpos;
    FILE *file;
    size_t i, j, d, n, cnt;
    int c;

    d = 0;
    for (i = begin->line; i > 0; i /= 10) {
        d++;
    }

    print_message_color(type);
    switch (type) {
    case SCAN_WARNING:
        printf("warning: ");
        break;
    case SCAN_ERROR:
        printf("error: ");
        break;
    }
    printf("\033[0m");

    printf("\033[1m");
    vprintf(format, args);
    printf("\033[0m");
    printf("\n");

    printf("\033[94m");
    for (i = 0; i < d; i++) {
        printf(" ");
    }
    printf("--> ");
    printf("\033[0m");
    printf("%s:%ld:%ld\n", scanner.filename, begin->line, begin->col);

    file = scanner.file;
    fgetpos(file, &fpos);
    fsetpos(file, &begin->fpos);
    fseek(file, -2, SEEK_CUR);

    printf("\033[94m");
    for (i = 0; i < d; i++) {
        printf(" ");
    }
    printf(" | \n");
    printf("\033[0m");

    cnt = 0;
    printf("\033[94m");
    printf("%ld | ", begin->line);
    printf("\033[0m");
    for (i = 0; i < begin->col - 1; i++) {
        c = fgetc(file);
        if (c == '\t') {
            n = 4 - (cnt % 4);
            for (j = 0; j < n; j++) {
                printf(" ");
            }
            cnt += n;
        } else {
            printf("%c", c);
            cnt++;
        }
    }

    print_message_color(type);
    for (i = begin->col; i < end->col; i++) {
        printf("%c", fgetc(file));
    }
    printf("\033[0m");
    while (1) {
        c = fgetc(file);
        if (c == EOF || c == '\n' || c == '\r') {
            break;
        }
        printf("%c", (char) c);
    }
    printf("\n");

    printf("\033[94m");
    for (i = 0; i < d; i++) {
        printf(" ");
    }
    printf(" | ");
    printf("\033[0m");
    for (i = 0; i < cnt; i++) {
        printf(" ");
    }

    print_message_color(type);
    for (i = begin->col; i < end->col; i++) {
        printf("^");
    }
    printf("\033[0m");
    printf("\n");
    printf("\n");

    fsetpos(file, &fpos);
}

void print_message(const scanner_loc_t *loc, const scan_message_t type, const char *format, ...)
{
    scanner_loc_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    print_message_impl(loc, &end, type, format, args);
    va_end(args);
}

void print_warning(const scanner_loc_t *loc, const char *format, ...)
{
    scanner_loc_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    print_message_impl(loc, &end, SCAN_WARNING, format, args);
    va_end(args);
}

void print_error(const scanner_loc_t *loc, const char *format, ...)
{
    scanner_loc_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    print_message_impl(loc, &end, SCAN_ERROR, format, args);
    va_end(args);
}

void print_token_message(const scanner_loc_t *begin, const scanner_loc_t *end, const scan_message_t type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    print_message_impl(begin, end, type, format, args);
    va_end(args);
}

void print_token_warning(const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    print_message_impl(begin, end, SCAN_WARNING, format, args);
    va_end(args);
}

void print_token_error(const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    print_message_impl(begin, end, SCAN_ERROR, format, args);
    va_end(args);
}
