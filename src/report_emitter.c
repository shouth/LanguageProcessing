#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "report.h"
#include "source.h"

typedef struct ReportEmitter ReportEmitter;
typedef struct Canvas        ReportCanvas;
typedef struct CanvasCell    ReportCell;

struct ReportEmitter {
  unsigned long number_margin;
  unsigned long tab_width;
};

typedef enum {
  CANVAS_NORMAL    = 1 << 0,
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
  Array         lines;
  unsigned long current_line;
  unsigned long current_column;
};

static void canvas_init(ReportCanvas *canvas)
{
  array_init(&canvas->lines, sizeof(Array));
  {
    Array line;
    array_init(&line, sizeof(ReportCell));
    array_push(&canvas->lines, &line);
  }
  canvas->current_line   = 0;
  canvas->current_column = 0;
}

static void canvas_deinit(ReportCanvas *canvas)
{
  unsigned long i;
  for (i = 0; i < array_count(&canvas->lines); ++i) {
    Array *line = array_at(&canvas->lines, i);
    array_deinit(line);
  }
  array_deinit(&canvas->lines);
}

#define BUFFER_SIZE 1024

static void canvas_next_line(ReportCanvas *canvas)
{
  ++canvas->current_line;
  canvas->current_column = 0;
  if (canvas->current_line >= array_count(&canvas->lines)) {
    Array line;
    array_init(&line, sizeof(ReportCell));
    array_push(&canvas->lines, &line);
  }
}

static void canvas_draw(
  ReportCanvas *canvas,
  unsigned long style, unsigned long foreground, unsigned long background,
  const char *format, ...)
{
  va_list args;
  Array  *line  = array_at(&canvas->lines, canvas->current_line);
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
        unsigned long initial_line_width = array_count(line);
        ReportCell    cell;
        cell.style      = style;
        cell.foreground = foreground;
        cell.background = background;
        cell.size       = size;
        memcpy(cell.character, buffer + index, size);
        if (canvas->current_column < initial_line_width) {
          memcpy(array_at(line, canvas->current_column), &cell, sizeof(ReportCell));
        } else {
          array_push(line, &cell);
        }
        ++canvas->current_column;
        index += size;
      }
    }
  }
  fclose(file);
}

static void canvas_position(ReportCanvas *canvas, unsigned long *line, unsigned long *column)
{
  *line   = canvas->current_line;
  *column = canvas->current_column;
}

static void canvas_seek(ReportCanvas *canvas, unsigned long line, unsigned long column)
{
  canvas->current_line   = line;
  canvas->current_column = column;

  while (canvas->current_line >= array_count(&canvas->lines)) {
    Array line;
    array_init(&line, sizeof(ReportCell));
    array_push(&canvas->lines, &line);
  }

  while (canvas->current_column >= array_count(array_at(&canvas->lines, canvas->current_line))) {
    ReportCell cell;
    cell.style      = CANVAS_NORMAL;
    cell.foreground = 0;
    cell.background = 0;
    cell.size       = 1;
    strcpy(cell.character, " ");
    array_push(array_at(&canvas->lines, canvas->current_line), &cell);
  }
}

