/* SPDX-License-Identifier: Apache-2.0 */

#ifndef SOURCE_H
#define SOURCE_H

#include <stddef.h>

typedef struct SourceLocation SourceLocation;
typedef struct SourceRange    SourceRange;
typedef struct Source         Source;

struct SourceLocation {
  size_t line;
  size_t column;
};

struct SourceRange {
  size_t offset;
  size_t length;
};

struct Source {
  char *filename;
  char *text;
  size_t text_length;
  size_t line_count;
  size_t *offset_tree;
};

Source *source_new(char const *filename, size_t filename_len);
void    source_free(Source *source);
int     source_offset_location(Source const *source, size_t offset, SourceLocation *location);
int     source_line_range(Source const *source, size_t line, SourceRange *range);

#endif /* SOURCE_H */
