#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "canvas.h"
#include "report.h"
#include "terminal.h"
#include "utility.h"

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
  const Report *report;
  const Source *source;
  int           number_margin;
  int           tab_width;
};

struct ReportAnnotation {
  unsigned long  start_offset;
  unsigned long  end_offset;
  SourceLocation start;
  SourceLocation end;
  char          *message;
};

struct Report {
  ReportKind    kind;
  unsigned long offset;
  char         *message;
  Array        *annotations;
  Array        *notes;
};

char *vformat(const char *format, va_list args)
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

    for (i = 0; i < array_count(report->notes); ++i) {
      char **note = array_at(report->notes, i);
      free(*note);
    }
    array_free(report->notes);

    for (i = 0; i < array_count(report->annotations); ++i) {
      ReportAnnotation *label = array_at(report->annotations, i);
      free(label->message);
    }
    array_free(report->annotations);
    free(report);
  }
}

Report *report_new_with_args(ReportKind kind, unsigned long offset, const char *format, va_list args)
{
  Report *report      = xmalloc(sizeof(Report));
  report->kind        = kind;
  report->offset      = offset;
  report->message     = vformat(format, args);
  report->notes       = array_new(sizeof(char *));
  report->annotations = array_new(sizeof(ReportAnnotation));
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
  array_push(report->annotations, &label);
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
  array_push(report->notes, &note);
}

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

static void write_head_line(Writer *writer, Canvas *canvas)
{
  switch (writer->report->kind) {
  case REPORT_KIND_ERROR:
    canvas_style(canvas, TERM_BOLD);
    canvas_style(canvas, TERM_FG_BRIGHT_RED);
    canvas_write(canvas, "[ERROR] ");
    break;
  case REPORT_KIND_WARN:
    canvas_style(canvas, TERM_FG_BRIGHT_YELLOW);
    canvas_write(canvas, "[WARN] ");
    break;
  case REPORT_KIND_NOTE:
    canvas_style(canvas, TERM_FG_BRIGHT_GREEN);
    canvas_write(canvas, "[NOTE] ");
    break;
  }
  canvas_style(canvas, TERM_RESET);

  canvas_style(canvas, TERM_FG_BRIGHT_WHITE);
  canvas_write(canvas, "%s", writer->report->message);
  canvas_style(canvas, TERM_RESET);
  canvas_next_line(canvas);
}

static void write_location_line(Writer *writer, Canvas *canvas)
{
  SourceLocation location;
  source_location(writer->source, writer->report->offset, &location);
  canvas_style(canvas, TERM_FAINT);
  canvas_write(canvas, " %*.s ╭─[", writer->number_margin, "");
  canvas_style(canvas, TERM_RESET);

  canvas_style(canvas, TERM_FG_BRIGHT_WHITE);
  canvas_write(canvas, "%s:%lu:%lu", writer->source->file_name, location.line + 1, location.column + 1);
  canvas_style(canvas, TERM_RESET);

  canvas_style(canvas, TERM_FAINT);
  canvas_write(canvas, "]");
  canvas_style(canvas, TERM_RESET);
  canvas_next_line(canvas);
}

