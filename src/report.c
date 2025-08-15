/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "report.h"
#include "term.h"
#include "util.h"

/* Report */

struct ReportAnnotation {
  size_t start_offset;
  size_t end_offset;
  size_t start_line;
  size_t start_column;
  size_t end_line;
  size_t end_column;
  char  *message;
};

struct Report {
  ReportKind    kind;
  unsigned long offset;
  char         *message;
  Vec(ReportAnnotation) annotations;
  Vec(char *) notes;
};

static char *vformat(const char *format, va_list args)
{
  FILE         *file   = tmpfile();
  unsigned long length = vfprintf(file, format, args);
  char         *text   = xmalloc(sizeof(char) * (length + 1));
  rewind(file);
  fread(text, sizeof(char), length, file);
  text[length] = '\0';
  fclose(file);
  return text;
}

Report *report_new(ReportKind kind, unsigned long offset, const char *format, ...)
{
  va_list args;
  Report *report;
  va_start(args, format);
  report = report_new_with_args(kind, offset, format, args);
  va_end(args);
  return report;
}

void report_free(Report *report)
{
  if (report) {
    unsigned long i;
    free(report->message);

    for (i = 0; i < report->notes.count; ++i) {
      free(report->notes.ptr[i]);
    }
    vec_free(&report->notes);

    for (i = 0; i < report->annotations.count; ++i) {
      free(report->annotations.ptr[i].message);
    }
    vec_free(&report->annotations);

    free(report);
  }
}

Report *report_new_with_args(ReportKind kind, unsigned long offset, const char *format, va_list args)
{
  Report *report  = xmalloc(sizeof(Report));
  report->kind    = kind;
  report->offset  = offset;
  report->message = vformat(format, args);
  vec_alloc(&report->notes, 0);
  vec_alloc(&report->annotations, 0);
  return report;
}

void report_annotation(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_annotation_with_args(report, start, end, format, args);
  va_end(args);
}

void report_annotation_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  ReportAnnotation label;
  label.start_offset = start;
  label.end_offset   = end;
  label.message      = format ? vformat(format, args) : NULL;
  vec_push(&report->annotations, &label, 1);
}

void report_note(Report *report, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_note_with_args(report, format, args);
  va_end(args);
}

void report_note_with_args(Report *report, const char *format, va_list args)
{
  char *note = vformat(format, args);
  vec_push(&report->notes, &note, 1);
}

/* Report emitter */

typedef struct LineSegment LineSegment;
typedef struct Indicator   Indicator;
typedef struct Connector   Connector;
typedef struct Writer      Writer;

typedef enum {
  INDICATOR_INLINE,
  INDICATOR_END,
  INDICATOR_BEGIN
} IndicatorKind;

typedef enum {
  CONNECTOR_END,
  CONNECTOR_BEGIN
} ConnectorKind;

struct LineSegment {
  const ReportAnnotation *annotation;
  unsigned long           start;
  unsigned long           end;
};

struct Indicator {
  const ReportAnnotation *annotation;
  IndicatorKind           kind;
  unsigned long           column;
  unsigned long           length;
};

struct Connector {
  const ReportAnnotation *annotation;
  ConnectorKind           kind;
  int                     multiline;
  unsigned long           column;
  unsigned long           depth;
};

struct Writer {
  Report const *report;
  char const   *filename;
  char const   *source;
  size_t        source_length;
  size_t       *offsets;
  size_t        line_count;
  int           number_margin;
  int           tab_width;
};

