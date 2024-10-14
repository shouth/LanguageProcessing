/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>

#include "diag.h"
#include "mppl_syntax.h"
#include "mppl_ty_ctxt.h"
#include "report.h"
#include "util.h"

/* utility */

char *expected_set_to_string(const MpplTokenKindSet *expected)
{
  MpplSyntaxKind kind;
  unsigned long  count;
  unsigned long  length;
  Slice(char) result;

  FILE *buffer = tmpfile();
  if (buffer == NULL) {
    return NULL;
  }

  count  = bitset_count(expected);
  length = 0;
  if (count > 2) {
    length += fprintf(buffer, "one of ");
  }

  for (kind = MPPL_BEGIN_TOKEN; kind < MPPL_END_TOKEN; kind++) {
    if (bitset_get(expected, kind)) {
      const char *lexeme = mppl_syntax_kind_static_lexeme(kind - MPPL_BEGIN_TOKEN);

      if (lexeme) {
        length += fprintf(buffer, "`%s`", lexeme);
      } else {
        switch (kind) {
        case MPPL_SYNTAX_EOF_TOKEN:
          length += fprintf(buffer, "end of file");
          break;
        case MPPL_SYNTAX_INTEGER_LIT:
          length += fprintf(buffer, "number");
          break;
        case MPPL_SYNTAX_STRING_LIT:
          length += fprintf(buffer, "string");
          break;
        case MPPL_SYNTAX_IDENT_TOKEN:
          length += fprintf(buffer, "identifier");
          break;

        default:
          unreachable();
        }
      }

      if (count > 2) {
        length += fprintf(buffer, ", ");
      } else if (count == 2) {
        length += fprintf(buffer, " or ");
      }
      --count;
    }
  }

  slice_alloc(&result, length + 1);
  rewind(buffer);
  fread(result.ptr, 1, length, buffer);
  result.ptr[length] = '\0';
  fclose(buffer);
  return result.ptr;
}

unsigned long print_mppl_ty(FILE *buffer, const MpplTy *ty)
{
  unsigned long length = 0;
  switch (ty->kind) {
  case MPPL_TY_INTEGER:
    length += fprintf(buffer, "integer");
    break;

  case MPPL_TY_CHAR:
    length += fprintf(buffer, "char");
    break;

  case MPPL_TY_BOOLEAN:
    length += fprintf(buffer, "boolean");
    break;

  case MPPL_TY_STRING:
    length += fprintf(buffer, "string");
    break;

  case MPPL_TY_ARRAY: {
    const MpplArrayTy *array_ty = (const MpplArrayTy *) ty;

    length += print_mppl_ty(buffer, array_ty->base);
    length += fprintf(buffer, "[%lu]", array_ty->size);
    break;
  }

  case MPPL_TY_PROC: {
    const MpplProcTy *proc_ty = (const MpplProcTy *) ty;

    unsigned long i;
    length += fprintf(buffer, "procedure(");
    for (i = 0; i < proc_ty->params.count; i++) {
      if (i > 0) {
        length += fprintf(buffer, ", ");
      }
      length += print_mppl_ty(buffer, proc_ty->params.ptr[i]);
    }
    length += fprintf(buffer, ")");
    break;
  }

  default:
    unreachable();
  }

  return length;
}

char *mppl_ty_to_string(const MpplTy *ty)
{
  unsigned long length;
  Slice(char) result;

  FILE *buffer = tmpfile();
  if (buffer == NULL) {
    return NULL;
  }

  length = print_mppl_ty(buffer, ty);
  slice_alloc(&result, length + 1);
  rewind(buffer);
  fread(result.ptr, 1, length, buffer);
  result.ptr[length] = '\0';
  fclose(buffer);
  return result.ptr;
}

/* lexer */

