#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "message.h"

msg_t *new_msg(const source_t *src, region_t region, msg_level_t level, const char *fmt, ...)
{
  msg_t  *ret;
  va_list args;

  assert(src && fmt);

  ret      = (msg_t *) xmalloc(sizeof(msg_t));
  ret->src = src;
  va_start(args, fmt);
  /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
  vsprintf(ret->msg, fmt, args);
  va_end(args);
  ret->region         = region;
  ret->level          = level;
  ret->inline_entries = NULL;
  ret->entries        = NULL;
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
  msg_entry_t  *entry;
  msg_entry_t **cur;
  va_list       args;

  assert(msg && fmt);

  entry = (msg_entry_t *) xmalloc(sizeof(msg_entry_t));
  va_start(args, fmt);
  /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
  vsprintf(entry->msg, fmt, args);
  va_end(args);
  for (cur = &msg->entries; *cur; cur = &(*cur)->next)
    ;
  entry->level = level;
  entry->next  = *cur;
  *cur         = entry;
}

void msg_add_inline_entry(msg_t *msg, region_t region, const char *fmt, ...)
{
  msg_inline_entry_t  *entry;
  msg_inline_entry_t **cur;
  va_list              args;

  assert(msg && fmt);

  entry = (msg_inline_entry_t *) xmalloc(sizeof(msg_inline_entry_t));
  va_start(args, fmt);
  /* v`n`sprintf is preferred to vsprintf here, but v`n`sprintf is unavailable in C89. */
  vsprintf(entry->msg, fmt, args);
  va_end(args);
  entry->region = region;
  entry->next   = NULL;

  for (cur = &msg->inline_entries; *cur; cur = &(*cur)->next) {
    if (region_compare((*cur)->region, region) >= 0) {
      break;
    }
  }
  entry->next = *cur;
  *cur        = entry;
}

