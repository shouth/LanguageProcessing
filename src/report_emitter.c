#include <stddef.h>
#include <stdlib.h>

#include "array.h"
#include "canvas.h"
#include "report.h"
#include "source.h"

static void draw_head_line(Canvas *canvas, const Report *report)
{
  switch (report->_kind) {
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
  canvas_draw(canvas, "%s", report->_message);
  canvas_style(canvas, CANVAS_RESET);
}

static void draw_location_line(Canvas *canvas, const Report *report, const Source *source, int number_margin)
{
  SourceLocation location;
  source_location(source, report->_offset, &location);
  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s ╭─[", number_margin, "");
  canvas_style(canvas, CANVAS_RESET);
  canvas_style_foreground(canvas, CANVAS_4BIT | 97);
  canvas_draw(canvas, "%s:%lu:%lu", source->file_name, location.line + 1, location.column + 1);
  canvas_style(canvas, CANVAS_RESET);
  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, "]");
  canvas_style(canvas, CANVAS_RESET);
}

static int compare_annotations_source_segment(const void *left, const void *right)
{
  const ReportAnnotation *left_annotation  = left;
  const ReportAnnotation *right_annotation = right;

  if (left_annotation->_start_offset != right_annotation->_start_offset) {
    return left_annotation->_start_offset < right_annotation->_start_offset ? -1 : 1;
  } else if (left_annotation->_end_offset != right_annotation->_end_offset) {
    return left_annotation->_end_offset > right_annotation->_end_offset ? -1 : 1;
  } else {
    return 0;
  }
}

static void draw_interest_source(
  Canvas           *canvas,
  ReportAnnotation *annotations,
  unsigned long     count,
  const Source     *source,
  unsigned long     line_number,
  int               tab_width)
{
  unsigned long i, j;
  unsigned long line_width;
  char         *line;

  unsigned long line_offset;
  unsigned long column_offset;

  line_width = 0;
  for (i = 0; i < source->line_lengths[line_number]; ++i) {
    if (source->text[source->line_offsets[line_number] + i] == '\t') {
      line_width += tab_width - (line_width % tab_width);
    } else {
      ++line_width;
    }
  }

  line        = malloc(line_width + 1);
  line_offset = 0;
  for (i = 0; i < source->line_lengths[line_number]; ++i) {
    if (source->text[source->line_offsets[line_number] + i] == '\t') {
      unsigned long adjusted_width = tab_width - (line_offset % tab_width);
      for (j = 0; j < adjusted_width; ++j) {
        line[line_offset] = ' ';
        ++line_offset;
      }
    } else {
      line[line_offset] = source->text[source->line_offsets[line_number] + i];
      ++line_offset;
    }
  }
  line[line_width] = '\0';

  qsort(annotations, count, sizeof(ReportAnnotation), &compare_annotations_source_segment);

  canvas_position(canvas, &line_offset, &column_offset);
  canvas_style_foreground(canvas, CANVAS_4BIT | 97);
  canvas_draw(canvas, "%s", line);
  canvas_style(canvas, CANVAS_RESET);

  for (i = 0; i < count; ++i) {
    unsigned long annotation_start
      = annotations[i]._start.line == line_number ? annotations[i]._start.column : 0;
    unsigned long annotation_end
      = annotations[i]._end.line == line_number ? annotations[i]._end.column + 1 : line_width;
    canvas_seek(canvas, line_offset, column_offset + annotation_start);
    canvas_style_foreground(canvas, CANVAS_4BIT | 91);
    canvas_draw(canvas, "%.*s", (int) (annotation_end - annotation_start), line + annotation_start);
    canvas_style(canvas, CANVAS_RESET);
  }
  free(line);
}

static void draw_connector_left_part(
  Canvas                 *canvas,
  const ReportAnnotation *annotations,
  const unsigned char    *flags,
  unsigned long           count)
{
  unsigned long i;
  for (i = 0; i < count; ++i) {
    if (flags[i]) {
      canvas_draw(canvas, "│ ");
    } else {
      canvas_draw(canvas, "  ");
    }
  }
}

static void draw_indicator_line(
  Canvas           *canvas,
  ReportAnnotation *line_annotations,
  unsigned long     line_annotation_count,
  ReportAnnotation *multiline_annotations,
  unsigned char    *multiline_annotation_flags,
  unsigned long     multiline_annotation_count,
  unsigned long     line_number,
  int               number_margin)
{
  unsigned long i, j;
  unsigned long line_offset;
  unsigned long column_offset;

  qsort(line_annotations, line_annotation_count, sizeof(ReportAnnotation), &compare_annotations_source_segment);

  canvas_style(canvas, CANVAS_FAINT);
  canvas_draw(canvas, " %*.s │ ", number_margin, "");
  canvas_style(canvas, CANVAS_RESET);
  draw_connector_left_part(
    canvas, multiline_annotations, multiline_annotation_flags, multiline_annotation_count);

  canvas_position(canvas, &line_offset, &column_offset);
  canvas_style_foreground(canvas, CANVAS_4BIT | 91);
  for (i = 0; i < line_annotation_count; ++i) {
    if (line_annotations[i]._start.line == line_annotations[i]._end.line) {
      canvas_seek(canvas, line_offset, column_offset + line_annotations[i]._start.column);
      canvas_draw(canvas, "┬");
      for (j = line_annotations[i]._start.column + 1; j <= line_annotations[i]._end.column; ++j) {
        canvas_draw(canvas, "─");
      }
    }
  }
  canvas_style(canvas, CANVAS_RESET);

  canvas_style_foreground(canvas, CANVAS_4BIT | 91);
  for (i = 0; i < line_annotation_count; ++i) {
    if (line_annotations[i]._start.line != line_annotations[i]._end.line) {
      if (line_annotations[i]._start.line == line_number) {
        canvas_seek(canvas, line_offset, column_offset + line_annotations[i]._start.column);
        canvas_draw(canvas, "▲");
      } else {
        canvas_seek(canvas, line_offset, column_offset + line_annotations[i]._end.column);
        canvas_draw(canvas, "▲");
      }
    }
  }
  canvas_style(canvas, CANVAS_RESET);
}

static int compare_annotations_connector(const void *left, const void *right)
{
  const ReportAnnotation *left_annotation  = left;
  const ReportAnnotation *right_annotation = right;

  if (left_annotation->_start.line != right_annotation->_start.line) {
    return left_annotation->_start.line > right_annotation->_start.line ? -1 : 1;
  } else if (left_annotation->_end.line != right_annotation->_end.line) {
    return left_annotation->_end.line > right_annotation->_end.line ? -1 : 1;
  } else {
    return 0;
  }
}

static void draw_annotation_lines(
  Canvas           *canvas,
  ReportAnnotation *local_annotations,
  unsigned long     local_count,
  ReportAnnotation *multiline_annotations,
  unsigned char    *multiline_annotations_flags,
  unsigned long     multiline_count,
  unsigned long     line_number,
  int               number_margin)
{
  unsigned long i;
  unsigned long column_end;

  qsort(local_annotations, local_count, sizeof(ReportAnnotation), &compare_annotations_connector);

  column_end = -1ul;
  for (i = 0; i < local_count; ++i) {
    if (local_annotations[i]._end.line == line_number) {
      if (column_end == -1ul || column_end < local_annotations[i]._end.column) {
        column_end = local_annotations[i]._end.column;
      }
    }
  }
}

static void draw_interest_lines(Canvas *canvas, const Report *report, const Source *source, int number_margin, int tab_width)
{
  Array         multiline_annotations;
  Array         multiline_annotations_flags;
  unsigned long i, j;

  unsigned long start_line    = -1ul;
  unsigned long end_line      = 0;
  unsigned long previous_line = -1ul;

  array_init(&multiline_annotations, sizeof(ReportAnnotation));
  array_init(&multiline_annotations_flags, sizeof(unsigned char));
  for (i = 0; i < array_count(&report->_annotations); ++i) {
    ReportAnnotation *annotation = array_at(&report->_annotations, i);
    if (annotation->_start.line != annotation->_end.line) {
      unsigned char flag = 0;
      array_push(&multiline_annotations, annotation);
      array_push(&multiline_annotations_flags, &flag);
    }

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

    for (j = 0; j < array_count(&report->_annotations); ++j) {
      ReportAnnotation *annotation = array_at(&report->_annotations, j);
      if (i == annotation->_start.line || i == annotation->_end.line) {
        array_push(&annotations, annotation);
      }
    }

    if (array_count(&annotations)) {
      canvas_style(canvas, CANVAS_FAINT);
      if (previous_line != -1ul && previous_line + 1 != i) {
        canvas_draw(canvas, " %*.s ┆", number_margin, "");
      } else {
        canvas_draw(canvas, " %*.s │", number_margin, "");
      }
      canvas_next_line(canvas);

      canvas_draw(canvas, " %*.lu │ ", number_margin, i + 1);
      canvas_style(canvas, CANVAS_RESET);
      draw_connector_left_part(
        canvas,
        array_data(&multiline_annotations),
        array_data(&multiline_annotations_flags),
        array_count(&multiline_annotations));
      draw_interest_source(
        canvas, array_data(&annotations), array_count(&annotations), source, i, tab_width);
      canvas_next_line(canvas);
      draw_indicator_line(
        canvas,
        array_data(&annotations), array_count(&annotations),
        array_data(&multiline_annotations), array_data(&multiline_annotations_flags),
        array_count(&multiline_annotations),
        i, number_margin);
      draw_annotation_lines(
        canvas,
        array_data(&annotations), array_count(&annotations),
        array_data(&multiline_annotations), array_data(&multiline_annotations_flags),
        array_count(&multiline_annotations),
        i, number_margin);
    }
    array_deinit(&annotations);
  }

  array_deinit(&multiline_annotations);
  array_deinit(&multiline_annotations_flags);
}

static void draw_tail_lines(Canvas *canvas, int number_margin)
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
  unsigned long i;
  int           number_margin;
  int           tab_width = 4;

  Canvas canvas;
  canvas_init(&canvas);

  number_margin = 0;
  for (i = 0; i < array_count(&report->_annotations); ++i) {
    int               margin;
    ReportAnnotation *annotation = array_at(&report->_annotations, i);
    display_location(source, annotation->_start_offset, tab_width, &annotation->_start);
    display_location(source, annotation->_end_offset - 1, tab_width, &annotation->_end);

    margin = digits(annotation->_start.line + 1);
    if (number_margin < margin) {
      number_margin = margin;
    }
    margin = digits(annotation->_end.line + 1);
    if (number_margin < margin) {
      number_margin = margin;
    }
  }

  draw_head_line(&canvas, report);
  canvas_next_line(&canvas);

  draw_location_line(&canvas, report, source, number_margin);
  canvas_next_line(&canvas);

  draw_interest_lines(&canvas, report, source, number_margin, tab_width);
  canvas_next_line(&canvas);

  draw_tail_lines(&canvas, number_margin);
  canvas_next_line(&canvas);

  canvas_print(&canvas, stderr);
  canvas_deinit(&canvas);
  report_deinit(report);
}
