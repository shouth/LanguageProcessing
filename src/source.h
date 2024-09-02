/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef SOURCE_H
#define SOURCE_H

#include "utility.h"

typedef struct SourceLocation SourceLocation;
typedef struct SourceRange    SourceRange;
typedef struct Source         Source;

typedef Seq(SourceRange) SourceRangeSeq;

struct SourceLocation {
  unsigned long line;
  unsigned long column;
};

struct SourceRange {
  unsigned long offset;
  unsigned long span;
};

struct Source {
  Seq(char) filename;
  Seq(char) text;
  SourceRangeSeq lines;
};

Source *source_new(const char *filename, unsigned long filename_len);
void    source_free(Source *source);
int     source_location(const Source *source, unsigned long offset, SourceLocation *location);

#endif
