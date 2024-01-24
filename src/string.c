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

  if (!map_entry(context->strings, &key, &index)) {
    String *instance = xmalloc(sizeof(String));
    char   *str      = xmalloc(length + 1);
    memcpy(str, string, length);
    str[length]      = '\0';
    instance->length = length;
    instance->data   = str;
    map_update(context->strings, &index, instance, instance);
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
  const String *l = left;
  const String *r = right;

  return l->length == r->length && !memcmp(l->data, r->data, l->length);
}

StringContext *string_context_new(void)
{
  StringContext *context = xmalloc(sizeof(StringContext));
  context->strings       = map_new(&string_hash, &string_equal);
  return context;
}

void string_context_free(StringContext *context)
{
  if (context) {
    MapIndex index;
    for (map_iterator(context->strings, &index); map_next(context->strings, &index);) {
      String *string = map_value(context->strings, &index);
      free((char *) string->data);
      free(string);
    }
    map_free(context->strings);
    free(context);
  }
}
