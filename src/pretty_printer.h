#ifndef PRETTY_PRINTER_H
#define PRETTY_PRINTER_H

#include "token.h"

typedef struct PrinterOption PrinterOption;

struct PrinterOption {
  struct {
    unsigned long foreground;
    unsigned long program;
    unsigned long keyword;
    unsigned long operator;
    unsigned long procedure;
    unsigned long parameter;
    unsigned long string;
    unsigned long literal;
  } color;
};

void mppl_pretty_print(const TokenNode *node, const PrinterOption *option);

#endif