static void canvas_print(ReportCanvas *canvas, FILE *stream)
{
  unsigned long line, column;
  for (line = 0; line < array_count(&canvas->lines); ++line) {
    Array *line_array = array_at(&canvas->lines, line);
    for (column = 0; column < array_count(line_array); ++column) {
      ReportCell *cell = array_at(line_array, column);
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
    if (line + 1 < array_count(&canvas->lines)) {
      fprintf(stream, "\n");
    }
  }
  fflush(stream);
}

static unsigned long digits(unsigned long number)
{
  unsigned long result = 1;
  while (number > 9) {
    ++result;
    number /= 10;
  }
  return result;
}

static int compare_annotation(const void *left, const void *right)
{
  const ReportAnnotation *left_annotation  = left;
  const ReportAnnotation *right_annotation = right;

  if (left_annotation->_start_offset != right_annotation->_start_offset) {
    return left_annotation->_start_offset < right_annotation->_start_offset ? -1 : 1;
  } else if (left_annotation->_end_offset != right_annotation->_end_offset) {
    return left_annotation->_end_offset < right_annotation->_end_offset ? -1 : 1;
  } else {
    return 0;
  }
}

static void display_location(const Source *source, unsigned long offset, unsigned long tab_width, SourceLocation *location)
{
  unsigned long i;
  unsigned long column = 0;
  source_location(source, offset, location);
  for (i = 0; i < location->column; ++i) {
    if (source->text[source->line_offsets[location->line] + i] == '\t') {
      column += tab_width - (column % tab_width);
    } else {
      ++column;
    }
  }
  location->column = column;
}

static void report_emitter_init(ReportEmitter *emitter, Report *report, const Source *source)
{
  unsigned long i;
  emitter->tab_width = 4;
  for (i = 0; i < array_count(&report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&report->_annotations, i);
    display_location(source, annotation->_start_offset, emitter->tab_width, &annotation->_start);
    display_location(source, annotation->_end_offset - 1, emitter->tab_width, &annotation->_end);
  }
  qsort(array_data(&report->_annotations), array_count(&report->_annotations), sizeof(ReportAnnotation), &compare_annotation);
  emitter->number_margin = digits(((ReportAnnotation *) array_back(&report->_annotations))->_end.line + 1);
}

static unsigned long display_line_width(const Source *source, unsigned long line, unsigned long tab_width)
{
  unsigned long i;
  unsigned long width = 0;
  for (i = 0; i < source->line_lengths[line]; ++i) {
    if (source->text[source->line_offsets[line] + i] == '\t') {
      width += tab_width - (width % tab_width);
    } else {
      ++width;
    }
  }
  return width;
}

static char *display_line(const Source *source, unsigned long line, unsigned long tab_width)
{
  unsigned long i, j;
  unsigned long width  = display_line_width(source, line, tab_width);
  char         *result = malloc(width + 1);
  unsigned long offset = 0;
  for (i = 0; i < source->line_lengths[line]; ++i) {
    if (source->text[source->line_offsets[line] + i] == '\t') {
      unsigned long adjusted_width = tab_width - (offset % tab_width);
      for (j = 0; j < adjusted_width; ++j) {
        result[offset] = ' ';
        ++offset;
      }
    } else {
      result[offset] = source->text[source->line_offsets[line] + i];
      ++offset;
    }
  }
  result[width] = '\0';
  return result;
}

void report_emit(Report *report, const Source *source)
{
  ReportEmitter emitter;
  ReportCanvas  canvas;
  report_emitter_init(&emitter, report, source);
  canvas_init(&canvas);

  switch (report->_kind) {
  case REPORT_KIND_ERROR:
    canvas_draw(&canvas, CANVAS_BOLD, CANVAS_4BIT | 91, 0, "[ERROR] ");
    break;
  case REPORT_KIND_WARN:
    canvas_draw(&canvas, CANVAS_NORMAL, CANVAS_4BIT | 93, 0, "[WARN] ");
    break;
  case REPORT_KIND_NOTE:
    canvas_draw(&canvas, CANVAS_NORMAL, CANVAS_4BIT | 96, 0, "[NOTE] ");
    break;
  }
  canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "%s", report->_message);
  canvas_next_line(&canvas);

  {
    SourceLocation location;
    source_location(source, report->_offset, &location);
    canvas_draw(&canvas, CANVAS_FAINT, 0, 0, " %*.s ╭─[",
      (int) emitter.number_margin, "");
    canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "%s:%lu:%lu",
      source->file_name,
      location.line + 1, location.column + 1);
    canvas_draw(&canvas, CANVAS_FAINT, 0, 0, "]");
    canvas_next_line(&canvas);
  }
 
  {
    Array         multiline_annotations;
    Array         multiline_annotations_in_range;
    unsigned long i, j;

    array_init(&multiline_annotations, sizeof(ReportAnnotation *));
    array_init(&multiline_annotations_in_range, sizeof(unsigned char));
    for (i = 0; i < array_count(&report->_annotations); ++i) {
      const ReportAnnotation *annotation = array_at(&report->_annotations, i);
      if (annotation->_start.line != annotation->_end.line) {
        unsigned char flag = 0;
        array_push(&multiline_annotations, &annotation);
        array_push(&multiline_annotations_in_range, &flag);
      }
    }

    for (i = 0; i < array_count(&report->_annotations); ++i) {
      const ReportAnnotation *start = array_at(&report->_annotations, i);
      const ReportAnnotation *end   = start + 1;
      while (end - start < (long) array_count(&report->_annotations) && start->_start.line == end->_start.line) {
        ++end;
      }

      canvas_draw(&canvas, CANVAS_FAINT, 0, 0, " %*.s │", emitter.number_margin, "");
      canvas_next_line(&canvas);
      canvas_draw(&canvas, CANVAS_FAINT, 0, 0, " %*.lu │ ", (int) emitter.number_margin, start->_start.line + 1);
      for (j = 0; j < array_count(&multiline_annotations); ++j) {
        unsigned char *flag = array_at(&multiline_annotations_in_range, j);
        if (*flag) {
          canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "│ ");
        } else {
          canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "  ");
        }
      }
      {
        unsigned long line_width = display_line_width(source, start->_start.line, emitter.tab_width);
        char         *line       = display_line(source, start->_start.line, emitter.tab_width);

        const ReportAnnotation *annotation;
        unsigned long           line_offset;
        unsigned long           column_offset;

        canvas_position(&canvas, &line_offset, &column_offset);
        canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "%s", line);

        for (annotation = start; annotation < end; ++annotation) {
          unsigned long annotation_end = annotation->_start.line == annotation->_end.line ? annotation->_end.column + 1 : line_width;
          canvas_seek(&canvas, line_offset, column_offset + annotation->_start.column);
          canvas_draw(&canvas, CANVAS_NORMAL, CANVAS_4BIT | 91, 0, "%.*s",
            (int) (annotation_end - annotation->_start.column), line + annotation->_start.column);
        }
        canvas_next_line(&canvas);

        canvas_draw(&canvas, CANVAS_FAINT, 0, 0, " %*.s │ ", (int) emitter.number_margin, "");
        for (j = 0; j < array_count(&multiline_annotations); ++j) {
          unsigned char *flag = array_at(&multiline_annotations_in_range, j);
          if (*flag) {
            canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "│ ");
          } else {
            canvas_draw(&canvas, CANVAS_NORMAL, 0, 0, "  ");
          }
        }

        canvas_position(&canvas, &line_offset, &column_offset);
        for (annotation = start; annotation < end; ++annotation) {
          if (annotation->_start.line == annotation->_end.line) {
            canvas_seek(&canvas, line_offset, column_offset + annotation->_start.column);
            canvas_draw(&canvas, CANVAS_NORMAL, CANVAS_4BIT | 91, 0, "┬");
            for (j = annotation->_start.column + 1; j <= annotation->_end.column; ++j) {
              canvas_draw(&canvas, CANVAS_NORMAL, CANVAS_4BIT | 91, 0, "─");
            }
          }
        }

        canvas_next_line(&canvas);
        free(line);
      }
    }
  }
  canvas_draw(&canvas, CANVAS_FAINT, 0, 0, " %*.s │", (int) emitter.number_margin, "");
  canvas_next_line(&canvas);

  {
    unsigned long i;
    canvas_draw(&canvas, CANVAS_FAINT, 0, 0, "─");
    for (i = 0; i <= emitter.number_margin; ++i) {
      canvas_draw(&canvas, CANVAS_FAINT, 0, 0, "─");
    }
    canvas_draw(&canvas, CANVAS_FAINT, 0, 0, "╯");
    canvas_next_line(&canvas);
  }

  canvas_print(&canvas, stderr);
  canvas_deinit(&canvas);
  report_deinit(report);
}
