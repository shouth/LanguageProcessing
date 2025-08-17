/* SPDX-License-Identifier: Apache-2.0 */

#include <stdarg.h>
#include <stdio.h>

#include "term.h"
#include "util.h"

/* TermStyle */

TermStyle term_default_style(void)
{
  TermStyle style;
  style.foreground = TERM_COLOR_NONE;
  style.background = TERM_COLOR_NONE;
  style.intensity  = TERM_INTENSITY_NORMAL;
  style.italic     = TERM_ITALIC_OFF;
  style.underline  = TERM_UNDERLINE_OFF;
  return style;
}

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <stdio.h>
#include <unistd.h>

static int term_has_style_support(FILE *file)
{
  return isatty(fileno(file));
}

#else

static int term_has_style_support(FILE *file)
{
  return 0;
}

#endif

#elif defined(_WIN32) || defined(_WIN64)

#include <io.h>
#include <stdio.h>

static int term_has_style_support(FILE *file)
{
  return _isatty(_fileno(file));
}

#else

static int term_has_style_support(FILE *file)
{
  return 0;
}

#endif

int term_enable_style(TermStyleFlag flag)
{
  static int use_color = TERM_STYLE_AUTO;
  if (flag != TERM_STYLE_TEST) {
    use_color = flag;
  }
  return use_color;
}

int term_use_style(FILE *file)
{
  int use_color = term_enable_style(TERM_STYLE_TEST);
  if (use_color == TERM_STYLE_AUTO) {
    return term_has_style_support(file);
  } else {
    return use_color;
  }
}

