#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct String        String;
typedef struct StringContext StringContext;

const String  *string_from(const char *string, unsigned long length, StringContext *context);
const char    *string_data(const String *string);
unsigned long  string_length(const String *string);
StringContext *string_context_new(void);
void           string_context_free(StringContext *context);

#endif
