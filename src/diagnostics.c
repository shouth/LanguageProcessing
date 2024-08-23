#include <string.h>

#include "diagnostics.h"
#include "mppl_syntax.h"
#include "report.h"
#include "source.h"
#include "stdio.h"
#include "utility.h"

struct Diag {
  Report *(*to_report)(const Diag *diagnostics, const Source *source);
};

Report *diag_to_report(const Diag *diagnostics, const Source *source)
{
  return diagnostics->to_report(diagnostics, source);
}

/* utility */

char *expected_set_to_string(const MpplSyntaxKindSet *expected)
{
  MpplSyntaxKind kind;
  unsigned long  count;
  unsigned long  length;
  char          *result;

  FILE *buffer = tmpfile();
  if (buffer == NULL) {
    return NULL;
  }

  count  = bitset_count(expected);
  length = 0;
  if (count > 1) {
    length += fprintf(buffer, "one of ");
  }

  for (kind = 0; kind < SENTINEL_MPPL_SYNTAX; kind++) {
    if (bitset_get(expected, kind)) {
      length += fprintf(buffer, "%s", mppl_syntax_kind_to_string(kind));
      if (count > 2) {
        length += fprintf(buffer, ", ");
      } else if (count == 2) {
        length += fprintf(buffer, " and ");
      }
      --count;
    }
  }

  result = xmalloc(length + 1);
  rewind(buffer);
  fread(result, 1, length, buffer);
  result[length] = '\0';
  fclose(buffer);
  return result;
}

/* lexer */

typedef struct StrayCharError StrayCharError;

struct StrayCharError {
  Diag              interface;
  unsigned long     offset;
  MpplSyntaxKindSet expected;
};

static Report *report_stray_char_error(const Diag *diag, const Source *source)
{
  const StrayCharError *e = (const StrayCharError *) diag;

  char    stray = source->text[e->offset];
  Report *report;
  if (is_graphic(stray)) {
    report = report_new(REPORT_KIND_ERROR, e->offset, "stray `%c` in program", stray);
  } else {
    report = report_new(REPORT_KIND_ERROR, e->offset, "stray `\\x%X` in program", (unsigned char) stray);
  }
  /* TODO: replace NULL with stringified syntax kind set */
  report_annotation(report, e->offset, e->offset + 1, NULL);
  return report;
}

Diag *diag_stray_char_error(unsigned long offset, MpplSyntaxKindSet *expected)
{
  StrayCharError *e      = xmalloc(sizeof(StrayCharError));
  e->interface.to_report = &report_stray_char_error;
  e->offset              = offset;
  e->expected            = *expected;
  return (Diag *) e;
}

typedef struct NonGraphicCharError NonGraphicCharError;

struct NonGraphicCharError {
  Diag          interface;
  unsigned long offset;
};

static Report *report_nongraphic_char_error(const Diag *diag, const Source *source)
{
  const NonGraphicCharError *e = (const NonGraphicCharError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "non-graphic character `\\x%X` in string", (unsigned char) source->text[e->offset]);
  report_annotation(report, e->offset, e->offset + 1, NULL);
  return report;
}

Diag *diag_nongraphic_char_error(unsigned long offset)
{
  NonGraphicCharError *e = xmalloc(sizeof(NonGraphicCharError));
  e->interface.to_report = &report_nongraphic_char_error;
  e->offset              = offset;
  return (Diag *) e;
}

typedef struct UnterminatedStringError UnterminatedStringError;

struct UnterminatedStringError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_unterminated_string(const Diag *diag, const Source *source)
{
  const UnterminatedStringError *e = (const UnterminatedStringError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "string is unterminated");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_unterminated_string_error(unsigned long offset, unsigned long length)
{
  UnterminatedStringError *e = xmalloc(sizeof(UnterminatedStringError));
  e->interface.to_report     = &report_unterminated_string;
  e->offset                  = offset;
  e->length                  = length;
  return (Diag *) e;
}

typedef struct UnterminatedCommentError UnterminatedCommentError;

struct UnterminatedCommentError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_unterminated_comment_error(const Diag *diagnostics, const Source *source)
{
  const UnterminatedCommentError *e = (const UnterminatedCommentError *) diagnostics;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "comment is unterminated");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_unterminated_comment_error(unsigned long offset, unsigned long length)
{
  UnterminatedCommentError *e = xmalloc(sizeof(UnterminatedCommentError));
  e->interface.to_report      = &report_unterminated_comment_error;
  e->offset                   = offset;
  e->length                   = length;
  return (Diag *) e;
}

typedef struct TooBigNumberError TooBigNumberError;

struct TooBigNumberError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_too_big_number_error(const Diag *diag, const Source *source)
{
  const TooBigNumberError *e = (const TooBigNumberError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "number is too big");
  report_annotation(report, e->offset, e->offset + e->length, "numbers need to be less than 32768");
  return report;
}

Diag *diag_too_big_number_error(unsigned long offset, unsigned long length)
{
  TooBigNumberError *e   = xmalloc(sizeof(TooBigNumberError));
  e->interface.to_report = &report_too_big_number_error;
  e->offset              = offset;
  e->length              = length;
  return (Diag *) e;
}

/* parser */

typedef struct UnexpectedTokenError UnexpectedTokenError;

struct UnexpectedTokenError {
  Diag              interface;
  unsigned long     offset;
  unsigned long     length;
  MpplSyntaxKindSet expected;
};

static Report *report_diag_unexpected_token_error(const Diag *diag, const Source *source)
{
  const UnexpectedTokenError *e = (const UnexpectedTokenError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "expected %s, found `%.*s`",
    expected_set_to_string(&e->expected), (int) e->length, source->text + e->offset);
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_unexpected_token_error(unsigned long offset, unsigned long length, MpplSyntaxKindSet *expected)
{
  UnexpectedTokenError *e = xmalloc(sizeof(UnexpectedTokenError));
  e->interface.to_report  = &report_diag_unexpected_token_error;
  e->offset               = offset;
  e->length               = length;
  e->expected             = *expected;
  return (Diag *) e;
}

typedef struct MissingSemicolonError MissingSemicolonError;

struct MissingSemicolonError {
  Diag          interface;
  unsigned long offset;
};

static Report *report_missing_semicolon_error(const Diag *diag, const Source *source)
{
  const MissingSemicolonError *e = (const MissingSemicolonError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "missing semicolon");
  report_annotation(report, e->offset, e->offset + 1, NULL);
  return report;
}

Diag *diag_missing_semicolon_error(unsigned long offset)
{
  MissingSemicolonError *e = xmalloc(sizeof(MissingSemicolonError));
  e->interface.to_report   = &report_missing_semicolon_error;
  e->offset                = offset;
  return (Diag *) e;
}

typedef struct BreakOutsideLoopError BreakOutsideLoopError;

struct BreakOutsideLoopError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_break_outside_loop_error(const Diag *diag, const Source *source)
{
  const BreakOutsideLoopError *e = (const BreakOutsideLoopError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "`break` statement outside loop");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_break_outside_loop_error(unsigned long offset, unsigned long length)
{
  BreakOutsideLoopError *e = xmalloc(sizeof(BreakOutsideLoopError));
  e->interface.to_report   = &report_break_outside_loop_error;
  e->offset                = offset;
  e->length                = length;
  return (Diag *) e;
}
