#ifndef CANVAS_H
#define CANVAS_H

#include <stdio.h>

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

Canvas       *canvas_new(void);
void          canvas_free(Canvas *canvas);
void          canvas_next_line(Canvas *canvas);
void          canvas_style(Canvas *canvas, unsigned long style);
void          canvas_style_foreground(Canvas *canvas, unsigned long color);
void          canvas_style_background(Canvas *canvas, unsigned long color);
void          canvas_write(Canvas *canvas, const char *format, ...);
unsigned long canvas_line(const Canvas *canvas);
unsigned long canvas_column(const Canvas *canvas);
void          canvas_seek(Canvas *canvas, unsigned long line, unsigned long column);
void          canvas_print(Canvas *canvas, FILE *stream);

#endif
