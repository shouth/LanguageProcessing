#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "symbol.h"
#include "utility.h"
#include "vector.h"

typedef struct SymbolData    SymbolData;
typedef struct SymbolContext SymbolContext;

struct SymbolData {
  const char *string;
  long        size;
};

struct SymbolContext {
  Map    _cache;
  Vector _data;
};

#define STATIC_SYMBOL_DATA(string) \
  {                                \
    string, sizeof(string) - 1     \
  }

static const SymbolData STATIC_SYMBOLS[] = {
  STATIC_SYMBOL_DATA("and"),
  STATIC_SYMBOL_DATA("array"),
  STATIC_SYMBOL_DATA("begin"),
  STATIC_SYMBOL_DATA("boolean"),
  STATIC_SYMBOL_DATA("break"),
  STATIC_SYMBOL_DATA("call"),
  STATIC_SYMBOL_DATA("char"),
  STATIC_SYMBOL_DATA("div"),
  STATIC_SYMBOL_DATA("do"),
  STATIC_SYMBOL_DATA("else"),
  STATIC_SYMBOL_DATA("end"),
  STATIC_SYMBOL_DATA("false"),
  STATIC_SYMBOL_DATA("if"),
  STATIC_SYMBOL_DATA("integer"),
  STATIC_SYMBOL_DATA("not"),
  STATIC_SYMBOL_DATA("of"),
  STATIC_SYMBOL_DATA("or"),
  STATIC_SYMBOL_DATA("procedure"),
  STATIC_SYMBOL_DATA("program"),
  STATIC_SYMBOL_DATA("read"),
  STATIC_SYMBOL_DATA("readln"),
  STATIC_SYMBOL_DATA("return"),
  STATIC_SYMBOL_DATA("then"),
  STATIC_SYMBOL_DATA("true"),
  STATIC_SYMBOL_DATA("var"),
  STATIC_SYMBOL_DATA("while"),
  STATIC_SYMBOL_DATA("write"),
  STATIC_SYMBOL_DATA("writeln"),

  STATIC_SYMBOL_DATA("+"),
  STATIC_SYMBOL_DATA("-"),
  STATIC_SYMBOL_DATA("*"),
  STATIC_SYMBOL_DATA("="),
  STATIC_SYMBOL_DATA("<>"),
  STATIC_SYMBOL_DATA("<"),
  STATIC_SYMBOL_DATA("<="),
  STATIC_SYMBOL_DATA(">"),
  STATIC_SYMBOL_DATA(">="),
  STATIC_SYMBOL_DATA("("),
  STATIC_SYMBOL_DATA(")"),
  STATIC_SYMBOL_DATA("["),
  STATIC_SYMBOL_DATA("("),
  STATIC_SYMBOL_DATA(":="),
  STATIC_SYMBOL_DATA("."),
  STATIC_SYMBOL_DATA(","),
  STATIC_SYMBOL_DATA(":"),
  STATIC_SYMBOL_DATA(";"),
};

#undef STATIC_SYMBOL_DATA

static unsigned long symbol_hasher(const void *value)
{
  const SymbolData *data = value;
  return fnv1a(FNV1A_INIT, data->string, data->size);
}

static int symbol_comparator(const void *left, const void *right)
{
  const SymbolData *l = left;
  const SymbolData *r = right;
  return !strcmp(l->string, r->string);
}

static SymbolContext *symbol_context(void)
{
  static SymbolContext *context = NULL;
  if (!context) {
    context = xmalloc(sizeof(SymbolContext));
    map_init(&context->_cache, &symbol_hasher, &symbol_comparator);
    vector_init(&context->_data);

    {
      Symbol symbol;
      for (symbol = 0; symbol < SYMBOL_SENTINEL; ++symbol) {
        map_update(&context->_cache, (void *) (STATIC_SYMBOLS + symbol), (void *) symbol);
        vector_push_back(&context->_data, (void *) (STATIC_SYMBOLS + symbol));
      }
    }
  }
  return context;
}

Symbol symbol_from(const char *string, unsigned long size)
{
  char *copy = xmalloc(sizeof(char) * (size + 1));
  strncpy(copy, string, size);
  copy[size] = '\0';
  return symbol_take_from(copy, size);
}

Symbol symbol_take_from(char *string, unsigned long size)
{
  MapEntry       entry;
  SymbolContext *context = symbol_context();
  SymbolData    *data    = xmalloc(sizeof(SymbolData));
  data->string           = string;
  data->size             = size;

  if (map_entry(&context->_cache, data, &entry)) {
    free((void *) data->string);
    free(data);
  } else {
    Symbol symbol = vector_size(&context->_data);
    map_entry_update(&entry, (void *) symbol);
    vector_push_back(&context->_data, data);
  }
  return (Symbol) map_entry_value(&entry);
}

static const SymbolData *symbol_data(Symbol symbol)
{
  const SymbolContext *context = symbol_context();
  return vector_data(&context->_data)[symbol];
}

const char *symbol_string(Symbol symbol)
{
  return symbol_data(symbol)->string;
}

unsigned long symbol_size(Symbol symbol)
{
  return symbol_data(symbol)->size;
}
