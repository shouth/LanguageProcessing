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

Source *source_new(const char *file_name, unsigned long file_name_length)
{
  Source *source = xmalloc(sizeof(Source));
  {
    source->file_name = xmalloc(file_name_length + 1);
    strncpy(source->file_name, file_name, file_name_length);
    source->file_name[file_name_length] = '\0';
    source->file_name_length            = file_name_length;
  }

  {
    FILE *file = fopen(source->file_name, "rb");
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

      source->text        = text.ptr;
      source->text_length = text.used - 1;

      fflush(stdout);
      fclose(file);
    } else {
      source->text        = NULL;
      source->text_length = -1ul;
    }
  }

  if (source->text) {
    ULongVec line_offsets;
    ULongVec line_lengths;

    unsigned long offset = 0;

    vec_alloc(&line_offsets, 0);
    vec_alloc(&line_lengths, 0);

    vec_push(&line_offsets, &offset, 1);
    while (offset <= source->text_length) {
      unsigned long length = strcspn(source->text + offset, "\r\n");
      if (source->text[offset + length] == '\0') {
        ++length;
      }
      vec_push(&line_lengths, &length, 1);
      offset += length;

      if (offset < source->text_length) {
        if (!strncmp(source->text + offset, "\r\n", 2) || !strncmp(source->text + offset, "\n\r", 2)) {
          offset += 2;
        } else {
          offset += 1;
        }
        vec_push(&line_offsets, &offset, 1);
      }
    }
    source->line_count   = line_offsets.used;
    source->line_offsets = line_offsets.ptr;
    source->line_lengths = line_lengths.ptr;
  } else {
    source->line_count   = -1ul;
    source->line_offsets = NULL;
    source->line_lengths = NULL;
  }

  if (source->file_name && source->text && source->line_offsets) {
    return source;
  } else {
    source_free(source);
    return NULL;
  }
}

void source_free(Source *source)
{
  if (source) {
    free(source->file_name);
    free(source->text);
    free(source->line_offsets);
    free(source->line_lengths);
    free(source);
  }
}

int source_location(const Source *source, unsigned long offset, SourceLocation *location)
{
  if (offset > source->text_length) {
    return 0;
  } else {
    unsigned long left  = 0;
    unsigned long right = source->line_count;
    while (right - left > 1) {
      unsigned long middle = (right - left) / 2 + left;
      if (offset < source->line_offsets[middle]) {
        right = middle;
      } else {
        left = middle;
      }
    }
    if (location) {
      location->line   = left;
      location->column = offset - source->line_offsets[left];
    }
    return 1;
  }
}
