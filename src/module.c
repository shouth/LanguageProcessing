#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"
#include "parser.h"
#include "token_cursor.h"
#include "utility.h"

void module_init(Module *module, const char *filename)
{
  module->_filename_size = strlen(filename);
  module->_filename      = xmalloc(sizeof(char) * (module->_filename_size + 1));
  strcpy(module->_filename, filename);

  module->_source      = NULL;
  module->_source_size = -1ul;
}

void module_deinit(Module *module)
{
  free(module->_filename);
  free(module->_source);
}

const char *module_source(Module *module)
{
  if (!module->_source) {
    unsigned long capacity = 1 << 6;
    FILE         *file     = fopen(module->_filename, "rb");

    if (file) {
      module->_source_size = 0;
      while (1) {
        char *next = realloc(module->_source, capacity);
        if (!next) {
          free(module->_source);
          module->_source_size = -1ul;
          break;
        }

        module->_source = next;
        module->_source_size += fread(module->_source + module->_source_size, sizeof(char), capacity - module->_source_size, file);
        if (module->_source_size < capacity) {
          module->_source[module->_source_size] = '\0';
          break;
        }

        capacity <<= 1;
      }
      fclose(file);
    }
  }
  return module->_source;
}

unsigned long module_source_size(Module *module)
{
  return module_source(module) ? module->_source_size : -1ul;
}

int module_token_cursor(Module *module, TokenCursor *cursor)
{
  int result = !!module_source(module);
  if (result) {
    token_cursor_init(cursor, module_source(module), module_source_size(module));
  }
  return result;
}

int module_token_tree(Module *module, TokenTree *tree)
{
  int result = !!module_source(module);
  if (result) {
    mppl_parse(module_source(module), module_source_size(module), tree);
  }
  return result;
}
