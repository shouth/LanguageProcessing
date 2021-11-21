#ifndef PARSER_H
#define PARSER_H

#include "rule_stream.h"
#include "source.h"

rule_stream_t *parse(const source_t *src);

#endif /* PARSER_H */
