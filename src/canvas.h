#ifndef CANVAS_H
#define CANVAS_H

#include <stdio.h>

typedef struct Canvas     Canvas;
typedef struct CanvasCell CanvasCell;

Canvas       *canvas_new(void);
void          canvas_free(Canvas *canvas);
void          canvas_next_line(Canvas *canvas);
void          canvas_style(Canvas *canvas, int attr, ...);
void          canvas_write(Canvas *canvas, const char *format, ...);
unsigned long canvas_line(const Canvas *canvas);
unsigned long canvas_column(const Canvas *canvas);
void          canvas_seek(Canvas *canvas, unsigned long line, unsigned long column);
void          canvas_print(Canvas *canvas, FILE *stream);

#endif
