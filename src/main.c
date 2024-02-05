#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "context.h"
#include "mppl_syntax.h"
#include "source.h"

int run_compiler(const char *filename)
{
  Ctx         *ctx    = ctx_new();
  Source      *source = source_new(filename, strlen(filename));
  MpplProgram *syntax;
  int          result = EXIT_FAILURE;

  if (mpplc_parse(source, ctx, &syntax)) {
    if (mpplc_resolve(source, syntax, ctx) && mpplc_check(source, syntax, ctx)) {
      mpplc_pretty_print(syntax, NULL);
      mpplc_codegen_casl2(source, syntax, ctx);
      result = EXIT_SUCCESS;
    }
    mppl_unref(syntax);
  }

  source_free(source);
  ctx_free(ctx);
  return result;
}

int main(int argc, const char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "[Usage] %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  } else {
    int result = run_compiler(argv[1]);
    return result;
  }
}
