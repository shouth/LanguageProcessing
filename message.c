#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "mppl.h"

msg_t *new_msg(const source_t *src, size_t pos, size_t len, msg_level_t level, const char *fmt, ...)
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
    ret->pos = pos;
    ret->len = len;
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

void msg_add_inline_entry(msg_t *msg, size_t pos, size_t len, const char *fmt, ...)
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
    entry->pos = pos;
    entry->len = len;
    entry->next = NULL;

    for (cur = &msg->inline_entries; *cur; cur = &(*cur)->next) {
        if ((*cur)->pos > pos || ((*cur)->pos == pos && (*cur)->len <= len)) {
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
    msg_inline_entry_t **cur0;
    msg_entry_t **cur1;
    int left_margin;
    size_t tmp;
    size_t offset;
    size_t i;
    int has_primary;
    location_t loc, preloc;
    int c, m;

    assert(msg);

    has_primary = 0;
    for (cur0 = &msg->inline_entries; *cur0; cur0 = &(*cur0)->next) {
        if ((*cur0)->pos == msg->pos && (*cur0)->len == msg->len) {
            has_primary = 1;
        }
    }
    if (!has_primary) {
        msg_add_inline_entry(msg, msg->pos, msg->len, "");
    }

    for (cur0 = &msg->inline_entries; *cur0; cur0 = &(*cur0)->next) {
        if ((*cur0)->next == NULL) {
            source_location(msg->src, msg->pos, &loc);
            tmp = loc.line;
            left_margin = 0;
            while (tmp > 0) {
                left_margin++;
                tmp /= 10;
            }
        }
    }

    set_level_color(msg->level);
    set_bold();
    printf("%s", level_str(msg->level));
    reset();
    set_bold();
    printf(": %s\n", msg->msg);
    reset();

    source_location(msg->src, msg->pos, &loc);
    printf("%*.s", left_margin, "");
    set_bold();
    printf("\033[94m");
    printf("--> ");
    reset();
    printf("%s:%ld:%ld\n", msg->src->filename, loc.line, loc.col);

    for (cur0 = &msg->inline_entries; *cur0; cur0 = &(*cur0)->next) {
        preloc = loc;
        source_location(msg->src, (*cur0)->pos, &loc);
        if (cur0 == &msg->inline_entries) {
            set_bold();
            printf("\033[94m");
            printf("%*.s |\n", left_margin, "");
            reset();
        } else if (loc.line - preloc.line > 1) {
            set_bold();
            printf("\033[94m");
            for (i = 0; i < left_margin + 2; i++) {
                putchar('~');
            }
            putchar('\n');
            reset();
        }

        offset = msg->src->lines_ptr[loc.line - 1];
        set_bold();
        printf("\033[94m");
        printf("%*.ld | ", left_margin, loc.line);
        reset();
        for (i = 0; i < loc.col - 1; i++) {
            put_sanitized(msg->src->src_ptr[offset + i]);
        }
        offset += loc.col - 1;

        has_primary = (*cur0)->pos == msg->pos && (*cur0)->len == msg->len;
        if (has_primary) {
            set_level_color(msg->level);
        } else {
            set_level_color(MSG_NOTE);
        }
        for (i = 0; i < (*cur0)->len; i++) {
            put_sanitized(msg->src->src_ptr[offset + i]);
        }
        offset += (*cur0)->len;
        reset();
        for (; offset < msg->src->lines_ptr[loc.line]; offset++) {
            put_sanitized(msg->src->src_ptr[offset]);
        }
        putchar('\n');

        offset = msg->src->lines_ptr[loc.line - 1];
        set_bold();
        printf("\033[94m");
        printf("%*.s | ", left_margin, "");
        reset();
        for (i = 0; i < loc.col - 1; i++) {
            c = msg->src->src_ptr[offset + i];
            put_sanitized(c == '\t' ? c : ' ');
        }
        offset += loc.col - 1;

        set_bold();
        if (has_primary) {
            set_level_color(msg->level);
            m = '^';
        } else {
            set_level_color(MSG_NOTE);
            m = '-';
        }
        for (i = 0; i < (*cur0)->len; i++) {
            putchar(m);
            c = msg->src->src_ptr[offset + i];
            if (c == '\t' || !isprint(c)) {
                putchar(m);
                putchar(m);
                putchar(m);
            }
        }
        printf(" %s\n", (*cur0)->msg);
        reset();
    }

    for (cur1 = &msg->entries; *cur1; cur1 = &(*cur1)->next) {
        printf("%*.s = ", left_margin, "");
        set_level_color((*cur1)->level);
        set_bold();
        printf("%s", level_str((*cur1)->level));
        reset();
        printf(": %s\n", (*cur1)->msg);
    }

    putchar('\n');
    delete_msg(msg);
}
