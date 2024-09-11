/* SPDX-License-Identifier: Apache-2.0 */

#ifndef SOURCE_H
#define SOURCE_H

#include "utility.h"

typedef struct SourceLocation SourceLocation;
typedef struct SourceRange    SourceRange;
typedef struct Source         Source;

typedef Slice(SourceRange) SourceRangeSlice;

struct SourceLocation {
  unsigned long line;
  unsigned long column;
};

struct SourceRange {
  unsigned long offset;
  unsigned long span;
};

struct Source {
  Slice(char) filename;
  Slice(char) text;
  SourceRangeSlice lines;
};

Source *source_new(const char *filename, unsigned long filename_len);
void    source_free(Source *source);
int     source_location(const Source *source, unsigned long offset, SourceLocation *location);

#endif
