#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "message.h"

msg_t *msg_new(const source_t *src, size_t pos, size_t len, const char *fmt, ...)
{
    msg_t *ret = (msg_t *) malloc(sizeof(msg_t));
    va_list args;

    ret->src = src;
    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(ret->msg, fmt, args);
    va_end(args);

    ret->pos = pos;
    ret->len = len;

    ret->inline_entries = NULL;
    ret->entries = NULL;
}

void msg_free(msg_t *msg)
{
    msg_entry_t **cur0, **precur0;
    msg_inline_entry_t **cur1, **precur1;

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
    msg_entry_t *entry = (msg_entry_t *) malloc(sizeof(msg_entry_t));
    msg_entry_t **cur;
    va_list args;

    va_start(args, fmt);
    /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
    vsprintf(entry->msg, fmt, args);
    va_end(args);

    entry->next = NULL;

    for (cur = &entry->next; *cur != NULL; cur = &(*cur)->next);
    *cur = entry;
}

void msg_add_inline_entry(msg_t *msg, size_t pos, size_t len, msg_level_t level, const char *fmt, ...)
{
    msg_inline_entry_t *entry = (msg_inline_entry_t *) malloc(sizeof(msg_inline_entry_t));
    msg_inline_entry_t **cur, **precur;
    va_list args;

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

void set_color(msg_level_t level)
{
    switch (level) {
    case MSG_NOTE:
        printf("\031[94m"); /* bright blue */
        break;

    case MSG_WARN:
        printf("\031[93m"); /* bright yellow */
        break;

    case MSG_ERROR:
        printf("\031[91m"); /* bright red */
        break;

    case MSG_FATAL:
        printf("\031[95m"); /* bright magenta */
        break;
    }
}

void reset_color()
{
    printf("\031[0m");
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

void msg_emit(const msg_t *msg)
{
    set_color(msg->level);
    printf("%s", level_str(msg->level));
    reset_color();
    printf(": %s", msg->msg);
}