Report *diag_stray_char_error(unsigned long offset, int stray, MpplTokenKindSet expected)
{
  char   *str = expected_set_to_string(&expected);
  Report *report;
  if (is_graphic(stray)) {
    report = report_new(REPORT_KIND_ERROR, offset, "stray `%c` in program", (char) stray);
  } else {
    report = report_new(REPORT_KIND_ERROR, offset, "stray `\\x%X` in program", (unsigned char) stray);
  }
  report_annotation(report, offset, offset + 1, "expected %s", str);
  free(str);
  return report;
}

Report *diag_nongraphic_char_error(unsigned long offset, int nongraphic)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "non-graphic character `\\x%X` in string", (unsigned char) nongraphic);
  report_annotation(report, offset, offset + 1, NULL);
  return report;
}

Report *diag_unterminated_string_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "string is unterminated");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_unterminated_comment_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "comment is unterminated");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_too_big_number_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "number is too big");
  report_annotation(report, offset, offset + length, "numbers need to be less than 32768");
  return report;
}

/* parser */

Report *diag_unexpected_token_error(unsigned long offset, unsigned long length, const char *found, MpplTokenKindSet expected)
{
  char   *str = expected_set_to_string(&expected);
  Report *report;
  if (found) {
    report = report_new(REPORT_KIND_ERROR, offset, "expected %s, found `%.*s`", str, (int) length, found);
    report_annotation(report, offset, offset + length, NULL);
  } else {
    report = report_new(REPORT_KIND_ERROR, offset, "expected %s, found end of file", str);
    report_annotation(report, offset, offset + 1, NULL);
  }
  free(str);
  return report;
}

Report *diag_expected_expression_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "expected expression");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_missing_semicolon_error(unsigned long offset)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "missing semicolon");
  report_annotation(report, offset, offset + 1, NULL);
  return report;
}

Report *diag_break_outside_loop_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`break` statement outside loop");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

/* resolver */

Report *diag_multiple_definition_error(unsigned long offset, unsigned long length, const char *name, unsigned long previous_offset)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%.*s` has multiple definitions", (int) length, name);
  report_annotation(report, offset, offset + length, "current definition");
  report_annotation(report, previous_offset, previous_offset + length, "previous definition");
  return report;
}

Report *diag_not_defined_error(unsigned long offset, unsigned long length, const char *name)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%.*s` is not defined", (int) length, name);
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

/* checker */ /* TODO: provide more precise error messages */

Report *diag_zero_sized_array_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "zero-sized array");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_non_array_subscript_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "non-array subscript");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_recursive_call_error(unsigned long offset, unsigned long length, const char *name)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "recursive call to `%s`", name);
  report_annotation(report, offset, offset + length, "recursive call is not allowed");
  return report;
}

Report *diag_mismatched_type_error(unsigned long offset, unsigned long length, const MpplTy *expected, const MpplTy *found)
{
  char   *expected_str = mppl_ty_to_string(expected);
  char   *found_str    = mppl_ty_to_string(found);
  Report *report       = report_new(REPORT_KIND_ERROR, offset, "mismatched type");
  report_annotation(report, offset, offset + length, "expected `%s`, found `%s`", expected_str, found_str);
  free(expected_str);
  free(found_str);
  return report;
}

Report *diag_non_standard_type_error(unsigned long offset, unsigned long length, const MpplTy *found)
{
  char   *found_str = mppl_ty_to_string(found);
  Report *report    = report_new(REPORT_KIND_ERROR, offset, "mismatched type");
  report_annotation(report, offset, offset + length, "expected `integer`, `char` or `boolean`, found `%s`", found_str);
  free(found_str);
  return report;
}

Report *diag_non_lvalue_assignment_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "tries to assign to a rvalue");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_mismatched_arguments_count_error(unsigned long offset, unsigned long length, unsigned long expected, unsigned long found)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "mismatched arguments count");
  report_annotation(report, offset, offset + length, "expected %lu, found %lu", expected, found);
  return report;
}

Report *diag_non_procedure_invocation_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "tries to invoke a non-procedure");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_invalid_input_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid input");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_invalid_output_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid output");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}
