#include <string.h>

#include "map.h"
#include "symbol.h"
#include "utility.h"

struct SymbolContext {
  Map *symbols;
};

#define F(name, string)                                                    \
  static const Symbol SYM_DATA_##name = { -(sizeof(string) - 1), string }; \
  const Symbol       *SYM_##name      = &SYM_DATA_##name;
SYM_LIST(F)
#undef F

const Symbol *symbol(const char *string, unsigned long length, SymbolContext *context)
{
  MapIndex index;
  Symbol   key;
  key.length = length;
  key.string = string;

  if (!map_find(context->symbols, &key, &index)) {
    Symbol *symbol = xmalloc(sizeof(Symbol));
    char   *str    = xmalloc(length + 1);
    memcpy(str, string, length);
    str[length]    = '\0';
    symbol->length = length;
    symbol->string = str;
    map_update(context->symbols, &index, symbol, symbol);
  }
  return map_value(context->symbols, &index);
}

static unsigned long symbol_hash(const void *symbol)
{
  const Symbol *sym = symbol;
  return fnv1a(FNV1A_INIT, sym->string, sym->length);
}

static int symbol_equal(const void *left, const void *right)
{
  const Symbol *left_symbol  = left;
  const Symbol *right_symbol = right;

  if (left_symbol->length != right_symbol->length) {
    return left_symbol->length < right_symbol->length ? -1 : 1;
  } else {
    return !memcmp(left_symbol->string, right_symbol->string, left_symbol->length);
  }
}

SymbolContext *symbol_context_new(void)
{
  SymbolContext *context = xmalloc(sizeof(SymbolContext));
  context->symbols       = map_new(&symbol_hash, &symbol_equal);

#define F(name, string)                                                             \
  {                                                                                 \
    MapIndex index;                                                                 \
    map_find(context->symbols, (void *) SYM_##name, &index);                        \
    map_update(context->symbols, &index, (void *) SYM_##name, (void *) SYM_##name); \
  }
  SYM_LIST(F)
#undef F

  return context;
}

void symbol_context_free(SymbolContext *context)
{
  MapIndex index;
  for (map_index(context->symbols, &index); map_next(context->symbols, &index);) {
    Symbol *symbol = map_value(context->symbols, &index);
    if (symbol->length < 0) {
      free((void *) symbol->string);
    }
    free(symbol);
  }
}
