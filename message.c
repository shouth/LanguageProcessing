#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "mppl.h"

msg_t *new_msg(const source_t *src, region_t region, msg_level_t level, const char *fmt, ...)
{
    msg_t *ret;
    va_list args;

    assert(src && fmt);

    ret = (msg_t *) xmalloc(sizeof(msg_t));
    ret->src = src;
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(ret->msg, fmt, args);
    va_end(args);
    ret->region = region;
    ret->level = level;
    ret->inline_entries = NULL;
    ret->entries = NULL;
    return ret;
}

void delete_msg_entry(msg_entry_t *entry)
{
    if (!entry) {
        return;
    }
    delete_msg_entry(entry->next);
    free(entry);
}

void delete_msg_inline_entry(msg_inline_entry_t *entry)
{
    if (!entry) {
        return;
    }
    delete_msg_inline_entry(entry->next);
    free(entry);
}

void delete_msg(msg_t *msg)
{
    if (!msg) {
        return;
    }
    delete_msg_entry(msg->entries);
    delete_msg_inline_entry(msg->inline_entries);
    free(msg);
}

void msg_add_entry(msg_t *msg, msg_level_t level, const char *fmt, ...)
{
    msg_entry_t *entry;
    msg_entry_t **cur;
    va_list args;

    assert(msg && fmt);

    entry = (msg_entry_t *) xmalloc(sizeof(msg_entry_t));
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(entry->msg, fmt, args);
    va_end(args);
    for (cur = &msg->entries; *cur; cur = &(*cur)->next);
    entry->level = level;
    entry->next = *cur;
    *cur = entry;
}

void msg_add_inline_entry(msg_t *msg, region_t region, const char *fmt, ...)
{
    msg_inline_entry_t *entry;
    msg_inline_entry_t **cur;
    va_list args;

    assert(msg && fmt);

    entry = (msg_inline_entry_t *) xmalloc(sizeof(msg_inline_entry_t));
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(entry->msg, fmt, args);
    va_end(args);
    entry->region = region;
    entry->next = NULL;

    for (cur = &msg->inline_entries; *cur; cur = &(*cur)->next) {
        if (region_compare((*cur)->region, region) >= 0) {
            break;
        }
    }
    entry->next = *cur;
    *cur = entry;
}

void set_level_color(msg_level_t level)
{
    switch (level) {
    case MSG_HELP:
        printf("\033[97m"); /* bright white */
        break;
    case MSG_NOTE:
        printf("\033[94m"); /* bright blue */
        break;
    case MSG_WARN:
        printf("\033[93m"); /* bright yellow */
        break;
    case MSG_ERROR:
        printf("\033[91m"); /* bright red */
        break;
    case MSG_FATAL:
        printf("\033[95m"); /* bright magenta */
        break;
    }
}

void set_bold()
{
    printf("\033[1m");
}

void reset()
{
    printf("\033[0m");
}

void put_sanitized(int c)
{
    if (c == '\t') {
        putchar(' ');
        putchar(' ');
        putchar(' ');
        putchar(' ');
        return;
    }

    if (c == '\r' || c == '\n') {
        putchar(' ');
        return;
    }

    if (!isprint(c)) {
        printf("\033[2m\\%03o\033[22m", (unsigned char) c);
        return;
    }

    putchar(c);
}

const char *level_str(msg_level_t level)
{
    switch (level) {
    case MSG_HELP:
        return "help";
    case MSG_NOTE:
        return "note";
    case MSG_WARN:
        return "warn";
    case MSG_ERROR:
        return "error";
    case MSG_FATAL:
        return "fatal";
    }

    return "";
}

