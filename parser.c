#include <stdint.h>

#include "cursol.h"
#include "parser.h"
#include "rule_stream.h"

typedef struct {
    const source_t *src;
    cursol_t cursol;

    uint64_t expected_terminals;
} parser_t;

rule_stream_t *parse(const source_t *src)
{

}
