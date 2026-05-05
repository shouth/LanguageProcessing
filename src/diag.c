/*
 * diag.c -- diagnostic messages
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "diag.h"
#include "ds.h"
#include "src.h"
#include "syn.h"

static char *diag_vasprintf(char const *fmt, va_list args)
{
  char *result = NULL;
  char *buffer = NULL;
  long len = 0;
  FILE *tmp = NULL;

  if (!(tmp = tmpfile())) {
    goto cleanup;
  }

  if ((len = vfprintf(tmp, fmt, args)) < 0) {
    goto cleanup;
  }

  rewind(tmp);

  if (!(buffer = malloc(len + 1))) {
    goto cleanup;
  }

  if (fread(buffer, 1, len, tmp) != (size_t) len) {
    goto cleanup;
  }
  buffer[len] = '\0';

  result = buffer;
  buffer = NULL;

cleanup:
  free(buffer);
  if (tmp) {
    fclose(tmp);
  }

  return result;
}

static char const *diag_syn_kinds_to_string(syn_kinds_t const *kinds)
{
  static char buffer[256];
  size_t off = 0;
  size_t count = bits_count(kinds);
  size_t i;

  if (count > 1) {
    off += sprintf(buffer + off, "one of ");
  }

  for (i = TOK_BEGIN; i < TOK_END; i++) {
    if (bits_test(kinds, i)) {
      if (i == SYN_IDENT) {
        off += sprintf(buffer + off, "identifier");
      } else if (i == SYN_NUMBER_LIT) {
        off += sprintf(buffer + off, "number literal");
      } else if (i == SYN_STRING_LIT) {
        off += sprintf(buffer + off, "string literal");
      } else {
        off += sprintf(buffer + off, "`%s`", syn_kind_to_string(i));
      }
      --count;
      if (count == 1) {
        off += sprintf(buffer + off, " or ");
      } else if (count > 1) {
        off += sprintf(buffer + off, ", ");
      }
    }
  }
  buffer[off] = '\0';

  return buffer;
}

void diag_init(struct diag *d)
{
  vec_init(&d->reports);
}

void diag_deinit(struct diag *d)
{
  size_t i, j, k;

  for (i = 0; i < d->reports.count; i++) {
    struct diag_report *report = vec_at(&d->reports, i);

    for (j = 0; j < report->entries.count; j++) {
      struct diag_entry *entry = vec_at(&report->entries, j);

      for (k = 0; k < entry->labels.count; k++) {
        struct diag_label *label = vec_at(&entry->labels, k);
        free(label->message);
      }
      vec_deinit(&entry->labels);

      for (k = 0; k < entry->notes.count; k++) {
        struct diag_note *note = vec_at(&entry->notes, k);
        free(note->message);
      }
      vec_deinit(&entry->notes);

      free(entry->message);
    }
    vec_deinit(&report->entries);
  }
  vec_deinit(&d->reports);
}

void diag_print(struct diag const *d, FILE *out)
{
  /* TODO: implement detailed diagnostic printing */

  size_t i, j;

  for (i = 0; i < d->reports.count; i++) {
    struct diag_report *report = vec_at(&d->reports, i);

    for (j = 0; j < report->entries.count; j++) {
      struct diag_entry *entry = vec_at(&report->entries, j);
      fprintf(out, "%s:%lu: %s\n", report->src->name, entry->off, entry->message ? entry->message : "");
    }
  }
}

struct diag_report *diag_add_report(struct diag *d, struct src *src)
{
  struct diag_report report;
  report.src = src;
  vec_init(&report.entries);
  vec_push(&d->reports, &report);
  return vec_back(&d->reports);
}

struct diag_entry *diag_add_entry(struct diag_report *report, enum diag_kind kind, size_t off, char const *fmt, ...)
{
  struct diag_entry entry;
  entry.kind = kind;
  entry.off = off;
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    entry.message = diag_vasprintf(fmt, args);
    va_end(args);
  } else {
    entry.message = NULL;
  }
  vec_init(&entry.labels);
  vec_init(&entry.notes);
  vec_push(&report->entries, &entry);
  return vec_back(&report->entries);
}

struct diag_label *diag_add_label(struct diag_entry *entry, size_t start, size_t end, char const *fmt, ...)
{
  struct diag_label label;
  label.start = start;
  label.end = end;
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    label.message = diag_vasprintf(fmt, args);
    va_end(args);
  } else {
    label.message = NULL;
  }
  vec_push(&entry->labels, &label);
  return vec_back(&entry->labels);
}

/* lex */

void diag_error_stray_char(struct diag *d, struct src *src, size_t off, int stray, syn_kinds_t const *expected)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "stray `%c` in program", stray);
  diag_add_label(entry, off, off + 1, "expected %s, found `%c`", diag_syn_kinds_to_string(expected), stray);
}

void diag_error_nongraphic_char(struct diag *d, struct src *src, size_t off, int nongraphic)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "nongraphic character `\\%02x` in program", nongraphic);
  diag_add_label(entry, off, off + 1, NULL);
}

void diag_error_unterminated_string(struct diag *d, struct src *src, size_t off, size_t len)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "string is not terminated");
  diag_add_label(entry, off, off + len, NULL);
}

void diag_error_unterminated_comment(struct diag *d, struct src *src, size_t off, size_t len)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "comment is not terminated");
  diag_add_label(entry, off, off + len, NULL);
}

void diag_error_too_large_integer(struct diag *d, struct src *src, size_t off, size_t len)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "integer literal is too large");
  diag_add_label(entry, off, off + len, "integer literal needs to be less than or equal to 32768");
}

/* parse */

void diag_error_unexpected_token(struct diag *d, struct src *src, size_t off, size_t len, char const *found, syn_kinds_t const *expected)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "unexpected token `%.*s`", (int) len, found);
  diag_add_label(entry, off, off + len, "expected %s, found `%.*s`", diag_syn_kinds_to_string(expected), (int) len, found);
}

void diag_error_expected(struct diag *d, struct src *src, size_t off, size_t len, char const *found, char const *expected)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "expected %s", expected);
  diag_add_label(entry, off, off + len, "expected %s, found `%.*s`", expected, (int) len, found);
}

void diag_error_break_outside_loop(struct diag *d, struct src *src, size_t off, size_t len)
{
  struct diag_report *report = diag_add_report(d, src);
  struct diag_entry *entry = diag_add_entry(report, DIAG_ERROR, off, "`break` outside of loop");
  diag_add_label(entry, off, off + len, "`break` can only be used inside loops");
}
