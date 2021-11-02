#include <stdarg.h>
#include "scanner-message.h"

static size_t digits_len(unsigned long int d)
{
    size_t ret = 0;
    for (; d > 0; d /= 10) {
        ret++;
    }
    return ret;
}

static void color_message(const scan_message_t type)
{
    printf("\033[1m");
    switch (type) {
    case SCAN_WARNING:
        printf("\033[93m");
        break;
    case SCAN_ERROR:
        printf("\033[91m");
        break;
    }
}

static void reset_color_message()
{
    printf("\033[0m");
}

static void main_message(const scan_message_t type, const char *format, va_list args)
{
    color_message(type);
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
}

static void location_message(const scanner_t *sc, const location_t *sloc)
{
    printf("\033[94m");
    printf("%*.s--> ", (int) digits_len(sloc->line), " ");
    printf("\033[0m");
    printf("%s:%ld:%ld", sc->filename, sloc->line, sloc->col);
}

static void line_number_message(size_t line_number, size_t len)
{
    printf("\033[94m");
    if (line_number != 0) {
        printf("%*.ld | ", (int) len, line_number);
    } else {
        printf("%*.s | ", (int) len, " ");
    }
    printf("\033[0m");
}

static void newline_message()
{
    printf("\n");
}

static size_t file_line_message(FILE *file, size_t len)
{
    size_t i, n;
    size_t cnt;
    int c;

    cnt = 0;
    for (i = 0; i < len; i++) {
        c = fgetc(file);
        if (c == '\t') {
            n = 4 - (cnt % 4);
            printf("%*.s", (int) n, " ");
            cnt += n;
        } else {
            printf("%c", c);
            cnt++;
        }
    }

    return cnt;
}

static size_t file_remaining_line_message(FILE *file)
{
    size_t n;
    size_t cnt;
    int c;

    while (1) {
        c = fgetc(file);
        if (c == EOF || c == '\n' || c == '\r') {
            break;
        }
        if (c == '\t') {
            n = 4 - (cnt % 4);
            printf("%*.s", (int) n, " ");
            cnt += n;
        } else {
            printf("%c", c);
            cnt++;
        }
    }

    return cnt;
}

static void indicator_message(size_t offset, size_t len)
{
    size_t i;

    printf("%*.s", (int) offset, " ");
    for (i = 0; i < len; i++) {
        printf("^");
    }
}

static void message_impl(
    const scanner_t *sc,
    const location_t *begin,
    const location_t *end,
    const scan_message_t type,
    const char *format, va_list args)
{
    fpos_t fpos;
    FILE *file;
    size_t i, cnt;

    main_message(type, format, args);
    newline_message();
    location_message(sc, begin);
    newline_message();

    line_number_message(0, digits_len(begin->line));
    newline_message();

    file = sc->file;
    fgetpos(file, &fpos);
    fsetpos(file, &begin->fpos);
    fseek(file, -(begin->col + 1), SEEK_CUR);

    line_number_message(begin->line, digits_len(begin->line));
    cnt = file_line_message(file, begin->col - 1);
    color_message(type);
    file_line_message(file, end->col - begin->col);
    reset_color_message();
    file_remaining_line_message(file);
    newline_message();

    line_number_message(0, digits_len(begin->line));
    color_message(type);
    indicator_message(cnt, end->col - begin->col);
    reset_color_message();
    newline_message();
    newline_message();

    fsetpos(file, &fpos);
}

void message(const scanner_t *sc, const location_t *loc, const scan_message_t type, const char *format, ...)
{
    location_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    message_impl(sc, loc, &end, type, format, args);
    va_end(args);
}

void message_warning(const scanner_t *sc, const location_t *loc, const char *format, ...)
{
    location_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    message_impl(sc, loc, &end, SCAN_WARNING, format, args);
    va_end(args);
}

void message_error(const scanner_t *sc, const location_t *loc, const char *format, ...)
{
    location_t end;
    va_list args;

    end = *loc;
    end.col++;
    va_start(args, format);
    message_impl(sc, loc, &end, SCAN_ERROR, format, args);
    va_end(args);
}

void message_token(const scanner_t *sc, const location_t *begin, const location_t *end, const scan_message_t type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    message_impl(sc, begin, end, type, format, args);
    va_end(args);
}

void message_token_warning(const scanner_t *sc, const location_t *begin, const location_t *end, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    message_impl(sc, begin, end, SCAN_WARNING, format, args);
    va_end(args);
}

void message_token_error(const scanner_t *sc, const location_t *begin, const location_t *end, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    message_impl(sc, begin, end, SCAN_ERROR, format, args);
    va_end(args);
}
