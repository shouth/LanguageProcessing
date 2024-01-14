#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "pretty_printer.h"
#include "session.h"

int run_compiler(Session *session)
{
  if (session_check(session)) {
    mppl_pretty_print(session_parse(session), NULL);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int main(int argc, const char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "[Usage] %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  } else {
    Session *session = session_new(argv[1]);
    int      result  = run_compiler(session);
    session_free(session);
    return result;
  }
}
