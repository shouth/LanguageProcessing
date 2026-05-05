/*
 * src.c -- source code
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_H
#define SRC_H

#include <stddef.h>

struct src {
  char *name;
  char *text;
  size_t text_len;
  size_t *line_offsets;
  size_t line_count;
};

int src_init(struct src *src, const char *path);

void src_deinit(struct src *src);

int src_locate(struct src *src, size_t offset, size_t *line, size_t *column);

#endif /* SRC_H */
