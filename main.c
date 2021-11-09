#include "parser.h"

int cb_prettyprint(const parser_t *pa, va_list va)
{
    parser_event_t ev = va_arg(va, parser_event_t);
    int symbol = va_arg(va, int);

    static int last_terminal;
    static int newline = 0;
    static int indent_level = 0;

    if (ev == PARSE_SUCCESS) {
        if (symbol <= NUMOFTOKEN) {
            if (newline || symbol == TEND || symbol == TELSE) {
                if (!newline) {
                    printf("\n");
                }
                newline = 0;
                if (last_terminal == TBEGIN) {
                    indent_level++;
                }
                if (symbol == TEND) {
                    indent_level--;
                }
                printf("%*.s", indent_level * 4, "");
            }

            switch (symbol) {
            case TTHEN:
            case TDO:
                indent_level++;
                break;
            }

            /* infix operators */
            switch (symbol) {
            case TOF:
            case TOR:
            case TDIV:
            case TAND:
            case TPLUS:
            case TMINUS:
            case TSTAR:
            case TEQUAL:
            case TNOTEQ:
            case TLE:
            case TLEEQ:
            case TGR:
            case TGREQ:
            case TASSIGN:
            case TCOLON:
                printf(" %s ", pa->lexer.buf);
                break;
            case TPROGRAM:
            case TVAR:
            case TPROCEDURE:
            case TIF:
            case TWHILE:
            case TNOT:
            case TCOMMA:
                printf("%s ", pa->lexer.buf);
                break;
            case TDO:
            case TTHEN:
                printf(" %s", pa->lexer.buf);
                break;
            default:
                printf("%s", pa->lexer.buf);
                break;
            }

            switch (symbol) {
            case TSEMI:
            case TTHEN:
            case TELSE:
            case TDO:
            case TBEGIN:
                printf("\n");
                newline = 1;
                break;
            default:
                break;
            }

            last_terminal = symbol;
        }
    }
}

int main(int nc, char *np[])
{
    parser_t parser;
    int ret;
    parser_init(&parser, np[1], cb_prettyprint);

    ret = mppl_rule_root(&parser);
    printf("%d", ret);
}
