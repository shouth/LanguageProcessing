#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "report.h"
#include "source.h"
#include "utility.h"
#include "vector.h"

typedef struct Label Label;
typedef struct Note  Note;

struct Label {
  unsigned long start;
  unsigned long end;
  char         *message;
};

struct Note {
  unsigned long start;
  unsigned long end;
  char         *message;
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

void report_init(Report *report, ReportKind kind, unsigned long offset, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_init_with_args(report, kind, offset, format, args);
  va_end(args);
}

void report_deinit(Report *report)
{
  unsigned long i;
  free(report->_message);

  for (i = 0; i < vector_count(&report->_notes); ++i) {
    Note *note = vector_at(&report->_notes, i);
    free(note->message);
  }
  vector_deinit(&report->_notes);

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    free(label->message);
  }
  vector_deinit(&report->_labels);
}

void report_init_with_args(Report *report, ReportKind kind, unsigned long offset, const char *format, va_list args)
{
  report->_kind    = kind;
  report->_offset  = offset;
  report->_message = vformat(format, args);
  vector_init(&report->_notes, sizeof(Note));
  vector_init(&report->_labels, sizeof(Label));
}

void report_label(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_label_with_args(report, start, end, format, args);
  va_end(args);
}

void report_label_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  Label label;
  label.start   = start;
  label.end     = end;
  label.message = vformat(format, args);
  vector_push(&report->_labels, &label);
}

void report_note(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_note_with_args(report, start, end, format, args);
  va_end(args);
}

void report_note_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  Note note;
  note.start   = start;
  note.end     = end;
  note.message = vformat(format, args);
  vector_push(&report->_notes, &note);
}

static unsigned long get_number_of_digits(unsigned long number)
{
  unsigned long result = 1;
  while (number > 9) {
    number /= 10;
    ++result;
  }
  return result;
}

static unsigned long get_line_number_margin(const Report *report, const Source *source)
{
  unsigned long  i;
  unsigned long  line = 0;
  SourceLocation location;

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);

    source_location(source, label->start, &location);
    if (line < location.line) {
      line = location.line;
    }
    source_location(source, label->end, &location);
    if (line < location.line) {
      line = location.line;
    }
  }

  return get_number_of_digits(line + 1);
}

static unsigned long get_start_line_number(const Report *report, const Source *source)
{
  unsigned long  result = ULONG_MAX;
  SourceLocation location;
  unsigned long  i;

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->start, &location);
    if (result > location.line) {
      result = location.line;
    }
  }

  return result;
}

static unsigned long get_end_line_number(const Report *report, const Source *source)
{
  unsigned long  result = 0;
  SourceLocation location;
  unsigned long  i;

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->end, &location);
    if (result < location.line) {
      result = location.line;
    }
  }

  return result;
}

static int is_line_skippable(const Report *report, const Source *source, unsigned long line)
{
  SourceLocation location;
  unsigned long  i;

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->start, &location);
    if (location.line == line) {
      return 0;
    }
    source_location(source, label->end, &location);
    if (location.line == line) {
      return 0;
    }
  }

  return 1;
}

static void print_header_line(const Report *report)
{
  switch (report->_kind) {
  case REPORT_KIND_ERROR:
    fprintf(stderr, "[ERROR] ");
    break;
  case REPORT_KIND_WARN:
    fprintf(stderr, "[WARN] ");
    break;
  case REPORT_KIND_NOTE:
    fprintf(stderr, "[NOTE] ");
    break;
  }
  fprintf(stderr, "%s\n", report->_message);
}

static void print_location_line(const Report *report, const Source *source, unsigned long margin)
{
  SourceLocation location;
  source_location(source, report->_offset, &location);
  fprintf(stderr, "%*.s ╭─[%s:%lu:%lu]\n", (int) margin, "", source->_file_name, location.line + 1, location.column + 1);
}

static void print_empty_body_line(unsigned long margin)
{
  fprintf(stderr, "%*.s │\n", (int) margin, "");
}

static void print_skipped_body_line(const Report *report, const Source *source, unsigned long margin, unsigned long line_number)
{
  fprintf(stderr, "%*.s ┆\n", (int) margin, "");
}

typedef enum {
  LINE_LABEL_KIND_END,
  LINE_LABEL_KIND_START,
  LINE_LABEL_KIND_INLINE
} LineLabelKind;

typedef struct LineLabel LineLabel;

struct LineLabel {
  unsigned long offset;
  LineLabelKind kind;
  const Label  *label;
  unsigned long index;
};

static int compare_line_label(const void *left, const void *right)
{
  const LineLabel *l = left;
  const LineLabel *r = right;

  if (l->offset != r->offset) {
    return l->offset < r->offset ? -1 : 1;
  } else if (l->kind != r->kind) {
    return l->kind < r->kind ? -1 : 1;
  } else if (l->kind == LINE_LABEL_KIND_END && l->label->start != r->label->start) {
    return l->label->start > r->label->start ? -1 : 1;
  } else if (l->kind == LINE_LABEL_KIND_START && l->label->end != r->label->end) {
    return l->label->end > r->label->end ? -1 : 1;
  } else if (l->label->end != r->label->end) {
    return l->label->end < r->label->end ? -1 : 1;
  } else {
    return l->index < r->index ? -1 : 1;
  }
}

