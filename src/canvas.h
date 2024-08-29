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

#ifndef CANVAS_H
#define CANVAS_H

#include <stdio.h>

#include "term.h"

typedef struct Canvas     Canvas;
typedef struct CanvasCell CanvasCell;

Canvas       *canvas_new(void);
void          canvas_free(Canvas *canvas);
void          canvas_next_line(Canvas *canvas);
void          canvas_write(Canvas *canvas, const TermStyle *style, const char *format, ...);
unsigned long canvas_line(const Canvas *canvas);
unsigned long canvas_column(const Canvas *canvas);
void          canvas_seek(Canvas *canvas, unsigned long line, unsigned long column);
void          canvas_print(Canvas *canvas, FILE *stream);

#endif
