#ifndef CANVAS_H
#define CANVAS_H

#include <stdio.h>

#include "array.h"

typedef struct Canvas     Canvas;
typedef struct CanvasCell CanvasCell;

typedef enum {
  CANVAS_RESET     = 1 << 0,
  CANVAS_BOLD      = 1 << 1,
  CANVAS_FAINT     = 1 << 2,
  CANVAS_ITALIC    = 1 << 3,
  CANVAS_UNDERLINE = 1 << 4
} CanvasStyleAttribute;

typedef enum {
  CANVAS_4BIT  = 1 << 24,
  CANVAS_8BIT  = 1 << 25,
  CANVAS_24BIT = 1 << 26
} CanvasColorAttribute;

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

void canvas_init(Canvas *canvas);
void canvas_deinit(Canvas *canvas);
void canvas_next_line(Canvas *canvas);
void canvas_style(Canvas *canvas, unsigned long style);
void canvas_style_foreground(Canvas *canvas, unsigned long color);
void canvas_style_background(Canvas *canvas, unsigned long color);
void canvas_write(Canvas *canvas, const char *format, ...);
void canvas_position(Canvas *canvas, unsigned long *line, unsigned long *column);
void canvas_seek(Canvas *canvas, unsigned long line, unsigned long column);
void canvas_print(Canvas *canvas, FILE *stream);

#endif
