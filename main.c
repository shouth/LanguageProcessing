#include <stdlib.h>
#include <stdio.h>

#include "source.h"
#include "parser.h"

int main(int argc, char **argv)
{
    source_t *src;

    if (argc < 2) {
        return -1;
    }

    src = source_new(argv[1]);
    parse(src);
    return 0;
}
