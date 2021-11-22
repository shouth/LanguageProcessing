#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "message.h"

msg_t *msg_new(const source_t *src, size_t pos, size_t len, msg_level_t level, const char *fmt, ...)
{
    msg_t *ret;
    va_list args;

    assert(src != NULL && fmt != NULL);

    ret = (msg_t *) malloc(sizeof(msg_t));
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
}

void msg_free(msg_t *msg)
{
    msg_entry_t **cur0, **precur0;
    msg_inline_entry_t **cur1, **precur1;

    if (msg == NULL) {
        return;
    }

    precur0 = &msg->entries;
    for (cur0 = precur0; *cur0 != NULL; cur0 = &(*cur0)->next) {
        if (cur0 != precur0) {
            free(precur0);
        }
        precur0 = cur0;
    }

    precur1 = &msg->inline_entries;
    for (cur1 = precur1; *cur1 != NULL; cur1 = &(*cur1)->next) {
        if (cur1 != precur1) {
            free(precur1);
        }
        precur1 = cur1;
    }

    free(msg);
}

void msg_add_entry(msg_t *msg, msg_level_t level, const char *fmt, ...)
{
    msg_entry_t *entry;
    msg_entry_t **cur;
    va_list args;

    assert(msg != NULL && fmt != NULL);

    entry = (msg_entry_t *) malloc(sizeof(msg_entry_t));
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(entry->msg, fmt, args);
    va_end(args);
    entry->next = NULL;
    for (cur = &entry->next; *cur != NULL; cur = &(*cur)->next);
    *cur = entry;
}

void msg_add_inline_entry(msg_t *msg, size_t pos, size_t len, const char *fmt, ...)
{
    msg_inline_entry_t *entry;
    msg_inline_entry_t **cur, **precur;
    va_list args;

    assert(msg != NULL && fmt != NULL);

    entry = (msg_inline_entry_t *) malloc(sizeof(msg_inline_entry_t));
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(entry->msg, fmt, args);
    va_end(args);
    entry->pos = pos;
    entry->len = len;
    entry->next = NULL;

    precur = &msg->inline_entries;
    for (cur = precur; *cur != NULL; cur = &(*cur)->next) {
        if ((*cur)->pos >= pos && (*cur)->len <= len) {
            break;
        }
        precur = cur;
    }

    if (*precur != NULL) {
        entry->next = (*precur)->next;
    }
    *precur = entry;
}

void set_level_color(msg_level_t level)
{
    switch (level) {
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

void reset_color()
{
    printf("\033[0m");
}

const char *level_str(msg_level_t level)
{
    switch (level) {
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
    size_t line, col;
    size_t offset;
    size_t i;
    int has_primary;

    assert(msg != NULL);

    has_primary = 0;
    for (cur0 = &msg->inline_entries; *cur0 != NULL; cur0 = &(*cur0)->next) {
        if ((*cur0)->pos == msg->pos && (*cur0)->len == msg->len) {
            has_primary = 1;
        }
    }
    if (!has_primary) {
        msg_add_inline_entry(msg, msg->pos, msg->len, "");
    }

    for (cur0 = &msg->inline_entries; *cur0 != NULL; cur0 = &(*cur0)->next) {
        if ((*cur0)->next == NULL) {
            source_location(msg->src, msg->pos, &tmp, NULL);
            left_margin = 0;
            while (tmp > 0) {
                left_margin++;
                tmp /= 10;
            }
        }
    }

    set_level_color(msg->level);
    printf("%s", level_str(msg->level));
    reset_color();
    printf(": %s\n", msg->msg);

    source_location(msg->src, msg->pos, &line, &col);
    printf("%*.s", left_margin, "");
    printf("--> %s:%ld:%ld\n", msg->src->filename, line + 1, col);

    for (cur0 = &msg->inline_entries; *cur0 != NULL; cur0 = &(*cur0)->next) {
        source_location(msg->src, (*cur0)->pos, &line, &col);
        printf("%*.s |\n", left_margin, "");

        offset = msg->src->lines_ptr[line];
        printf("%*.ld | ", left_margin, line + 1);
        printf("%*.s", (int) col, msg->src->src_ptr + offset);
        offset += col;

        has_primary = (*cur0)->pos == msg->pos && (*cur0)->len == msg->len;
        if (has_primary) {
            set_level_color(msg->level);
        } else {
            set_level_color(MSG_NOTE);
        }
        printf("%*.s", (int) (*cur0)->len, msg->src->src_ptr + offset);
        offset += (*cur0)->len;
        reset_color();
        printf("%*.s\n", (int) (msg->src->lines_ptr[line + 1] - offset), msg->src->src_ptr + offset);

        printf("%*.s | ", left_margin, "");
        printf("%*.s", (int) col, "");
        if (has_primary) {
            set_level_color(msg->level);
            for (i = 0; i < (*cur0)->len; i++) {
                putchar('^');
            }
        } else {
            set_level_color(MSG_NOTE);
            for (i = 0; i < (*cur0)->len; i++) {
                putchar('-');
            }
        }
        reset_color();
        printf(" %s\n", (*cur0)->msg);
    }

    for (cur1 = &msg->entries; *cur1 != NULL; cur1 = &(*cur1)->next) {
        printf("%*.s = ", left_margin, "");
        set_level_color((*cur1)->level);
        printf("%s", level_str((*cur1)->level));
        reset_color();
        printf(": %s\n", (*cur1)->msg);
    }

    msg_free(msg);
}
