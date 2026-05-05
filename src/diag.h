/*
 * diag.h -- diagnostic messages
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DIAG_H
#define DIAG_H

#include <stddef.h>

#include "ds.h"
#include "src.h"
#include "syn.h"

#if defined(__GNUC__) || defined(__clang__)

#define diag_format(string_index, first_to_check) \
  __attribute__((format(printf, string_index, first_to_check)))

#else

#define diag_format(string_index, first_to_check)

#endif

enum diag_kind {
  DIAG_ERROR,
  DIAG_WARNING,
  DIAG_NOTE
};

struct diag_label {
  size_t start;
  size_t end;
  char *message;
};

struct diag_note {
  char *message;
};

struct diag_entry {
  enum diag_kind kind;
  size_t off;
  char *message;
  vec(struct diag_label) labels;
  vec(struct diag_note) notes;
};

struct diag_report {
  struct src *src;
  vec(struct diag_entry) entries;
};

struct diag {
  vec(struct diag_report) reports;
};

void diag_init(struct diag *d);

void diag_deinit(struct diag *d);

void diag_print(struct diag const *d, FILE *out);

struct diag_report *diag_add_report(struct diag *d, struct src *src);

struct diag_entry *diag_add_entry(struct diag_report *report, enum diag_kind kind, size_t off, char const *fmt, ...) diag_format(4, 5);

struct diag_label *diag_add_label(struct diag_entry *entry, size_t start, size_t end, char const *fmt, ...) diag_format(4, 5);

/* lex */

void diag_error_stray_char(struct diag *d, struct src *src, size_t off, int stray, syn_kinds_t const *expected);

void diag_error_nongraphic_char(struct diag *d, struct src *src, size_t off, int nongraphic);

void diag_error_unterminated_string(struct diag *d, struct src *src, size_t off, size_t len);

void diag_error_unterminated_comment(struct diag *d, struct src *src, size_t off, size_t len);

void diag_error_too_large_integer(struct diag *d, struct src *src, size_t off, size_t len);

/* parse */

void diag_error_unexpected_token(struct diag *d, struct src *src, size_t off, size_t len, char const *found, syn_kinds_t const *expected);

void diag_error_expected(struct diag *d, struct src *src, size_t off, size_t len, char const *found, char const *expected);

void diag_error_break_outside_loop(struct diag *d, struct src *src, size_t off, size_t len);

#endif /* DIAG_H */
