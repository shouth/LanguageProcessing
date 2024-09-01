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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source.h"
#include "utility.h"

Source *source_new(const char *filename, unsigned long filename_len)
{
  Source *source = xmalloc(sizeof(Source));

  seq_alloc(&source->filename, filename_len + 1);
  strncpy(source->filename.ptr, filename, filename_len);
  source->filename.ptr[filename_len] = '\0';

  {
    FILE *file = fopen(source->filename.ptr, "rb");
    if (file) {
      CharVec       text;
      char          buffer[4096];
      unsigned long length;

      vec_alloc(&text, 0);
      while ((length = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
        vec_push(&text, buffer, length);
      }
      buffer[0] = '\0';
      vec_push(&text, buffer, 1);

      source->text.ptr   = text.ptr;
      source->text.count = text.used - 1;

      fflush(stdout);
      fclose(file);
    } else {
      source->text.ptr   = NULL;
      source->text.count = 0;
    }
  }

  if (source->text.ptr) {
    typedef Vec(SourceRange) SourceRangeVec;

    unsigned long offset = 0;

    SourceRangeVec lines;
    vec_alloc(&lines, 0);

    while (offset <= source->text.count) {
      SourceRange line;

      unsigned long span = strcspn(source->text.ptr + offset, "\r\n");
      if (source->text.ptr[offset + span] == '\0') {
        ++span;
      }

      line.offset = offset;
      line.span   = span;
      vec_push(&lines, &line, 1);

      offset += span;
      if (offset < source->text.count) {
        if (!strncmp(source->text.ptr + offset, "\r\n", 2) || !strncmp(source->text.ptr + offset, "\n\r", 2)) {
          offset += 2;
        } else {
          offset += 1;
        }
      }
    }
    source->lines.ptr   = lines.ptr;
    source->lines.count = lines.used;
  } else {
    source->lines.ptr   = NULL;
    source->lines.count = 0;
  }

  if (source->filename.ptr && source->text.ptr && source->lines.ptr) {
    return source;
  } else {
    source_free(source);
    return NULL;
  }
}

void source_free(Source *source)
{
  if (source) {
    seq_free(&source->filename);
    seq_free(&source->text);
    seq_free(&source->lines);
    free(source);
  }
}

int source_location(const Source *source, unsigned long offset, SourceLocation *location)
{
  if (offset > source->text.count) {
    return 0;
  } else {
    unsigned long left  = 0;
    unsigned long right = source->lines.count;
    while (right - left > 1) {
      unsigned long middle = (right - left) / 2 + left;
      if (offset < source->lines.ptr[middle].offset) {
        right = middle;
      } else {
        left = middle;
      }
    }
    if (location) {
      location->line   = left;
      location->column = offset - source->lines.ptr[left].offset;
    }
    return 1;
  }
}