void term_style(FILE *file, const TermStyle *style)
{
  if (!term_use_style(file)) {
    return;
  }

  if (!style) {
    fprintf(file, "\033[0m");
    return;
  }

  switch (style->intensity) {
  case TERM_INTENSITY_NORMAL:
    /* Do nothing */
    break;
  case TERM_INTENSITY_STRONG:
    fprintf(file, "\033[1m");
    break;
  case TERM_INTENSITY_FAINT:
    fprintf(file, "\033[2m");
    break;
  default:
    unreachable();
  }

  switch (style->italic) {
  case TERM_ITALIC_OFF:
    /* Do nothing */
    break;
  case TERM_ITALIC_ON:
    fprintf(file, "\033[3m");
    break;
  default:
    unreachable();
  }

  switch (style->underline) {
  case TERM_UNDERLINE_OFF:
    /* Do nothing */
    break;
  case TERM_UNDERLINE_ON:
    fprintf(file, "\033[4m");
    break;
  default:
    unreachable();
  }

  switch (style->foreground) {
  case TERM_COLOR_NONE:
    /* Do nothing */
    break;
  case TERM_COLOR_BLACK:
    fprintf(file, "\033[30m");
    break;
  case TERM_COLOR_RED:
    fprintf(file, "\033[31m");
    break;
  case TERM_COLOR_GREEN:
    fprintf(file, "\033[32m");
    break;
  case TERM_COLOR_YELLOW:
    fprintf(file, "\033[33m");
    break;
  case TERM_COLOR_BLUE:
    fprintf(file, "\033[34m");
    break;
  case TERM_COLOR_MAGENTA:
    fprintf(file, "\033[35m");
    break;
  case TERM_COLOR_CYAN:
    fprintf(file, "\033[36m");
    break;
  case TERM_COLOR_WHITE:
    fprintf(file, "\033[37m");
    break;
  case TERM_COLOR_BLACK | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[90m");
    break;
  case TERM_COLOR_RED | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[91m");
    break;
  case TERM_COLOR_GREEN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[92m");
    break;
  case TERM_COLOR_YELLOW | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[93m");
    break;
  case TERM_COLOR_BLUE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[94m");
    break;
  case TERM_COLOR_MAGENTA | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[95m");
    break;
  case TERM_COLOR_CYAN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[96m");
    break;
  case TERM_COLOR_WHITE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[97m");
    break;
  default: {
    if (style->foreground & TERM_COLOR_256) {
      unsigned long fg = style->foreground & ((1ul << 24) - 1);
      fprintf(file, "\033[38;2;%lu;%lu;%lum", fg >> 16, (fg >> 8) & 0xff, fg & 0xff);
    } else {
      unreachable();
    }
  }
  }

  switch (style->background) {
  case TERM_COLOR_NONE:
    /* Do nothing */
    break;
  case TERM_COLOR_BLACK:
    fprintf(file, "\033[40m");
    break;
  case TERM_COLOR_RED:
    fprintf(file, "\033[41m");
    break;
  case TERM_COLOR_GREEN:
    fprintf(file, "\033[42m");
    break;
  case TERM_COLOR_YELLOW:
    fprintf(file, "\033[43m");
    break;
  case TERM_COLOR_BLUE:
    fprintf(file, "\033[44m");
    break;
  case TERM_COLOR_MAGENTA:
    fprintf(file, "\033[45m");
    break;
  case TERM_COLOR_CYAN:
    fprintf(file, "\033[46m");
    break;
  case TERM_COLOR_WHITE:
    fprintf(file, "\033[47m");
    break;
  case TERM_COLOR_BLACK | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[100m");
    break;
  case TERM_COLOR_RED | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[101m");
    break;
  case TERM_COLOR_GREEN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[102m");
    break;
  case TERM_COLOR_YELLOW | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[103m");
    break;
  case TERM_COLOR_BLUE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[104m");
    break;
  case TERM_COLOR_MAGENTA | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[105m");
    break;
  case TERM_COLOR_CYAN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[106m");
    break;
  case TERM_COLOR_WHITE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[107m");
    break;
  default: {
    if (style->background & TERM_COLOR_256) {
      unsigned long bg = style->background & ((1ul << 24) - 1);
      fprintf(file, "\033[48;2;%lu;%lu;%lum", bg >> 16, (bg >> 8) & 0xff, bg & 0xff);
    } else {
      unreachable();
    }
  }
  }
}

void term_print(FILE *file, const TermStyle *style, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  term_style(file, style);
  vfprintf(file, format, args);
  term_style(file, NULL);
  va_end(args);
}

/* TermBuf */

typedef struct TermBufCell TermBufCell;
typedef struct TermBufLine TermBufLine;

struct TermBufCell {
  char      character[4];
  int       size;
  TermStyle style;
};

struct TermBufLine {
  TermBufCell *cells;
  size_t       cell_count;
  size_t       cell_capacity;
};

struct TermBuf {
  TermBufLine *lines;
  size_t       line_count;
  size_t       line_capacity;
  size_t       current_line;
  size_t       current_column;
};

TermBuf *term_buf_new(void)
{
  TermBuf *buf = malloc(sizeof(TermBuf));
  buf->lines = NULL;
  buf->line_count = 0;
  buf->line_capacity = 0;
  buf->current_line = 0;
  buf->current_column = 0;
  return buf;
}

void term_buf_free(TermBuf *buf)
{
  if (buf) {
    size_t i;
    for (i = 0; i < buf->line_count; ++i) {
      free(buf->lines[i].cells);
    }
    free(buf->lines);
    free(buf);
  }
}

#define BUFFER_SIZE 1024

static TermBufCell *locate(TermBuf *buf, size_t line, size_t column)
{
  size_t i;

  if (line >= buf->line_capacity) {
    TermBufLine *lines = buf->lines;
    size_t capacity = buf->line_capacity;
    for (i = 1; i < sizeof(size_t) * CHAR_BIT; i <<= 1) {
      capacity |= capacity >> i;
    }
    ++capacity;
    buf->lines = xmalloc(sizeof(TermBufLine) * capacity);
    buf->line_capacity = capacity;
    memcpy(buf->lines, lines, sizeof(TermBufLine) * buf->line_count);
    free(lines);
  }
  if (line >= buf->line_count) {
    for (i = buf->line_count; i <= line; ++i) {
      TermBufLine line = { NULL };
      buf->lines[i] = line;
    }
    buf->line_count = line;
  }

  if (column >= buf->lines[line].cell_capacity) {
    TermBufCell *cells = buf->lines[line].cells;
    size_t capacity = buf->lines[line].cell_capacity;
    for (i = 1; i < sizeof(size_t) * CHAR_BIT; i <<= 1) {
      capacity |= capacity >> i;
    }
    ++capacity;
    buf->lines[line].cells = xmalloc(sizeof(TermBufCell) * capacity);
    buf->lines[line].cell_capacity = capacity;
    memcpy(buf->lines[line].cells, cells, sizeof(TermBufCell) * buf->lines[line].cell_count);
    free(cells);
  }
  if (column >= buf->lines[line].cell_count) {
    for (i = buf->lines[line].cell_count; i <= column; ++i) {
      TermBufCell cell = { " ", 1 };
      buf->lines[line].cells[i] = cell;
    }
    buf->lines[line].cell_count = column;
  }

  return &buf->lines[line].cells[column];
}

void term_buf_next_line(TermBuf *buf)
{
  term_buf_seek(buf, buf->current_line + 1, 0);
}

void term_buf_write(TermBuf *buf, TermStyle const *style, char const *format, ...)
{
  va_list args;

  FILE *file  = tmpfile();
  long  index = 0;
  char  buffer[BUFFER_SIZE + 1];

  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  rewind(file);
  while (fgets(buffer + index, BUFFER_SIZE - index, file)) {
    while (buffer[index]) {
      long size = utf8_len(buffer + index, BUFFER_SIZE - index);
      if (size <= 0) {
        int remain = strlen(buffer + index);
        memmove(buffer, buffer + index, remain);
        index = remain;
        break;
      }

      {
        TermBufCell *cell = locate(buf, buf->current_line, buf->current_column);
        cell->style = style ? *style : term_default_style();
        cell->size  = size;
        memcpy(cell->character, buffer + index, size);
        ++buf->current_column;
        index += size;
      }
    }
  }
  fclose(file);
}

unsigned long term_buf_line(TermBuf const *buf)
{
  return buf->current_line;
}

unsigned long term_buf_column(TermBuf const *buf)
{
  return buf->current_column;
}

void term_buf_seek(TermBuf *buf, size_t line, size_t column)
{
  locate(buf, line, column);
  buf->current_line   = line;
  buf->current_column = column;
}

void term_buf_print(TermBuf *buf, FILE *file)
{
  unsigned long line, column;
  for (line = 0; line < buf->line_count; ++line) {
    for (column = 0; column < buf->lines[line].cell_count; ++column) {
      TermBufCell *cell = &buf->lines[line].cells[column];
      term_style(file, &cell->style);
      fprintf(file, "%.*s", (int) cell->size, cell->character);
      term_style(file, NULL);
    }
    if (line + 1 < buf->line_count) {
      fprintf(file, "\n");
    }
  }
  fflush(file);
}