static int compare_line_segments(const void *left, const void *right)
{
  const LineSegment *left_segment  = left;
  const LineSegment *right_segment = right;

  if (left_segment->start != right_segment->start) {
    return left_segment->start < right_segment->start ? -1 : 1;
  } else if (left_segment->end != right_segment->end) {
    return left_segment->end > right_segment->end ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_indicators(const void *left, const void *right)
{
  const Indicator *left_indicator  = left;
  const Indicator *right_indicator = right;

  if (left_indicator->kind != right_indicator->kind) {
    return left_indicator->kind < right_indicator->kind ? -1 : 1;
  } else if (left_indicator->column != right_indicator->column) {
    return left_indicator->column < right_indicator->column ? -1 : 1;
  } else if (left_indicator->length != right_indicator->length) {
    return left_indicator->length > right_indicator->length ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_connectors(const void *left, const void *right)
{
  const Connector *left_connector  = left;
  const Connector *right_connector = right;

  if (left_connector->column != right_connector->column) {
    return left_connector->column < right_connector->column ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_annotations(const void *left, const void *right)
{
  const ReportAnnotation *left_annotation  = left;
  const ReportAnnotation *right_annotation = right;

  if (left_annotation->start_offset != right_annotation->start_offset) {
    return left_annotation->start_offset < right_annotation->start_offset ? -1 : 1;
  } else if (left_annotation->end_offset != right_annotation->end_offset) {
    return left_annotation->end_offset < right_annotation->end_offset ? -1 : 1;
  } else {
    return 0;
  }
}

static void write_head_line(Writer *writer, TermBuf *canvas)
{
  TermStyle style;

  switch (writer->report->kind) {
  case REPORT_KIND_ERROR:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[ERROR] ");
    break;

  case REPORT_KIND_WARN:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_YELLOW;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[WARN] ");
    break;

  case REPORT_KIND_NOTE:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_CYAN;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[NOTE] ");
    break;
  }

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_WHITE;
  term_buf_write(canvas, &style, "%s", writer->report->message);

  term_buf_next_line(canvas);
}

static void write_location_line(Writer *writer, TermBuf *canvas)
{
  TermStyle style;

  size_t line = fenwick_upper_bound(writer->offsets, writer->line_count, writer->report->offset);
  size_t column = writer->report->offset - fenwick_query(writer->offsets, writer->line_count, line);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.s ╭─[", writer->number_margin, "");

  style            = term_default_style();
  style.foreground = TERM_COLOR_WHITE | TERM_COLOR_BRIGHT;
  term_buf_write(canvas, &style, "%s:%lu:%lu", writer->filename, line + 1, column + 1);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, "]");

  term_buf_next_line(canvas);
}

static void write_annotation_left(
  Writer                 *writer,
  TermBuf                *canvas,
  unsigned long           line_number,
  unsigned long           line_column,
  const ReportAnnotation *connect,
  int                     dotted)
{
  unsigned long i;
  TermStyle     style;

  const ReportAnnotation *strike = NULL;

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
  for (i = 0; i < writer->report->annotations.count; ++i) {
    ReportAnnotation *annotation = &writer->report->annotations.ptr[i];
    if (annotation->start_line != annotation->end_line) {
      if (strike) {
        term_buf_write(canvas, &style, "──");
      } else if (line_number < annotation->start_line || line_number > annotation->end_line) {
        term_buf_write(canvas, &style, "  ");
      } else if (line_number == annotation->start_line) {
        if (line_column == -1ul) {
          term_buf_write(canvas, &style, "  ");
        } else if (line_column > annotation->start_column) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->start_column) {
          term_buf_write(canvas, &style, "  ");
        } else if (annotation == connect) {
          term_buf_write(canvas, &style, "╭─");
          strike = annotation;
        } else {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        }
      } else if (line_number == annotation->end_line) {
        if (line_column == -1ul) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->end_column) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column > annotation->end_column) {
          term_buf_write(canvas, &style, "  ");
        } else if (annotation == connect) {
          term_buf_write(canvas, &style, "╰─");
          strike = annotation;
        } else {
          term_buf_write(canvas, &style, "  ");
        }
      } else {
        term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
      }
    }
  }
}

static void write_source_line(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  unsigned long i, j;
  TermStyle     style;

  unsigned long line_width;
  char         *line;

  unsigned long line_offset;
  unsigned long column_offset;

  Vec(LineSegment) segments;
  Vec(LineSegment) nongraphics;

  size_t offset = fenwick_query(writer->offsets, writer->line_count, line_number);
  size_t length = fenwick_query(writer->offsets, writer->line_count, line_number + 1) - offset;

  vec_alloc(&segments, 0);
  vec_alloc(&nongraphics, 0);

  line_width = 0;
  for (i = 0; i < length; ++i) {
    char c = writer->source[offset + i];
    if (c == '\t') {
      line_width += writer->tab_width - (line_width % writer->tab_width);
    } else if (!is_graphic(c)) {
      line_width += strlen("\\xXX");
    } else {
      ++line_width;
    }
  }

  line        = malloc(line_width + 1);
  line_offset = 0;
  for (i = 0; i < length; ++i) {
    char c = writer->source[offset + i];
    if (c == '\t') {
      unsigned long adjusted_width = writer->tab_width - (line_offset % writer->tab_width);
      line_offset += sprintf(line + line_offset, "%*.s", (int) adjusted_width, "");
    } else if (!is_graphic(c)) {
      LineSegment segment;
      segment.annotation = NULL;

      segment.start = line_offset;
      line_offset += sprintf(line + line_offset, "\\x%X", (unsigned char) (c & 0xFF));
      segment.end = line_offset - 1;
      vec_push(&nongraphics, &segment, 1);
    } else {
      line[line_offset++] = c ? c : ' ';
    }
  }
  line[line_width] = '\0';

  for (i = 0; i < writer->report->annotations.count; ++i) {
    ReportAnnotation *annotation = &writer->report->annotations.ptr[i];
    LineSegment       segment;
    segment.annotation = annotation;
    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      segment.start = annotation->start_column;
      segment.end   = annotation->end_column;
    } else if (annotation->start_line == line_number) {
      segment.start = annotation->start_column;
      segment.end   = line_width;
    } else if (annotation->end_line == line_number) {
      segment.start = 0;
      segment.end   = annotation->end_column;
    } else {
      continue;
    }
    vec_push(&segments, &segment, 1);
  }
  qsort(segments.ptr, segments.count, sizeof(LineSegment), &compare_line_segments);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.lu │ ", writer->number_margin, line_number + 1);

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = term_buf_line(canvas);
  column_offset = term_buf_column(canvas);

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_WHITE;
  term_buf_write(canvas, &style, "%s", line);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  for (j = 0; j < nongraphics.count; ++j) {
    LineSegment *nongraphic = &nongraphics.ptr[j];
    term_buf_seek(canvas, line_offset, column_offset + nongraphic->start);
    term_buf_write(canvas, &style, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
  }

  for (i = 0; i < segments.count; ++i) {
    LineSegment *segment = &segments.ptr[i];
    term_buf_seek(canvas, line_offset, column_offset + segment->start);

    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    term_buf_write(canvas, &style, "%.*s", (int) (segment->end - segment->start + 1), line + segment->start);

    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    style.intensity  = TERM_INTENSITY_FAINT;
    for (j = 0; j < nongraphics.count; ++j) {
      LineSegment *nongraphic = &nongraphics.ptr[j];
      if (nongraphic->start >= segment->start && nongraphic->end <= segment->end) {
        term_buf_seek(canvas, line_offset, column_offset + nongraphic->start);
        term_buf_write(canvas, &style, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
      }
    }
  }
  term_buf_next_line(canvas);

  free(line);
  vec_free(&segments);
  vec_free(&nongraphics);
}

static void write_indicator_line(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  unsigned long i, j;
  TermStyle     style;

  unsigned long line_offset;
  unsigned long column_offset;

  Vec(Indicator) indicators;

  vec_alloc(&indicators, 0);

  for (i = 0; i < writer->report->annotations.count; ++i) {
    ReportAnnotation *annotation = &writer->report->annotations.ptr[i];
    Indicator         indicator;
    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_INLINE;
      indicator.column     = annotation->start_column;
      indicator.length     = annotation->end_column - annotation->start_column + 1;
    } else if (annotation->start_line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_BEGIN;
      indicator.column     = annotation->start_column;
      indicator.length     = 1;
    } else if (annotation->end_line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_END;
      indicator.column     = annotation->end_column;
      indicator.length     = 1;
    } else {
      continue;
    }
    vec_push(&indicators, &indicator, 1);
  }
  qsort(indicators.ptr, indicators.count, sizeof(Indicator), &compare_indicators);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = term_buf_line(canvas);
  column_offset = term_buf_column(canvas);

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
  for (i = 0; i < indicators.count; ++i) {
    Indicator *indicator = &indicators.ptr[i];
    term_buf_seek(canvas, line_offset, column_offset + indicator->column);
    switch (indicator->kind) {
    case INDICATOR_INLINE:
      term_buf_write(canvas, &style, indicator->annotation->message ? "┬" : "─");
      for (j = 1; j < indicator->length; ++j) {
        term_buf_write(canvas, &style, "─");
      }
      break;

    case INDICATOR_END:
    case INDICATOR_BEGIN:
      term_buf_write(canvas, &style, "▲");
      break;
    }
  }
  term_buf_next_line(canvas);

  vec_free(&indicators);
}

static void write_annotation_lines(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  unsigned long i, j;
  TermStyle     style;

  unsigned long label_offset = 0;
  unsigned long line_offset;
  unsigned long column_offset;
  unsigned long end_line_offset;
  unsigned long depth;

  Vec(Connector) connectors;

  vec_alloc(&connectors, 0);

  for (i = 0; i < writer->report->annotations.count; ++i) {
    ReportAnnotation *annotation = &writer->report->annotations.ptr[i];
    Connector         connector;
    connector.annotation = annotation;

    if (annotation->start_line == line_number && label_offset < annotation->start_column) {
      label_offset = annotation->start_column;
    }
    if (annotation->end_line == line_number && label_offset < annotation->end_column) {
      label_offset = annotation->end_column;
    }

    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 0;
      connector.column    = annotation->start_column;
      connector.depth     = -1ul;
    } else if (annotation->start_line == line_number) {
      connector.kind      = CONNECTOR_BEGIN;
      connector.multiline = 1;
      connector.column    = annotation->start_column;
      connector.depth     = -1ul;
    } else if (annotation->end_line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 1;
      connector.column    = annotation->end_column;
      connector.depth     = -1ul;
    } else {
      continue;
    }

    vec_push(&connectors, &connector, 1);
  }
  qsort(connectors.ptr, connectors.count, sizeof(Connector), &compare_connectors);

  depth = 0;

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  for (i = 0; i < connectors.count; ++i) {
    Connector *connector = &connectors.ptr[i];
    if (connector->multiline || connector->annotation->message) {
      connector->depth = depth;
      if (depth > 0) {
        term_buf_next_line(canvas);
      }

      term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");
      write_annotation_left(writer, canvas, line_number, connector->column, connector->annotation, 0);
      if (depth == 0) {
        line_offset   = term_buf_line(canvas);
        column_offset = term_buf_column(canvas);
      }
      term_buf_next_line(canvas);
      term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");
      write_annotation_left(writer, canvas, line_number, connector->column, NULL, 0);
      depth += 2;
    }
  }
  end_line_offset = term_buf_line(canvas);

  for (i = connectors.count; i > 0; --i) {
    Connector *connector = &connectors.ptr[i - 1];

    style            = term_default_style();
    style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
    if (connector->depth != -1ul) {
      for (j = 0; j < connector->depth; ++j) {
        term_buf_seek(canvas, line_offset + j, column_offset + connector->column);
        term_buf_write(canvas, &style, "│");
      }
    }

    switch (connector->kind) {
    case CONNECTOR_END:
      style            = term_default_style();
      style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
      if (connector->multiline) {
        term_buf_seek(canvas, line_offset + j, column_offset);
        for (j = 0; j < connector->column; ++j) {
          term_buf_write(canvas, &style, "─");
        }
        term_buf_write(canvas, &style, connector->annotation->message ? "┴" : "╯");
      } else if (connector->annotation->message) {
        term_buf_seek(canvas, line_offset + j, column_offset + connector->column);
        term_buf_write(canvas, &style, "╰");
      }

      if (connector->annotation->message) {
        for (j = connector->column + 1; j < label_offset + 3; ++j) {
          term_buf_write(canvas, &style, "─");
        }

        style            = term_default_style();
        style.foreground = TERM_COLOR_WHITE | TERM_COLOR_BRIGHT;
        term_buf_write(canvas, &style, " %s", connector->annotation->message);
      }
      break;

    case CONNECTOR_BEGIN:
      style            = term_default_style();
      style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
      term_buf_seek(canvas, line_offset + j, column_offset);
      for (j = 0; j < connector->column; ++j) {
        term_buf_write(canvas, &style, "─");
      }
      term_buf_write(canvas, &style, "╯");
      break;
    }
  }
  term_buf_seek(canvas, end_line_offset, 0);

  vec_free(&connectors);
}

static void write_interest_lines(Writer *writer, TermBuf *canvas)
{
  unsigned long i, j;
  TermStyle     style;

  unsigned long start_line    = -1ul;
  unsigned long end_line      = 0;
  unsigned long previous_line = -1ul;

  for (i = 0; i < writer->report->annotations.count; ++i) {
    ReportAnnotation *annotation = &writer->report->annotations.ptr[i];
    if (start_line > annotation->start_line) {
      start_line = annotation->start_line;
    }
    if (end_line < annotation->end_line) {
      end_line = annotation->end_line;
    }
  }

  for (i = start_line; i <= end_line; ++i) {
    for (j = 0; j < writer->report->annotations.count; ++j) {
      ReportAnnotation *annotation = &writer->report->annotations.ptr[j];
      if (i == annotation->start_line || i == annotation->end_line) {
        int dotted = previous_line != -1ul && previous_line + 1 != i;

        style           = term_default_style();
        style.intensity = TERM_INTENSITY_FAINT;
        term_buf_write(canvas, &style, " %*.s %s", writer->number_margin, "", dotted ? "╎ " : "│ ");
        write_annotation_left(writer, canvas, i, -1ul, NULL, dotted);
        term_buf_next_line(canvas);

        write_source_line(writer, canvas, i);
        write_indicator_line(writer, canvas, i);
        write_annotation_lines(writer, canvas, i);
        previous_line = i;
        break;
      }
    }
  }
}

static void write_tail_line(Writer *writer, TermBuf *canvas)
{
  int       i;
  TermStyle style;

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, "─");
  for (i = 0; i <= writer->number_margin; ++i) {
    term_buf_write(canvas, &style, "─");
  }
  term_buf_write(canvas, &style, "╯");
  term_buf_next_line(canvas);
}

static int digits(unsigned long number)
{
  int result = 1;
  while (number > 9) {
    ++result;
    number /= 10;
  }
  return result;
}

static size_t display_locate(Writer *writer, unsigned long offset, int start, size_t *out_column)
{
  size_t i;
  size_t column;
  size_t line;
  size_t line_column;
  size_t line_offset;

  if (!start) {
    --offset;
  }
  line = fenwick_upper_bound(writer->offsets, writer->line_count, offset);
  line_offset = fenwick_query(writer->offsets, writer->line_count, line);
  if (!start) {
    ++line_column;
  }

  column = line_column;
  for (i = 0; i < line_column; ++i) {
    char c = writer->source[line_offset + i];
    if (c == '\t') {
      column += writer->tab_width - (column % writer->tab_width);
    } else if (!is_graphic(c)) {
      column += strlen("\\xXX");
    } else {
      ++column;
    }
  }

  if (!start) {
    --column;
  }

  if (out_column) {
    *out_column = column;
  }
  return line;
}

static size_t *build_fenwick(char const *text, size_t length, size_t *line_count)
{
  size_t  i;
  size_t  line = 0;
  size_t  capacity = 16;
  size_t *offsets = xmalloc(sizeof(size_t) * capacity);

  do {
    size_t line_length = strcspn(text, "\r\n");
    if (!strncmp(text, "\r\n", 2) || !strncmp(text, "\n\r", 2)) {
      ++line_length;
    }
    ++line_length;

    if (line + 1 >= capacity) {
      size_t *old_offsets = offsets;
      capacity *= 2;
      offsets = xmalloc(sizeof(size_t) * capacity);
      memcpy(offsets, old_offsets, sizeof(size_t) * line);
      free(old_offsets);
    }
    offsets[line++] = line_length;

    text += line_length;
  } while (*text);

  fenwick_construct(offsets, line + 1);
  return offsets;
}

void report_emit(Report *report, char const *filename, char const *source)
{
  Writer        writer;
  TermBuf      *canvas = term_buf_new();
  unsigned long i;

  qsort(report->annotations.ptr, report->annotations.count, sizeof(ReportAnnotation), &compare_annotations);

  writer.report        = report;
  writer.source        = source;
  writer.filename      = filename;
  writer.offsets       = build_fenwick(source, strlen(source), &writer.line_count);
  writer.tab_width     = 4;
  writer.number_margin = 0;
  for (i = 0; i < report->annotations.count; ++i) {
    int               margin;
    ReportAnnotation *annotation = &report->annotations.ptr[i];
    annotation->start_line = display_locate(&writer, annotation->start_offset, 1, &annotation->start_column);
    annotation->end_line   = display_locate(&writer, annotation->end_offset, 0, &annotation->end_column);

    margin = digits(annotation->start_line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
    margin = digits(annotation->end_line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
  }

  write_head_line(&writer, canvas);
  write_location_line(&writer, canvas);
  write_interest_lines(&writer, canvas);
  write_tail_line(&writer, canvas);

  term_buf_print(canvas, stderr);
  term_buf_free(canvas);
  report_free(report);
}