static void print_pointer_line(
  const Source *source, unsigned long margin, unsigned long line_number,
  const LineLabel *pointers, unsigned long pointer_count)
{
  SourceLine    line;
  unsigned long i;
  unsigned long offset;
  unsigned long display_offset = 0;
  source_line(source, line_number, &line);

  fprintf(stderr, "%*.s │ ", (int) margin, "");

  for (offset = line.offset; offset <= line.offset + pointers->offset; ++offset) {
    if (source_text(source)[offset] == '\t') {
      display_offset += fprintf(stderr, "%*.s", (int) (4 - display_offset % 4), "");
    } else {
      display_offset += fprintf(stderr, " ");
    }
  }

  for (i = 0; i < pointer_count; ++i) {
    /*
      1. スタックを確認する
        a. ラベルが入っている場合 
      2. 先頭のラベルを取り出す
      3. 出力可能なラベル幅を計算して出力する
        a. ラベルを完全に出力できた場合 2. に戻る
        b. ラベルを途中まで出力した場合 現在のラベルをスタックに入れる
    */
  }
  fprintf(stderr, "\n");
}

static void print_message_lines(
  const Source *source, unsigned long margin, unsigned long line_number,
  const LineLabel *messages, unsigned long message_count,
  const LineLabel *pointers, unsigned long pointer_count)
{
  SourceLine    line;
  unsigned long i;
  source_line(source, line_number, &line);

  for (i = 0; i < message_count; ++i) {
    fprintf(stderr, "%*.s │ ", (int) margin, "");
  }
  fprintf(stderr, "\n");
}

static void print_label_lines(const Report *report, const Source *source, unsigned long margin, unsigned long line_number)
{
  unsigned long i;
  unsigned long message_offset = 0;

  Vector pointers;
  Vector messages;

  vector_init(&pointers, sizeof(LineLabel));
  vector_init(&messages, sizeof(LineLabel));

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    SourceLocation start;
    SourceLocation end;
    Label         *label = vector_at(&report->_labels, i);
    source_location(source, label->start, &start);
    source_location(source, label->end, &end);

    if (start.line == line_number || end.line == line_number) {
      LineLabel line_label;
      line_label.label = label;
      line_label.index = i;
      if (start.line == end.line) {
        line_label.kind   = LINE_LABEL_KIND_INLINE;
        line_label.offset = start.column;
        vector_push(&pointers, &line_label);
        vector_push(&messages, &line_label);
      } else if (start.line == line_number) {
        line_label.kind   = LINE_LABEL_KIND_START;
        line_label.offset = start.column;
        vector_push(&pointers, &line_label);
      } else {
        line_label.kind   = LINE_LABEL_KIND_END;
        line_label.offset = end.column;
        vector_push(&pointers, &line_label);
        vector_push(&messages, &line_label);
      }

      if (message_offset < line_label.offset) {
        message_offset = line_label.offset;
      }
    }
  }

  qsort(vector_data(&pointers), vector_count(&pointers), sizeof(LineLabel), &compare_line_label);
  qsort(vector_data(&messages), vector_count(&messages), sizeof(LineLabel), &compare_line_label);

  print_pointer_line(
    source, margin, line_number,
    vector_data(&pointers), vector_count(&pointers));
  print_message_lines(
    source, margin, line_number,
    vector_data(&messages), vector_count(&messages),
    vector_data(&pointers), vector_count(&pointers));

  vector_deinit(&pointers);
  vector_deinit(&messages);
}

static void print_body_line(const Report *report, const Source *source, unsigned long margin, unsigned long line_number)
{
  SourceLine    line;
  unsigned long offset;
  unsigned long display_offset = 0;
  source_line(source, line_number, &line);
  fprintf(stderr, "%*.s%lu │ ", (int) (margin - get_number_of_digits(line_number)), "", line_number);
  for (offset = line.offset; offset < line.offset + line.length; ++offset) {
    if (source_text(source)[offset] == '\t') {
      display_offset += fprintf(stderr, "%*.s", (int) (4 - display_offset % 4), "");
    } else {
      display_offset += fprintf(stderr, "%c", source_text(source)[offset]);
    }
  }
  fprintf(stderr, "\n");
}

static void print_tail_line(unsigned long margin)
{
  unsigned long i;
  for (i = 0; i <= margin; ++i) {
    fprintf(stderr, "─");
  }
  fprintf(stderr, "╯\n");
}

void report_emit(Report *report, const Source *source)
{
  print_header_line(report);
  if (vector_count(&report->_labels)) {
    unsigned long line_number_margin     = get_line_number_margin(report, source);
    unsigned long start_line_number      = get_start_line_number(report, source);
    unsigned long end_line_number        = get_end_line_number(report, source);
    unsigned long last_print_line_number = start_line_number;
    unsigned long i;

    print_location_line(report, source, line_number_margin);
    print_empty_body_line(line_number_margin);

    for (i = start_line_number; i <= end_line_number; ++i) {
      if (is_line_skippable(report, source, i)) {
        if (last_print_line_number + 1 == i) {
          print_skipped_body_line(report, source, line_number_margin, i);
        }
      } else {
        print_body_line(report, source, line_number_margin, i);
        print_label_lines(report, source, line_number_margin, i);
        last_print_line_number = i;
      }
    }

    print_empty_body_line(line_number_margin);
    print_tail_line(line_number_margin);
  }
  report_deinit(report);
}
