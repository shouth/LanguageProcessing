/*
 * file.c -- file
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_H
#define FILE_H

#include <stddef.h>

struct file {
  char *name;
  char *text;
  size_t text_len;
  size_t *line_offsets;
  size_t line_count;
};

int file_init(struct file *file, const char *path);

void file_deinit(struct file *file);

int file_locate(struct file const *file, size_t offset, size_t *line, size_t *column);

#endif /* FILE_H */
