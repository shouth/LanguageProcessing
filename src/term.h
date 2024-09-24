/* SPDX-License-Identifier: Apache-2.0 */

#ifndef TERM_H
#define TERM_H

#include <stdio.h>

#include "util.h"

/* TermStyle */

#define TERM_STYLE_TEST -1
#define TERM_STYLE_OFF  0
#define TERM_STYLE_ON   1
#define TERM_STYLE_AUTO 2

#define TERM_COLOR_NONE    0ul
#define TERM_COLOR_BLACK   1ul
#define TERM_COLOR_RED     2ul
#define TERM_COLOR_GREEN   3ul
#define TERM_COLOR_YELLOW  4ul
#define TERM_COLOR_BLUE    5ul
#define TERM_COLOR_MAGENTA 6ul
#define TERM_COLOR_CYAN    7ul
#define TERM_COLOR_WHITE   8ul
#define TERM_COLOR_BRIGHT  1ul << 24
#define TERM_COLOR_256     1ul << 25

#define TERM_INTENSITY_NORMAL 0
#define TERM_INTENSITY_STRONG 1
#define TERM_INTENSITY_FAINT  2

#define TERM_ITALIC_OFF 0
#define TERM_ITALIC_ON  1

#define TERM_UNDERLINE_OFF 0
#define TERM_UNDERLINE_ON  1

typedef signed char      TermStyleFlag;
typedef unsigned long    TermColorStyle;
typedef unsigned char    TermIntensityStyle;
typedef unsigned char    TermItalicStyle;
typedef unsigned char    TermUnderlineStyle;
typedef struct TermStyle TermStyle;

struct TermStyle {
  TermColorStyle     foreground;
  TermColorStyle     background;
  TermIntensityStyle intensity;
  TermItalicStyle    italic;
  TermUnderlineStyle underline;
};

TermStyle term_default_style(void);
int       term_enable_style(TermStyleFlag flag);
int       term_use_style(FILE *file);
void      term_style(FILE *file, const TermStyle *style);
void      term_print(FILE *file, const TermStyle *style, const char *format, ...) format(printf, 3, 4);

/* TermBuf */

typedef struct TermBuf TermBuf;

TermBuf      *term_buf_new(void);
void          term_buf_free(TermBuf *canvas);
void          term_buf_next_line(TermBuf *canvas);
void          term_buf_write(TermBuf *canvas, const TermStyle *style, const char *format, ...) format(printf, 3, 4);
unsigned long term_buf_line(const TermBuf *canvas);
unsigned long term_buf_column(const TermBuf *canvas);
void          term_buf_seek(TermBuf *canvas, unsigned long line, unsigned long column);
void          term_buf_print(TermBuf *canvas, FILE *stream);

#endif /* TERM_H */
