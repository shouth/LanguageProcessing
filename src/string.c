#include <string.h>

#include "map.h"
#include "string.h"
#include "utility.h"

struct String {
  long        length;
  const char *data;
};

struct StringContext {
  Map *strings;
};

const String *string_from(const char *string, unsigned long length, StringContext *context)
{
  MapIndex index;
  String   key;
  key.length = length;
  key.data   = string;

  if (!map_find(context->strings, &key, &index)) {
    String *string = xmalloc(sizeof(String));
    char   *str    = xmalloc(length + 1);
    memcpy(str, string, length);
    str[length]    = '\0';
    string->length = length;
    string->data   = str;
    map_update(context->strings, &index, string, string);
  }
  return map_value(context->strings, &index);
}

const char *string_data(const String *string)
{
  return string->data;
}

unsigned long string_length(const String *string)
{
  return string->length;
}

static unsigned long string_hash(const void *string)
{
  const String *sym = string;
  return fnv1a(FNV1A_INIT, sym->data, sym->length);
}

static int string_equal(const void *left, const void *right)
{
  const String *left_string  = left;
  const String *right_string = right;

  if (left_string->length != right_string->length) {
    return left_string->length < right_string->length ? -1 : 1;
  } else {
    return !memcmp(left_string->data, right_string->data, left_string->length);
  }
}

StringContext *string_context_new(void)
{
  StringContext *context = xmalloc(sizeof(StringContext));
  context->strings       = map_new(&string_hash, &string_equal);
  return context;
}

void string_context_free(StringContext *context)
{
  MapIndex index;
  for (map_index(context->strings, &index); map_next(context->strings, &index);) {
    String *string = map_value(context->strings, &index);
    free((char *) string->data);
    free(string);
  }
}