void set_level_color(msg_level_t level)
{
  switch (level) {
  case MSG_HELP:
    term_set(SGR_FG_BRIGHT_WHITE);
    break;
  case MSG_NOTE:
    term_set(SGR_FG_BRIGHT_BLUE);
    break;
  case MSG_WARN:
    term_set(SGR_FG_BRIGHT_YELLOW);
    break;
  case MSG_ERROR:
    term_set(SGR_FG_BRIGHT_RED);
    break;
  case MSG_FATAL:
    term_set(SGR_FG_BRIGHT_MAGENTA);
    break;
  }
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
    term_set(SGR_FAINT);
    printf("\\%03o", (unsigned char) c);
    term_set(SGR_NORMAL_INTENSITY);
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
  msg_entry_t        *cur1;
  int                 left_margin;
  long                i, j;
  int                 has_primary;
  location_t          preloc;
  int                 c, m;

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
      location_t loc = src_location(msg->src, cur0->region.pos);
      long       tmp = loc.line;
      left_margin    = 0;
      while (tmp > 0) {
        left_margin++;
        tmp /= 10;
      }
    }
  }

  {
    set_level_color(msg->level);
    term_set(SGR_BOLD);
    printf("%s", level_str(msg->level));
    term_set(SGR_RESET);
    term_set(SGR_BOLD);
    printf(": %s\n", msg->msg);
    term_set(SGR_RESET);
  }

  {
    location_t loc = src_location(msg->src, msg->region.pos);
    printf("%*.s", left_margin, "");
    term_set(SGR_BOLD);
    term_set(SGR_FG_BRIGHT_BLUE);
    printf("--> ");
    term_set(SGR_RESET);
    printf("%s:%ld:%ld\n", msg->src->filename, loc.line, loc.col);
  }

  for (cur0 = msg->inline_entries; cur0; cur0 = cur0->next) {
    location_t begin = src_location(msg->src, cur0->region.pos);
    location_t end   = src_location(msg->src, cur0->region.pos + cur0->region.len);
    if (cur0 == msg->inline_entries) {
      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.s |\n", left_margin, "");
      term_set(SGR_RESET);
    } else if (begin.line - preloc.line > 1) {
      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      for (i = 0; i < left_margin + 2; i++) {
        putchar('~');
      }
      putchar('\n');
      term_set(SGR_RESET);
    }

    if (begin.line == end.line) {
      long offset = msg->src->lines[begin.line - 1];
      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.ld |   ", left_margin, begin.line);
      term_set(SGR_RESET);
      for (i = 0; i < begin.col - 1; i++) {
        put_sanitized(msg->src->src[offset + i]);
      }
      offset += begin.col - 1;

      has_primary = region_compare(cur0->region, msg->region) == 0;
      set_level_color(has_primary ? msg->level : MSG_NOTE);
      for (i = 0; i < cur0->region.len; i++) {
        put_sanitized(msg->src->src[offset + i]);
      }
      offset += cur0->region.len;
      term_set(SGR_RESET);
      for (; offset < msg->src->lines[begin.line]; offset++) {
        put_sanitized(msg->src->src[offset]);
      }
      putchar('\n');

      offset = msg->src->lines[begin.line - 1];
      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.s |   ", left_margin, "");
      term_set(SGR_RESET);
      for (i = 0; i < begin.col - 1; i++) {
        c = msg->src->src[offset + i];
        put_sanitized(c == '\t' ? c : ' ');
      }
      offset += begin.col - 1;

      term_set(SGR_BOLD);
      set_level_color(has_primary ? msg->level : MSG_NOTE);
      m = has_primary ? '^' : '-';
      for (i = 0; i < cur0->region.len; i++) {
        putchar(m);
        c = msg->src->src[offset + i];
        if (c == '\t' || !isprint(c)) {
          putchar(m);
          putchar(m);
          putchar(m);
        }
      }
      printf(" %s\n", cur0->msg);
      term_set(SGR_RESET);
    } else {
      long offset = msg->src->lines[begin.line - 1];
      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.ld |   ", left_margin, begin.line);
      term_set(SGR_RESET);
      for (i = 0; i < begin.col - 1; i++) {
        put_sanitized(msg->src->src[offset + i]);
      }
      offset += i;

      has_primary = region_compare(cur0->region, msg->region) == 0;
      set_level_color(has_primary ? msg->level : MSG_NOTE);
      for (; offset < msg->src->lines[begin.line]; offset++) {
        put_sanitized(msg->src->src[offset]);
      }
      putchar('\n');

      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.s | _", left_margin, "");
      term_set(SGR_RESET);

      offset = msg->src->lines[begin.line - 1];
      m      = has_primary ? '^' : '-';
      term_set(SGR_BOLD);
      set_level_color(has_primary ? msg->level : MSG_NOTE);
      for (i = 0; i < begin.col; i++) {
        putchar('_');
        c = msg->src->src[offset + i];
        if (c == '\t' || !isprint(c)) {
          putchar('_');
          putchar('_');
          putchar('_');
        }
      }
      putchar(m);
      term_set(SGR_RESET);
      putchar('\n');

      for (i = begin.line; i < end.line; i++) {
        long line_len = msg->src->lines[i + 1] - msg->src->lines[i];
        term_set(SGR_BOLD);
        set_level_color(has_primary ? msg->level : MSG_NOTE);
        printf("%*.ld | | ", left_margin, i + 1);
        term_set(SGR_RESET);

        set_level_color(has_primary ? msg->level : MSG_NOTE);
        j      = 0;
        offset = msg->src->lines[i];
        if (i == end.line - 1) {
          for (; j < end.col - 1; j++) {
            put_sanitized(msg->src->src[offset + j]);
          }
          term_set(SGR_RESET);
        }
        for (; j < line_len; j++) {
          put_sanitized(msg->src->src[offset + j]);
        }
        term_set(SGR_RESET);
        putchar('\n');
      }

      term_set(SGR_BOLD);
      term_set(SGR_FG_BRIGHT_BLUE);
      printf("%*.s | ", left_margin, "");
      term_set(SGR_RESET);

      term_set(SGR_BOLD);
      set_level_color(has_primary ? msg->level : MSG_NOTE);
      offset = msg->src->lines[end.line - 1];
      putchar('|');
      for (i = 0; i < end.col - 1; i++) {
        putchar('_');
        c = msg->src->src[offset + i];
        if (c == '\t' || !isprint(c)) {
          putchar('_');
          putchar('_');
          putchar('_');
        }
      }
      putchar(m);
      putchar(' ');
      printf("%s", cur0->msg);
      term_set(SGR_RESET);
      putchar('\n');
    }
    preloc = begin;
  }

  for (cur1 = msg->entries; cur1; cur1 = cur1->next) {
    printf("%*.s = ", left_margin, "");
    set_level_color(cur1->level);
    term_set(SGR_BOLD);
    printf("%s", level_str(cur1->level));
    term_set(SGR_RESET);
    printf(": %s\n", cur1->msg);
  }

  putchar('\n');
  delete_msg(msg);
}
