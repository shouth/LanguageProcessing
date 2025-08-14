#include <assert.h>
#include <string.h>

#include "util.h"

int main(void)
{
  size_t offsets[14] = { 0 };
  size_t lengths[14] = { 19, 29, 7, 39, 13, 12, 23, 17, 22, 14, 7, 33, 7, 0 };
  size_t i;

  for (i = 0; i < 14; ++i) {
    text_offsets_update(offsets, 14, i, lengths[i]);
  }

  {
    size_t sum = 0;
    for (i = 1; i < 14; ++i) {
      sum += lengths[i - 1];
      assert(text_offsets_at(offsets, 14, i) == sum);
    }
  }

  {
    size_t line;
    size_t column;

    line = text_offsets_locate(offsets, 14, 19, &column);
    assert(line == 1);
    assert(column == 0);

    line = text_offsets_locate(offsets, 14, 25, &column);
    assert(line == 1);
    assert(column == 6);
  }
}
