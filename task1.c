#include "cursol.h"
#include "lexer.h"
#include "source.h"
#include "token.h"
#include "terminal.h"

int main(int argc, char **argv)
{
    source_t *src;
    cursol_t cursol;
    token_t token;
    terminal_t terminal;

    if (argc < 2) {
        return -1;
    }

    src = source_new(argv[1]);
    cursol_init(&cursol, src, src->src_ptr, src->src_size);

    while (1) {
        lex(&cursol, &token);
        terminal_from_token(&terminal, &token);

        if (terminal.data.type == TERMINAL_EOF) {
            printf("reached EOF");
            break;
        }
    }
}