void msg_emit(msg_t *msg)
{
    msg_inline_entry_t *cur0;
    msg_entry_t *cur1;
    int left_margin;
    size_t i, j;
    int has_primary;
    location_t preloc;
    int c, m;

    assert(msg);

    has_primary = 0;
    for (cur0 = msg->inline_entries; cur0; cur0 = cur0->next) {
        if (region_compare(cur0->region, msg->region) == 0) {
            has_primary = 1;
        }
    }
    if (!has_primary) {
        msg_add_inline_entry(msg, msg->region, "");
    }

    for (cur0 = msg->inline_entries; cur0; cur0 = cur0->next) {
        if (cur0->next == NULL) {
            location_t loc = source_location(msg->src, msg->region.pos);
            size_t tmp = loc.line;
            left_margin = 0;
            while (tmp > 0) {
                left_margin++;
                tmp /= 10;
            }
        }
    }

    {
        set_level_color(msg->level);
        set_bold();
        printf("%s", level_str(msg->level));
        reset();
        set_bold();
        printf(": %s\n", msg->msg);
        reset();
    }

    {
        location_t loc = source_location(msg->src, msg->region.pos);
        printf("%*.s", left_margin, "");
        set_bold();
        printf("\033[94m");
        printf("--> ");
        reset();
        printf("%s:%ld:%ld\n", msg->src->input_filename, loc.line, loc.col);
    }

    for (cur0 = msg->inline_entries; cur0; cur0 = cur0->next) {
        location_t begin = source_location(msg->src, cur0->region.pos);
        location_t end = source_location(msg->src, cur0->region.pos + cur0->region.len);
        if (cur0 == msg->inline_entries) {
            set_bold();
            printf("\033[94m");
            printf("%*.s |\n", left_margin, "");
            reset();
        } else if (begin.line - preloc.line > 1) {
            set_bold();
            printf("\033[94m");
            for (i = 0; i < left_margin + 2; i++) {
                putchar('~');
            }
            putchar('\n');
            reset();
        }

        if (begin.line == end.line) {
            size_t offset = msg->src->lines_ptr[begin.line - 1];
            set_bold();
            printf("\033[94m");
            printf("%*.ld |   ", left_margin, begin.line);
            reset();
            for (i = 0; i < begin.col - 1; i++) {
                put_sanitized(msg->src->src_ptr[offset + i]);
            }
            offset += begin.col - 1;

            has_primary = region_compare(cur0->region, msg->region) == 0;
            set_level_color(has_primary ? msg->level : MSG_NOTE);
            for (i = 0; i < cur0->region.len; i++) {
                put_sanitized(msg->src->src_ptr[offset + i]);
            }
            offset += cur0->region.len;
            reset();
            for (; offset < msg->src->lines_ptr[begin.line]; offset++) {
                put_sanitized(msg->src->src_ptr[offset]);
            }
            putchar('\n');

            offset = msg->src->lines_ptr[begin.line - 1];
            set_bold();
            printf("\033[94m");
            printf("%*.s |   ", left_margin, "");
            reset();
            for (i = 0; i < begin.col - 1; i++) {
                c = msg->src->src_ptr[offset + i];
                put_sanitized(c == '\t' ? c : ' ');
            }
            offset += begin.col - 1;

            set_bold();
            set_level_color(has_primary ? msg->level : MSG_NOTE);
            m = has_primary ? '^' : '-';
            for (i = 0; i < cur0->region.len; i++) {
                putchar(m);
                c = msg->src->src_ptr[offset + i];
                if (c == '\t' || !isprint(c)) {
                    putchar(m);
                    putchar(m);
                    putchar(m);
                }
            }
            printf(" %s\n", cur0->msg);
            reset();
        } else {
            size_t offset = msg->src->lines_ptr[begin.line - 1];
            set_bold();
            printf("\033[94m");
            printf("%*.ld |   ", left_margin, begin.line);
            reset();
            for (i = 0; i < begin.col - 1; i++) {
                put_sanitized(msg->src->src_ptr[offset + i]);
            }
            offset += i;

            has_primary = region_compare(cur0->region, msg->region) == 0;
            set_level_color(has_primary ? msg->level : MSG_NOTE);
            for (; offset < msg->src->lines_ptr[begin.line]; offset++) {
                put_sanitized(msg->src->src_ptr[offset]);
            }
            putchar('\n');

            set_bold();
            printf("\033[94m");
            printf("%*.s | _", left_margin, "");
            reset();

            offset = msg->src->lines_ptr[begin.line - 1];
            m = has_primary ? '^' : '-';
            set_bold();
            set_level_color(has_primary ? msg->level : MSG_NOTE);
            for (i = 0; i < begin.col; i++) {
                putchar('_');
                c = msg->src->src_ptr[offset + i];
                if (c == '\t' || !isprint(c)) {
                    putchar('_');
                    putchar('_');
                    putchar('_');
                }
            }
            putchar(m);
            reset();
            putchar('\n');

            for (i = begin.line; i < end.line; i++) {
                size_t line_len = msg->src->lines_ptr[i + 1] - msg->src->lines_ptr[i];
                set_bold();
                set_level_color(has_primary ? msg->level : MSG_NOTE);
                printf("%*.ld | | ", left_margin, i + 1);
                reset();

                set_level_color(has_primary ? msg->level : MSG_NOTE);
                j = 0;
                offset = msg->src->lines_ptr[i];
                if (i == end.line - 1) {
                    for (; j < end.col - 1; j++) {
                        put_sanitized(msg->src->src_ptr[offset + j]);
                    }
                    reset();
                }
                for (; j < line_len; j++) {
                    put_sanitized(msg->src->src_ptr[offset + j]);
                }
                reset();
                putchar('\n');
            }

            set_bold();
            printf("\033[94m");
            printf("%*.s | ", left_margin, "");
            reset();

            set_bold();
            set_level_color(has_primary ? msg->level : MSG_NOTE);
            offset = msg->src->lines_ptr[end.line - 1];
            putchar('|');
            for (i = 0; i < end.col - 1; i++) {
                putchar('_');
                c = msg->src->src_ptr[offset + i];
                if (c == '\t' || !isprint(c)) {
                    putchar('_');
                    putchar('_');
                    putchar('_');
                }
            }
            putchar(m);
            putchar(' ');
            printf("%s", cur0->msg);
            reset();
            putchar('\n');
        }
        preloc = begin;
    }

    for (cur1 = msg->entries; cur1; cur1 = cur1->next) {
        printf("%*.s = ", left_margin, "");
        set_level_color(cur1->level);
        set_bold();
        printf("%s", level_str(cur1->level));
        reset();
        printf(": %s\n", cur1->msg);
    }

    putchar('\n');
    delete_msg(msg);
}
