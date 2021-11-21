#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

#include "cursol.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    uint64_t expected_terminals;
} parser_t;

#endif /* PARSER_H */
