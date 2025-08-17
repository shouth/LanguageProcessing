#include <assert.h>
#include <string.h>

#include "util.h"

int main(void)
{
  size_t offsets[14] = { 0 };
  size_t lengths[14] = { 19, 29, 7, 39, 13, 12, 23, 17, 22, 14, 7, 33, 7, 0 };
  size_t i;

  {
    size_t sum = 0;
    for (i = 0; i < 14; ++i) {
      fenwick_add(offsets, 14, i + 1, lengths[i]);
    }
    for (i = 1; i <= 14; ++i) {
      sum += lengths[i - 1];
      assert(fenwick_query(offsets, 14, i) == sum);
    }
  }

  {
    size_t sum = 0;
    memcpy(offsets, lengths, sizeof(lengths));
    fenwick_build(offsets, 14);
    for (i = 1; i <= 14; ++i) {
      sum += lengths[i - 1];
      assert(fenwick_query(offsets, 14, i) == sum);
    }
  }

  {
    size_t line;
    size_t column;

    line = fenwick_upper_bound(offsets, 14, 19);
    column = 19 - fenwick_query(offsets, 14, line);
    assert(line == 1);
    assert(column == 0);

    line = fenwick_upper_bound(offsets, 14, 25);
    column = 25 - fenwick_query(offsets, 14, line);
    assert(line == 1);
    assert(column == 6);
  }

  return 0;
}
