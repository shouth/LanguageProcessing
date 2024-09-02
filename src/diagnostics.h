#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "mppl_syntax.h"
#include "report.h"

typedef struct Diag Diag;

void    diag_free(Diag *diag);
Report *diag_to_report(const Diag *diag);

/* lexer */

Diag *diag_stray_char_error(unsigned long offset, int stray, MpplSyntaxKindSet expected);
Diag *diag_nongraphic_char_error(unsigned long offset, int nongraphic);
Diag *diag_unterminated_string_error(unsigned long offset, unsigned long length);
Diag *diag_unterminated_comment_error(unsigned long offset, unsigned long length);
Diag *diag_too_big_number_error(unsigned long offset, unsigned long length);

/* parser */

Diag *diag_unexpected_token_error(unsigned long offset, unsigned long length, char *found, MpplSyntaxKindSet expected);
Diag *diag_expected_expression_error(unsigned long offset, unsigned long length);
Diag *diag_missing_semicolon_error(unsigned long offset);
Diag *diag_break_outside_loop_error(unsigned long offset, unsigned long length);

#endif /* DIAGNOSTICS_H */