static void write_annotation_left(
  Writer                 *writer,
  Canvas                 *canvas,
  unsigned long           line_number,
  unsigned long           line_column,
  const ReportAnnotation *connect,
  int                     dotted)
{
  const ReportAnnotation *strike = NULL;
  unsigned long           i;

  canvas_style(canvas, TERM_FG_BRIGHT_RED);
  for (i = 0; i < array_count(writer->report->annotations); ++i) {
    ReportAnnotation *annotation = array_at(writer->report->annotations, i);
    if (annotation->start.line != annotation->end.line) {
      if (strike) {
        canvas_write(canvas, "──");
      } else if (line_number < annotation->start.line || line_number > annotation->end.line) {
        canvas_write(canvas, "  ");
      } else if (line_number == annotation->start.line) {
        if (line_column == -1ul) {
          canvas_write(canvas, "  ");
        } else if (line_column > annotation->start.column) {
          canvas_write(canvas, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->start.column) {
          canvas_write(canvas, "  ");
        } else if (annotation == connect) {
          canvas_write(canvas, "╭─");
          strike = annotation;
        } else {
          canvas_write(canvas, dotted ? "╎ " : "│ ");
        }
      } else if (line_number == annotation->end.line) {
        if (line_column == -1ul) {
          canvas_write(canvas, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->end.column) {
          canvas_write(canvas, dotted ? "╎ " : "│ ");
        } else if (line_column > annotation->end.column) {
          canvas_write(canvas, "  ");
        } else if (annotation == connect) {
          canvas_write(canvas, "╰─");
          strike = annotation;
        } else {
          canvas_write(canvas, "  ");
        }
      } else {
        canvas_write(canvas, dotted ? "╎ " : "│ ");
      }
    }
  }
  canvas_style(canvas, TERM_RESET);
}

static void write_source_line(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long line_width;
  char         *line;

  unsigned long line_offset;
  unsigned long column_offset;

  Array *segments    = array_new(sizeof(LineSegment));
  Array *nongraphics = array_new(sizeof(LineSegment));

  line_width = 0;
  for (i = 0; i < writer->source->line_lengths[line_number]; ++i) {
    char c = writer->source->text[writer->source->line_offsets[line_number] + i];
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
  for (i = 0; i < writer->source->line_lengths[line_number]; ++i) {
    char c = writer->source->text[writer->source->line_offsets[line_number] + i];
    if (c == '\t') {
      unsigned long adjusted_width = writer->tab_width - (line_offset % writer->tab_width);
      line_offset += sprintf(line + line_offset, "%*.s", (int) adjusted_width, "");
    } else if (!is_graphic(c)) {
      LineSegment segment;
      segment.annotation = NULL;

      segment.start = line_offset;
      line_offset += sprintf(line + line_offset, "\\x%X", (unsigned char) (c & 0xFF));
      segment.end = line_offset - 1;
      array_push(nongraphics, &segment);
    } else {
      line[line_offset++] = writer->source->text[writer->source->line_offsets[line_number] + i];
    }
  }
  line[line_width] = '\0';

  for (i = 0; i < array_count(writer->report->annotations); ++i) {
    ReportAnnotation *annotation = array_at(writer->report->annotations, i);
    LineSegment       segment;
    segment.annotation = annotation;
    if (annotation->start.line == line_number && annotation->end.line == line_number) {
      segment.start = annotation->start.column;
      segment.end   = annotation->end.column;
      array_push(segments, &segment);
    } else if (annotation->start.line == line_number) {
      segment.start = annotation->start.column;
      segment.end   = line_width;
      array_push(segments, &segment);
    } else if (annotation->end.line == line_number) {
      segment.start = 0;
      segment.end   = annotation->end.column;
      array_push(segments, &segment);
    }
  }
  qsort(array_data(segments), array_count(segments), sizeof(LineSegment), &compare_line_segments);

  canvas_style(canvas, TERM_FAINT);
  canvas_write(canvas, " %*.lu │ ", writer->number_margin, line_number + 1);
  canvas_style(canvas, TERM_RESET);

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = canvas_line(canvas);
  column_offset = canvas_column(canvas);
  canvas_style(canvas, TERM_FG_BRIGHT_WHITE);
  canvas_write(canvas, "%s", line);
  canvas_style(canvas, TERM_RESET);

  for (j = 0; j < array_count(nongraphics); ++j) {
    LineSegment *nongraphic = array_at(nongraphics, j);
    canvas_seek(canvas, line_offset, column_offset + nongraphic->start);
    canvas_style(canvas, TERM_FAINT);
    canvas_write(canvas, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
    canvas_style(canvas, TERM_RESET);
  }

  for (i = 0; i < array_count(segments); ++i) {
    LineSegment *segment = array_at(segments, i);
    canvas_seek(canvas, line_offset, column_offset + segment->start);
    canvas_style(canvas, TERM_FG_BRIGHT_RED);
    canvas_write(canvas, "%.*s", (int) (segment->end - segment->start + 1), line + segment->start);
    canvas_style(canvas, TERM_RESET);

    for (j = 0; j < array_count(nongraphics); ++j) {
      LineSegment *nongraphic = array_at(nongraphics, j);
      if (nongraphic->start >= segment->start && nongraphic->end <= segment->end) {
        canvas_seek(canvas, line_offset, column_offset + nongraphic->start);
        canvas_style(canvas, TERM_FAINT);
        canvas_style(canvas, TERM_FG_BRIGHT_RED);
        canvas_write(canvas, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
        canvas_style(canvas, TERM_RESET);
      }
    }
  }
  canvas_next_line(canvas);

  free(line);
  array_free(nongraphics);
  array_free(segments);
}

static void write_indicator_line(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long line_offset;
  unsigned long column_offset;

  Array *indicators = array_new(sizeof(Indicator));
  for (i = 0; i < array_count(writer->report->annotations); ++i) {
    ReportAnnotation *annotation = array_at(writer->report->annotations, i);
    Indicator         indicator;
    if (annotation->start.line == line_number && annotation->end.line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_INLINE;
      indicator.column     = annotation->start.column;
      indicator.length     = annotation->end.column - annotation->start.column + 1;
      array_push(indicators, &indicator);
    } else if (annotation->start.line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_BEGIN;
      indicator.column     = annotation->start.column;
      indicator.length     = 1;
      array_push(indicators, &indicator);
    } else if (annotation->end.line == line_number) {
      indicator.annotation = annotation;
      indicator.kind       = INDICATOR_END;
      indicator.column     = annotation->end.column;
      indicator.length     = 1;
      array_push(indicators, &indicator);
    }
  }
  qsort(array_data(indicators), array_count(indicators), sizeof(Indicator), &compare_indicators);

  canvas_style(canvas, TERM_FAINT);
  canvas_write(canvas, " %*.s │ ", writer->number_margin, "");
  canvas_style(canvas, TERM_RESET);

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = canvas_line(canvas);
  column_offset = canvas_column(canvas);
  for (i = 0; i < array_count(indicators); ++i) {
    Indicator *indicator = array_at(indicators, i);
    canvas_seek(canvas, line_offset, column_offset + indicator->column);
    canvas_style(canvas, TERM_FG_BRIGHT_RED);
    switch (indicator->kind) {
    case INDICATOR_INLINE:
      canvas_write(canvas, indicator->annotation->message ? "┬" : "─");
      for (j = 1; j < indicator->length; ++j) {
        canvas_write(canvas, "─");
      }
      break;

    case INDICATOR_END:
    case INDICATOR_BEGIN:
      canvas_write(canvas, "▲");
      break;
    }
    canvas_style(canvas, TERM_RESET);
  }
  canvas_next_line(canvas);

  array_free(indicators);
}

static void write_annotation_lines(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long label_offset = 0;
  unsigned long line_offset;
  unsigned long column_offset;
  unsigned long end_line_offset;
  unsigned long depth;

  Array *connectors = array_new(sizeof(Connector));
  for (i = 0; i < array_count(writer->report->annotations); ++i) {
    ReportAnnotation *annotation = array_at(writer->report->annotations, i);
    Connector         connector;
    connector.annotation = annotation;

    if (annotation->start.line == line_number && annotation->end.line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 0;
      connector.column    = annotation->start.column;
      connector.depth     = -1ul;
      array_push(connectors, &connector);
    } else if (annotation->start.line == line_number) {
      connector.kind      = CONNECTOR_BEGIN;
      connector.multiline = 1;
      connector.column    = annotation->start.column;
      connector.depth     = -1ul;
      array_push(connectors, &connector);
    } else if (annotation->end.line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 1;
      connector.column    = annotation->end.column;
      connector.depth     = -1ul;
      array_push(connectors, &connector);
    }

    if (annotation->start.line == line_number && label_offset < annotation->start.column) {
      label_offset = annotation->start.column;
    }
    if (annotation->end.line == line_number && label_offset < annotation->end.column) {
      label_offset = annotation->end.column;
    }
  }
  qsort(array_data(connectors), array_count(connectors), sizeof(Connector), &compare_connectors);

  depth = 0;
  for (i = 0; i < array_count(connectors); ++i) {
    Connector *connector = array_at(connectors, i);
    if (connector->multiline || connector->annotation->message) {
      connector->depth = depth;
      if (depth > 0) {
        canvas_next_line(canvas);
      }
      canvas_style(canvas, TERM_FAINT);
      canvas_write(canvas, " %*.s │ ", writer->number_margin, "");
      canvas_style(canvas, TERM_RESET);
      write_annotation_left(writer, canvas, line_number, connector->column, connector->annotation, 0);
      if (depth == 0) {
        line_offset   = canvas_line(canvas);
        column_offset = canvas_column(canvas);
      }
      canvas_next_line(canvas);
      canvas_style(canvas, TERM_FAINT);
      canvas_write(canvas, " %*.s │ ", writer->number_margin, "");
      canvas_style(canvas, TERM_RESET);
      write_annotation_left(writer, canvas, line_number, connector->column, NULL, 0);
      depth += 2;
    }
  }
  end_line_offset = canvas_line(canvas);

  for (i = array_count(connectors); i > 0; --i) {
    Connector *connector = array_at(connectors, i - 1);

    canvas_style(canvas, TERM_FG_BRIGHT_RED);
    if (connector->depth != -1ul) {
      for (j = 0; j < connector->depth; ++j) {
        canvas_seek(canvas, line_offset + j, column_offset + connector->column);
        canvas_write(canvas, "│");
      }
    }
    canvas_style(canvas, TERM_RESET);
    switch (connector->kind) {
    case CONNECTOR_END:
      canvas_style(canvas, TERM_FG_BRIGHT_RED);
      if (connector->multiline) {
        canvas_seek(canvas, line_offset + j, column_offset);
        for (j = 0; j < connector->column; ++j) {
          canvas_write(canvas, "─");
        }
        canvas_write(canvas, connector->annotation->message ? "┴" : "╯");
      } else if (connector->annotation->message) {
        canvas_seek(canvas, line_offset + j, column_offset + connector->column);
        canvas_write(canvas, "╰");
      }

      if (connector->annotation->message) {
        for (j = connector->column + 1; j < label_offset + 3; ++j) {
          canvas_write(canvas, "─");
        }
        canvas_style(canvas, TERM_RESET);

        canvas_style(canvas, TERM_FG_BRIGHT_WHITE);
        canvas_write(canvas, " %s", connector->annotation->message);
      }
      canvas_style(canvas, TERM_RESET);
      break;

    case CONNECTOR_BEGIN:
      canvas_seek(canvas, line_offset + j, column_offset);
      canvas_style(canvas, TERM_FG_BRIGHT_RED);
      for (j = 0; j < connector->column; ++j) {
        canvas_write(canvas, "─");
      }
      canvas_write(canvas, "╯");
      canvas_style(canvas, TERM_RESET);
      break;
    }
  }
  canvas_seek(canvas, end_line_offset, 0);
  array_free(connectors);
}

static void write_interest_lines(Writer *writer, Canvas *canvas)
{
  unsigned long i, j;

  unsigned long start_line    = -1ul;
  unsigned long end_line      = 0;
  unsigned long previous_line = -1ul;

  for (i = 0; i < array_count(writer->report->annotations); ++i) {
    ReportAnnotation *annotation = array_at(writer->report->annotations, i);
    if (start_line > annotation->start.line) {
      start_line = annotation->start.line;
    }
    if (end_line < annotation->end.line) {
      end_line = annotation->end.line;
    }
  }

  for (i = start_line; i <= end_line; ++i) {
    for (j = 0; j < array_count(writer->report->annotations); ++j) {
      ReportAnnotation *annotation = array_at(writer->report->annotations, j);
      if (i == annotation->start.line || i == annotation->end.line) {
        int dotted = previous_line != -1ul && previous_line + 1 != i;
        canvas_style(canvas, TERM_FAINT);
        canvas_write(canvas, " %*.s %s", writer->number_margin, "", dotted ? "╎ " : "│ ");
        canvas_style(canvas, TERM_RESET);
        write_annotation_left(writer, canvas, i, -1ul, NULL, dotted);
        canvas_next_line(canvas);

        write_source_line(writer, canvas, i);
        write_indicator_line(writer, canvas, i);
        write_annotation_lines(writer, canvas, i);
        previous_line = i;
        break;
      }
    }
  }
}

static void write_tail_line(Writer *writer, Canvas *canvas)
{
  int i;
  canvas_style(canvas, TERM_FAINT);
  canvas_write(canvas, "─");
  for (i = 0; i <= writer->number_margin; ++i) {
    canvas_write(canvas, "─");
  }
  canvas_write(canvas, "╯");
  canvas_style(canvas, TERM_RESET);
  canvas_next_line(canvas);
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

static void display_location(const Source *source, unsigned long offset, unsigned long tab_width, int start, SourceLocation *location)
{
  unsigned long i;
  unsigned long column = 0;

  if (!start) {
    --offset;
  }
  source_location(source, offset, location);
  if (!start) {
    ++location->column;
  }

  for (i = 0; i < location->column; ++i) {
    char c = source->text[source->line_offsets[location->line] + i];
    if (c == '\t') {
      column += tab_width - (column % tab_width);
    } else if (!is_graphic(c)) {
      column += strlen("\\xXX");
    } else {
      ++column;
    }
  }
  location->column = column;

  if (!start) {
    --location->column;
  }
}

void report_emit(Report *report, const Source *source)
{
  Writer        writer;
  Canvas       *canvas = canvas_new();
  unsigned long i;

  qsort(array_data(report->annotations), array_count(report->annotations), sizeof(ReportAnnotation), &compare_annotations);

  writer.report        = report;
  writer.source        = source;
  writer.tab_width     = 4;
  writer.number_margin = 0;
  for (i = 0; i < array_count(report->annotations); ++i) {
    int               margin;
    ReportAnnotation *annotation = array_at(report->annotations, i);
    display_location(source, annotation->start_offset, writer.tab_width, 1, &annotation->start);
    display_location(source, annotation->end_offset, writer.tab_width, 0, &annotation->end);

    margin = digits(annotation->start.line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
    margin = digits(annotation->end.line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
  }

  write_head_line(&writer, canvas);
  write_location_line(&writer, canvas);
  write_interest_lines(&writer, canvas);
  write_tail_line(&writer, canvas);

  canvas_print(canvas, stderr);
  canvas_free(canvas);
  report_free(report);
}
