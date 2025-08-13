#include <assert.h>
#include <string.h>

#include "source.h"

int main(void)
{
  SourceLocation l;
  SourceRange r;

  char *const file = "./test/source.mpl";
  Source *source = source_new(file, strlen(file));
  assert(source != NULL);

  source_offset_location(source, 19, &l);
  source_line_range(source, 0, &r);
  assert(l.line == 1);
  assert(l.column == 0);
  assert(r.offset == 0);
  assert(r.length == 19);

  source_offset_location(source, 38, &l);
  source_line_range(source, 1, &r);
  assert(l.line == 1);
  assert(l.column == 19);
  assert(r.offset == 19);
  assert(r.length == 29);
}
