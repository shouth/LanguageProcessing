#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "canvas.h"
#include "terminal.h"

typedef struct CanvasStyle CanvasStyle;

struct CanvasStyle {
  signed char   intensity;
  signed char   italic;
  signed char   underline;
  unsigned long fg;
  unsigned long bg;
};

struct CanvasCell {
  char        character[4];
  int         size;
  CanvasStyle style;
};

struct Canvas {
  Array        *lines;
  unsigned long current_line;
  unsigned long current_column;
  CanvasStyle   style;
};

Canvas *canvas_new(void)
{
  Canvas *canvas = malloc(sizeof(Canvas));
  Array  *line   = array_new(sizeof(CanvasCell));
  canvas->lines  = array_new(sizeof(Array *));
  array_push(canvas->lines, &line);
  canvas->current_line    = 0;
  canvas->current_column  = 0;
  canvas->style.intensity = 0;
  canvas->style.italic    = 0;
  canvas->style.underline = 0;
  canvas->style.fg        = 0;
  canvas->style.bg        = 0;
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

void canvas_style(Canvas *canvas, int attr, ...)
{
  switch (attr) {
  case TERM_RESET:
    canvas->style.intensity = 0;
    canvas->style.italic    = 0;
    canvas->style.underline = 0;
    canvas->style.fg        = 0;
    canvas->style.bg        = 0;
    break;

  case TERM_BOLD:
    canvas->style.intensity = 1;
    break;

  case TERM_FAINT:
    canvas->style.intensity = -1;
    break;

  case TERM_ITALIC:
    canvas->style.italic = 1;
    break;

  case TERM_UNDERLINE:
    canvas->style.underline = 1;
    break;

  case TERM_FG_BLACK:
  case TERM_FG_RED:
  case TERM_FG_GREEN:
  case TERM_FG_YELLOW:
  case TERM_FG_BLUE:
  case TERM_FG_MAGENTA:
  case TERM_FG_CYAN:
  case TERM_FG_WHITE:
  case TERM_FG_BRIGHT_BLACK:
  case TERM_FG_BRIGHT_RED:
  case TERM_FG_BRIGHT_GREEN:
  case TERM_FG_BRIGHT_YELLOW:
  case TERM_FG_BRIGHT_BLUE:
  case TERM_FG_BRIGHT_MAGENTA:
  case TERM_FG_BRIGHT_CYAN:
  case TERM_FG_BRIGHT_WHITE:
    canvas->style.fg = attr;
    break;

  case TERM_FG_SELECT: {
    va_list args;
    va_start(args, attr);
    switch (va_arg(args, int)) {
    case TERM_COLOR_RGB: {
      unsigned long r  = va_arg(args, int);
      unsigned long g  = va_arg(args, int);
      unsigned long b  = va_arg(args, int);
      canvas->style.fg = (TERM_COLOR_RGB << 24) | (r << 16) | (g << 8) | b;
      break;
    }
    case TERM_COLOR_256: {
      unsigned long index = va_arg(args, int);
      canvas->style.fg    = (TERM_COLOR_256 << 24) | index;
      break;
    }
    }
    va_end(args);
    break;
  }

  case TERM_BG_BLACK:
  case TERM_BG_RED:
  case TERM_BG_GREEN:
  case TERM_BG_YELLOW:
  case TERM_BG_BLUE:
  case TERM_BG_MAGENTA:
  case TERM_BG_CYAN:
  case TERM_BG_WHITE:
  case TERM_BG_BRIGHT_BLACK:
  case TERM_BG_BRIGHT_RED:
  case TERM_BG_BRIGHT_GREEN:
  case TERM_BG_BRIGHT_YELLOW:
  case TERM_BG_BRIGHT_BLUE:
  case TERM_BG_BRIGHT_MAGENTA:
  case TERM_BG_BRIGHT_CYAN:
  case TERM_BG_BRIGHT_WHITE:
    canvas->style.bg = attr;
    break;

  case TERM_BG_SELECT: {
    va_list args;
    va_start(args, attr);
    switch (va_arg(args, int)) {
    case TERM_COLOR_RGB: {
      unsigned long r  = va_arg(args, int);
      unsigned long g  = va_arg(args, int);
      unsigned long b  = va_arg(args, int);
      canvas->style.bg = (TERM_COLOR_RGB << 24) | (r << 16) | (g << 8) | b;
      break;
    }
    case TERM_COLOR_256: {
      unsigned long index = va_arg(args, int);
      canvas->style.bg    = (TERM_COLOR_256 << 24) | index;
      break;
    }
    }
    va_end(args);
    break;
  }
  }
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
        cell.style = canvas->style;
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
    cell.style = canvas->style;
    cell.size  = 1;
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
      CanvasCell *cell      = array_at(*line_array, column);
      int         fg_format = cell->style.fg >> 24;
      int         bg_format = cell->style.bg >> 24;
      if (cell->style.intensity > 0) {
        fprintf(stream, "\033[1m");
      } else if (cell->style.intensity < 0) {
        fprintf(stream, "\033[2m");
      }
      if (cell->style.italic) {
        fprintf(stream, "\033[3m");
      }
      if (cell->style.underline) {
        fprintf(stream, "\033[4m");
      }
      switch (fg_format) {
      case 0:
        if (cell->style.fg) {
          fprintf(stream, "\033[%lum", cell->style.fg);
        }
        break;
      case TERM_COLOR_RGB:
        fprintf(stream, "\033[38;2;%lu;%lu;%lum",
          (cell->style.fg >> 16) & 0xFF, (cell->style.fg >> 8) & 0xFF, cell->style.fg & 0xFF);
        break;
      case TERM_COLOR_256:
        fprintf(stream, "\033[38;5;%lum", cell->style.fg & 0xFF);
        break;
      }
      switch (bg_format) {
      case 0:
        if (cell->style.bg) {
          fprintf(stream, "\033[%lum", cell->style.bg);
        }
        break;
      case TERM_COLOR_RGB:
        fprintf(stream, "\033[48;2;%lu;%lu;%lum",
          (cell->style.bg >> 16) & 0xFF, (cell->style.bg >> 8) & 0xFF, cell->style.bg & 0xFF);
        break;
      case TERM_COLOR_256:
        fprintf(stream, "\033[48;5;%lum", cell->style.bg & 0xFFFFFF);
        break;
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
