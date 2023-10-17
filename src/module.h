#ifndef MODULE_H
#define MODULE_H

#include "lexer.h"

typedef struct Module Module;

struct Module {
  char         *_filename;
  unsigned long _filename_size;
  char         *_source;
  unsigned long _source_size;
};

void          module_init(Module *module, const char *filename);
void          module_deinit(Module *module);
const char   *module_source(Module *module);
unsigned long module_source_size(Module *module);
int           module_lexer(Module *module, Lexer *lexer);

#endif
