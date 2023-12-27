#include <stddef.h>
#include <stdlib.h>

#include "array.h"
#include "canvas.h"
#include "report.h"
#include "source.h"

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
  unsigned long start;
  unsigned long end;
};

struct Indicator {
  IndicatorKind kind;
  unsigned long column;
  unsigned long length;
};

struct Connector {
  const ReportAnnotation *annotation;
  ConnectorKind           kind;
  int                     multiline;
  unsigned long           column;
};

struct Writer {
  const Report *report;
  const Source *source;
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

  if (left_connector->kind != right_connector->kind) {
    return left_connector->kind < right_connector->kind ? -1 : 1;
  } else if (left_connector->column != right_connector->column) {
    return left_connector->column < right_connector->column ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_annotations(const void *left, const void *right)
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

static void write_head_line(Writer *writer, Canvas *canvas)
{
  switch (writer->report->_kind) {
  case REPORT_KIND_ERROR:
    canvas_style(canvas, CANVAS_BOLD);
    canvas_style_foreground(canvas, CANVAS_4BIT | 91);
    canvas_draw(canvas, "[ERROR] ");
    break;
  case REPORT_KIND_WARN:
    canvas_style_foreground(canvas, CANVAS_4BIT | 93);
    canvas_draw(canvas, "[WARN] ");
    break;
  case REPORT_KIND_NOTE:
    canvas_style_foreground(canvas, CANVAS_4BIT | 96);
    canvas_draw(canvas, "[NOTE] ");
    break;
  }
  canvas_style(canvas, CANVAS_RESET);
  canvas_style_foreground(canvas, CANVAS_4BIT | 97);
  canvas_draw(canvas, "%s", writer->report->_message);
  canvas_style(canvas, CANVAS_RESET);
}

static void write_location_line(Writer *writer, Canvas *canvas)
{
  SourceLocation location;
  source_location(writer->source, writer->report->_offset, &location);
  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s ╭─[", writer->number_margin, "");
  canvas_style(canvas, CANVAS_RESET);
  canvas_style_foreground(canvas, CANVAS_4BIT | 97);
  canvas_draw(canvas, "%s:%lu:%lu", writer->source->file_name, location.line + 1, location.column + 1);
  canvas_style(canvas, CANVAS_RESET);
  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, "]");
  canvas_style(canvas, CANVAS_RESET);
}

static void write_annotation_left(Writer *writer, Canvas *canvas, unsigned long line_number, unsigned long line_column, int connect)
{
  const ReportAnnotation *strike = NULL;
  unsigned long i;

  for (i = 0; i < array_count(&writer->report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&writer->report->_annotations, i);
    if (annotation->_start.line != annotation->_end.line) {
      if (strike) {
        canvas_draw(canvas, "──");
      } else if (line_number < annotation->_start.line || line_number > annotation->_end.line) {
        canvas_draw(canvas, "  ");
      } else if (line_number < annotation->_end.line) {
        canvas_draw(canvas, "│ ");
      } else if (line_column < annotation->_end.column) {
        canvas_draw(canvas, "│ ");
      } else if (line_column > annotation->_end.column) {
        canvas_draw(canvas, "  ");
      } else if (connect) {
        canvas_draw(canvas, "╰─");
        strike = annotation;
      } else {
        canvas_draw(canvas, "│ ");
      }
    }
  }
}

static void write_source_line(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long line_width;
  char         *line;

  unsigned long line_offset;
  unsigned long column_offset;

  Array segments;

  line_width = 0;
  for (i = 0; i < writer->source->line_lengths[line_number]; ++i) {
    if (writer->source->text[writer->source->line_offsets[line_number] + i] == '\t') {
      line_width += writer->tab_width - (line_width % writer->tab_width);
    } else {
      ++line_width;
    }
  }

  line        = malloc(line_width + 1);
  line_offset = 0;
  for (i = 0; i < writer->source->line_lengths[line_number]; ++i) {
    if (writer->source->text[writer->source->line_offsets[line_number] + i] == '\t') {
      unsigned long adjusted_width = writer->tab_width - (line_offset % writer->tab_width);
      for (j = 0; j < adjusted_width; ++j) {
        line[line_offset] = ' ';
        ++line_offset;
      }
    } else {
      line[line_offset] = writer->source->text[writer->source->line_offsets[line_number] + i];
      ++line_offset;
    }
  }
  line[line_width] = '\0';

  array_init(&segments, sizeof(LineSegment));
  for (i = 0; i < array_count(&writer->report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&writer->report->_annotations, i);
    LineSegment       segment;
    if (annotation->_start.line == line_number && annotation->_end.line == line_number) {
      segment.start = annotation->_start.column;
      segment.end   = annotation->_end.column;
      array_push(&segments, &segment);
    } else if (annotation->_start.line == line_number) {
      segment.start = annotation->_start.column;
      segment.end   = line_width;
      array_push(&segments, &segment);
    } else if (annotation->_end.line == line_number) {
      segment.start = 0;
      segment.end   = annotation->_end.column;
      array_push(&segments, &segment);
    }
  }
  qsort(array_data(&segments), array_count(&segments), sizeof(LineSegment), &compare_line_segments);

  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.lu │ ", writer->number_margin, line_number + 1);
  canvas_style(canvas, CANVAS_RESET);

  write_annotation_left(writer, canvas, line_number, 0, 0);
  canvas_position(canvas, &line_offset, &column_offset);
  canvas_style_foreground(canvas, CANVAS_4BIT | 97);
  canvas_draw(canvas, "%s", line);
  canvas_style(canvas, CANVAS_RESET);

  for (i = 0; i < array_count(&segments); ++i) {
    LineSegment *segment = array_at(&segments, i);
    canvas_seek(canvas, line_offset, column_offset + segment->start);
    canvas_style_foreground(canvas, CANVAS_4BIT | 91);
    canvas_draw(canvas, "%.*s", (int) (segment->end - segment->start + 1), line + segment->start);
    canvas_style(canvas, CANVAS_RESET);
  }

  free(line);
  array_deinit(&segments);
}

static void write_indicator_line(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long line_offset;
  unsigned long column_offset;

  Array indicators;
  array_init(&indicators, sizeof(Indicator));
  for (i = 0; i < array_count(&writer->report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&writer->report->_annotations, i);
    Indicator         indicator;
    if (annotation->_start.line == line_number && annotation->_end.line == line_number) {
      indicator.kind   = INDICATOR_INLINE;
      indicator.column = annotation->_start.column;
      indicator.length = annotation->_end.column - annotation->_start.column + 1;
      array_push(&indicators, &indicator);
    } else if (annotation->_start.line == line_number) {
      indicator.kind   = INDICATOR_BEGIN;
      indicator.column = annotation->_start.column;
      indicator.length = 1;
      array_push(&indicators, &indicator);
    } else if (annotation->_end.line == line_number) {
      indicator.kind   = INDICATOR_END;
      indicator.column = annotation->_end.column;
      indicator.length = 1;
      array_push(&indicators, &indicator);
    }
  }
  qsort(array_data(&indicators), array_count(&indicators), sizeof(Indicator), &compare_indicators);

  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s │ ", writer->number_margin, "");
  canvas_style(canvas, CANVAS_RESET);

  write_annotation_left(writer, canvas, line_number, 0, 0);
  canvas_position(canvas, &line_offset, &column_offset);
  for (i = 0; i < array_count(&indicators); ++i) {
    Indicator *indicator = array_at(&indicators, i);
    canvas_seek(canvas, line_offset, column_offset + indicator->column);
    canvas_style_foreground(canvas, CANVAS_4BIT | 91);
    switch (indicator->kind) {
    case INDICATOR_INLINE:
      canvas_draw(canvas, "┬");
      for (j = 1; j < indicator->length; ++j) {
        canvas_draw(canvas, "─");
      }
      break;

    case INDICATOR_END:
    case INDICATOR_BEGIN:
      canvas_draw(canvas, "▲");
      break;
    }
    canvas_style(canvas, CANVAS_RESET);
  }

  array_deinit(&indicators);
}

static void write_annotation_lines(Writer *writer, Canvas *canvas, unsigned long line_number)
{
  unsigned long i, j;
  unsigned long label_offset = -1ul;
  unsigned long line_offset;
  unsigned long column_offset;

  Array connectors;
  array_init(&connectors, sizeof(Connector));
  for (i = 0; i < array_count(&writer->report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&writer->report->_annotations, i);
    Connector         connector;
    connector.annotation = annotation;

    if (annotation->_start.line == line_number && annotation->_end.line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 0;
      connector.column    = annotation->_start.column;
      array_push(&connectors, &connector);
    } else if (annotation->_start.line == line_number) {
      connector.kind      = CONNECTOR_BEGIN;
      connector.multiline = 1;
      connector.column    = annotation->_start.column;
      array_push(&connectors, &connector);
    } else if (annotation->_end.line == line_number) {
      connector.kind      = CONNECTOR_END;
      connector.multiline = 1;
      connector.column    = annotation->_end.column;
      array_push(&connectors, &connector);
    }

    if (annotation->_end.line == line_number && label_offset > annotation->_end.column) {
      label_offset = annotation->_end.column;
    }
  }
  qsort(array_data(&connectors), array_count(&connectors), sizeof(Connector), &compare_connectors);

  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s │ ", writer->number_margin, "");
  write_annotation_left(writer, canvas, line_number, 0, 0);
  canvas_position(canvas, &line_offset, &column_offset);
  for (i = 1; i < 2 * array_count(&connectors) - 1; ++i) {
    canvas_next_line(canvas);
    canvas_draw(canvas, " %*.s │ ", writer->number_margin, "");
    write_annotation_left(writer, canvas, line_number, 0, i % 2);
  }
  canvas_style(canvas, CANVAS_RESET);

  for (i = array_count(&connectors); i > 0; --i) {
    Connector *connector = array_at(&connectors, i - 1);

    canvas_style_foreground(canvas, CANVAS_4BIT | 91);
    for (j = 0; j < 2 * i - 2; ++j) {
      canvas_seek(canvas, line_offset + j, column_offset + connector->column);
      canvas_draw(canvas, "│");
    }
    canvas_style(canvas, CANVAS_RESET);
    switch (connector->kind) {
    case CONNECTOR_END:
      canvas_style_foreground(canvas, CANVAS_4BIT | 91);
      if (connector->multiline) {
        canvas_seek(canvas, line_offset + j, column_offset);
        for (j = 0; j < connector->column; ++j) {
          canvas_draw(canvas, "─");
        }
        canvas_draw(canvas, "┴");
      } else {
        canvas_seek(canvas, line_offset + j, column_offset + connector->column);
        canvas_draw(canvas, "╰");
      }
      for (j = connector->column + 1; j < label_offset + 3; ++j) {
        canvas_draw(canvas, "─");
      }
      canvas_style(canvas, CANVAS_RESET);
      canvas_style_foreground(canvas, CANVAS_4BIT | 97);
      canvas_draw(canvas, " %s", connector->annotation->_message);
      canvas_style(canvas, CANVAS_RESET);
      break;

    case CONNECTOR_BEGIN:
      canvas_seek(canvas, line_offset + j, column_offset);
      canvas_style_foreground(canvas, CANVAS_4BIT | 91);
      for (j = 0; j < connector->column; ++j) {
        canvas_draw(canvas, "─");
      }
      canvas_draw(canvas, "╯");
      canvas_style(canvas, CANVAS_RESET);
      break;
    }
  }

  array_deinit(&connectors);
}

static void write_interest_lines(Writer *writer, Canvas *canvas)
{
  unsigned long i, j;

  unsigned long start_line    = -1ul;
  unsigned long end_line      = 0;
  unsigned long previous_line = -1ul;

  for (i = 0; i < array_count(&writer->report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&writer->report->_annotations, i);
    if (start_line > annotation->_start.line) {
      start_line = annotation->_start.line;
    }
    if (end_line < annotation->_end.line) {
      end_line = annotation->_end.line;
    }
  }

  for (i = start_line; i <= end_line; ++i) {
    Array annotations;
    array_init(&annotations, sizeof(ReportAnnotation));

    for (j = 0; j < array_count(&writer->report->_annotations); ++j) {
      ReportAnnotation *annotation = array_at(&writer->report->_annotations, j);
      if (i == annotation->_start.line || i == annotation->_end.line) {
        array_push(&annotations, annotation);
      }
    }

    if (array_count(&annotations)) {
      if (i != start_line) {
        canvas_next_line(canvas);
      }
      canvas_style(canvas, CANVAS_FAINT);
      if (previous_line != -1ul && previous_line + 1 != i) {
        canvas_draw(canvas, " %*.s ┆", writer->number_margin, "");
      } else {
        canvas_draw(canvas, " %*.s │", writer->number_margin, "");
      }
      canvas_style(canvas, CANVAS_RESET);
      canvas_next_line(canvas);

      write_source_line(writer, canvas, i);
      canvas_next_line(canvas);
      write_indicator_line(writer, canvas, i);
      canvas_next_line(canvas);
      write_annotation_lines(writer, canvas, i);
    }
    array_deinit(&annotations);
  }
}

static void write_tail_lines(Canvas *canvas, int number_margin)
{
  int i;
  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s │", number_margin, "");
  canvas_next_line(canvas);

  canvas_draw(canvas, "─");
  for (i = 0; i <= number_margin; ++i) {
    canvas_draw(canvas, "─");
  }
  canvas_draw(canvas, "╯");
  canvas_style(canvas, CANVAS_RESET);
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

void report_emit(Report *report, const Source *source)
{
  Writer        writer;
  Canvas        canvas;
  unsigned long i;

  qsort(array_data(&report->_annotations), array_count(&report->_annotations), sizeof(ReportAnnotation), &compare_annotations);

  canvas_init(&canvas);

  writer.report        = report;
  writer.source        = source;
  writer.tab_width     = 4;
  writer.number_margin = 0;
  for (i = 0; i < array_count(&report->_annotations); ++i) {
    int               margin;
    ReportAnnotation *annotation = array_at(&report->_annotations, i);
    display_location(source, annotation->_start_offset, writer.tab_width, &annotation->_start);
    display_location(source, annotation->_end_offset - 1, writer.tab_width, &annotation->_end);

    margin = digits(annotation->_start.line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
    margin = digits(annotation->_end.line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
  }

  write_head_line(&writer, &canvas);
  canvas_next_line(&canvas);

  write_location_line(&writer, &canvas);
  canvas_next_line(&canvas);

  write_interest_lines(&writer, &canvas);
  canvas_next_line(&canvas);

  write_tail_lines(&canvas, writer.number_margin);
  canvas_next_line(&canvas);

  canvas_print(&canvas, stderr);
  canvas_deinit(&canvas);
  report_deinit(report);
}
