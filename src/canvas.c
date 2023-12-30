#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "canvas.h"

struct CanvasCell {
  char          character[4];
  int           size;
  unsigned int  style;
  unsigned long foreground;
  unsigned long background;
};

struct Canvas {
  Array        *lines;
  unsigned long current_line;
  unsigned long current_column;
  unsigned long style;
  unsigned long foreground;
  unsigned long background;
};

Canvas *canvas_new(void)
{
  Canvas *canvas = malloc(sizeof(Canvas));
  Array  *line   = array_new(sizeof(CanvasCell));
  canvas->lines  = array_new(sizeof(Array *));
  array_push(canvas->lines, &line);
  canvas->current_line   = 0;
  canvas->current_column = 0;
  canvas->style          = CANVAS_RESET;
  canvas->foreground     = 0;
  canvas->background     = 0;
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

void canvas_style(Canvas *canvas, unsigned long style)
{
  if (style & CANVAS_RESET) {
    canvas->style      = 0;
    canvas->foreground = 0;
    canvas->background = 0;
  } else {
    canvas->style = style;
  }
}

void canvas_style_foreground(Canvas *canvas, unsigned long color)
{
  canvas->foreground = color;
}

void canvas_style_background(Canvas *canvas, unsigned long color)
{
  canvas->background = color;
}

void canvas_write(Canvas *canvas, const char *format, ...)
{
  va_list args;
  Array **line  = array_at(canvas->lines, canvas->current_line);
  FILE   *file  = tmpfile();
  long    index = 0;
  char    buffer[BUFFER_SIZE + 1];

  setlocale(LC_ALL, "C.UTF-8");
  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  rewind(file);
  mblen(NULL, 0);
  while (fgets(buffer + index, BUFFER_SIZE - index, file)) {
    while (buffer[index]) {
      int size = mblen(buffer + index, BUFFER_SIZE - index);
      if (size < 0) {
        int remain = strlen(buffer + index);
        memmove(buffer, buffer + index, remain);
        index = remain;
        break;
      }

      {
        unsigned long initial_line_width = array_count(*line);
        CanvasCell    cell;
        cell.style      = canvas->style;
        cell.foreground = canvas->foreground;
        cell.background = canvas->background;
        cell.size       = size;
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
    cell.style      = 0;
    cell.foreground = 0;
    cell.background = 0;
    cell.size       = 1;
    strcpy(cell.character, " ");
    array_push(*last_line, &cell);
  }
}

void canvas_print(Canvas *canvas, FILE *stream)
{
  unsigned long line, column;
  for (line = 0; line < array_count(canvas->lines); ++line) {
    Array **line_array = array_at(canvas->lines, line);
    for (column = 0; column < array_count(*line_array); ++column) {
      CanvasCell *cell = array_at(*line_array, column);
      if (cell->style & CANVAS_BOLD) {
        fprintf(stream, "\033[1m");
      }
      if (cell->style & CANVAS_FAINT) {
        fprintf(stream, "\033[2m");
      }
      if (cell->style & CANVAS_ITALIC) {
        fprintf(stream, "\033[3m");
      }
      if (cell->style & CANVAS_UNDERLINE) {
        fprintf(stream, "\033[4m");
      }
      if (cell->foreground & CANVAS_4BIT) {
        fprintf(stream, "\033[%lum", cell->foreground & 0xFFFFFF);
      } else if (cell->foreground & CANVAS_8BIT) {
        fprintf(stream, "\033[38;5;%lum", cell->foreground & 0xFFFFFF);
      } else if (cell->foreground & CANVAS_24BIT) {
        fprintf(stream, "\033[38;2;%lu;%lu;%lum",
          (cell->foreground >> 16) & 0xFF,
          (cell->foreground >> 8) & 0xFF,
          cell->foreground & 0xFF);
      }
      if (cell->background & CANVAS_4BIT) {
        fprintf(stream, "\033[%lum", cell->background & 0xFFFFFF);
      } else if (cell->background & CANVAS_8BIT) {
        fprintf(stream, "\033[48;5;%lum", cell->background & 0xFFFFFF);
      } else if (cell->background & CANVAS_24BIT) {
        fprintf(stream, "\033[48;2;%lu;%lu;%lum",
          (cell->background >> 16) & 0xFF,
          (cell->background >> 8) & 0xFF,
          cell->background & 0xFF);
      }
      fprintf(stream, "%.*s", (int) cell->size, cell->character);
      fprintf(stream, "\033[0m");
    }
    if (line + 1 < array_count(canvas->lines)) {
      fprintf(stream, "\n");
    }
  }
  fflush(stream);
}
