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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "canvas.h"
#include "term.h"
#include "utility.h"

struct CanvasCell {
  char      character[4];
  int       size;
  TermStyle style;
};

struct Canvas {
  Array        *lines;
  unsigned long current_line;
  unsigned long current_column;
};

Canvas *canvas_new(void)
{
  Canvas *canvas = malloc(sizeof(Canvas));
  Array  *line   = array_new(sizeof(CanvasCell));
  canvas->lines  = array_new(sizeof(Array *));
  array_push(canvas->lines, &line);
  canvas->current_line   = 0;
  canvas->current_column = 0;
  return canvas;
}

void canvas_free(Canvas *canvas)
{
  if (canvas) {
    unsigned long i;
    for (i = 0; i < array_count(canvas->lines); ++i) {
      Array **line = array_at(canvas->lines, i);
      array_free(*line);
    }
    array_free(canvas->lines);
    free(canvas);
  }
}

#define BUFFER_SIZE 1024

void canvas_next_line(Canvas *canvas)
{
  ++canvas->current_line;
  canvas->current_column = 0;
  if (canvas->current_line >= array_count(canvas->lines)) {
    Array *line = array_new(sizeof(CanvasCell));
    array_push(canvas->lines, &line);
  }
}

void canvas_write(Canvas *canvas, const TermStyle *style, const char *format, ...)
{
  va_list args;
  Array **line  = array_at(canvas->lines, canvas->current_line);
  FILE   *file  = tmpfile();
  long    index = 0;
  char    buffer[BUFFER_SIZE + 1];

  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  rewind(file);
  while (fgets(buffer + index, BUFFER_SIZE - index, file)) {
    while (buffer[index]) {
      long size = utf8_len(buffer + index, BUFFER_SIZE - index);
      if (size < 0) {
        int remain = strlen(buffer + index);
        memmove(buffer, buffer + index, remain);
        index = remain;
        break;
      }

      {
        unsigned long initial_line_width = array_count(*line);
        CanvasCell    cell;
        cell.style = *style;
        cell.size  = size;
        memcpy(cell.character, buffer + index, size);
        if (canvas->current_column < initial_line_width) {
          memcpy(array_at(*line, canvas->current_column), &cell, sizeof(CanvasCell));
        } else {
          array_push(*line, &cell);
        }
        ++canvas->current_column;
        index += size;
      }
    }
  }
  fclose(file);
}

unsigned long canvas_line(const Canvas *canvas)
{
  return canvas->current_line;
}

unsigned long canvas_column(const Canvas *canvas)
{
  return canvas->current_column;
}

void canvas_seek(Canvas *canvas, unsigned long line, unsigned long column)
{
  Array **last_line;
  canvas->current_line   = line;
  canvas->current_column = column;

  while (canvas->current_line >= array_count(canvas->lines)) {
    Array *line = array_new(sizeof(CanvasCell));
    array_push(canvas->lines, &line);
  }

  last_line = array_at(canvas->lines, canvas->current_line);
  while (canvas->current_column >= array_count(*last_line)) {
    CanvasCell cell;
    cell.style = term_default_style();
    cell.size  = 1;
    strcpy(cell.character, " ");
    array_push(*last_line, &cell);
  }
}

void canvas_print(Canvas *canvas, FILE *file)
{
  unsigned long line, column;
  for (line = 0; line < array_count(canvas->lines); ++line) {
    Array **line_array = array_at(canvas->lines, line);
    for (column = 0; column < array_count(*line_array); ++column) {
      CanvasCell *cell = array_at(*line_array, column);
      term_style(file, &cell->style);
      fprintf(file, "%.*s", (int) cell->size, cell->character);
      term_reset(file);
    }
    if (line + 1 < array_count(canvas->lines)) {
      fprintf(file, "\n");
    }
  }
  fflush(file);
}
