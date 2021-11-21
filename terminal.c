#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "terminal.h"
#include "token.h"

typedef struct {
    terminal_type_t terminal;
    const char *string;
} keyword_map_t;

const keyword_map_t keyword_map[] = {
    { TERMINAL_PROGRAM,   "program"   },
    { TERMINAL_VAR,       "var"       },
    { TERMINAL_ARRAY,     "array"     },
    { TERMINAL_OF,        "of"        },
    { TERMINAL_BEGIN,     "begin"     },
    { TERMINAL_END,       "end"       },
    { TERMINAL_IF,        "if"        },
    { TERMINAL_THEN,      "then"      },
    { TERMINAL_ELSE,      "else"      },
    { TERMINAL_PROCEDURE, "procedure" },
    { TERMINAL_RETURN,    "return"    },
    { TERMINAL_CALL,      "call"      },
    { TERMINAL_WHILE,     "while"     },
    { TERMINAL_DO,        "do"        },
    { TERMINAL_NOT,       "not"       },
    { TERMINAL_OR,        "or"        },
    { TERMINAL_DIV,       "div"       },
    { TERMINAL_AND,       "and"       },
    { TERMINAL_CHAR,      "char"      },
    { TERMINAL_INTEGER,   "integer"   },
    { TERMINAL_BOOLEAN,   "boolean"   },
    { TERMINAL_READ,      "read"      },
    { TERMINAL_WRITE,     "write"     },
    { TERMINAL_READLN,    "readln"    },
    { TERMINAL_WRITELN,   "writeln"   },
    { TERMINAL_TRUE,      "true"      },
    { TERMINAL_FALSE,     "false"     },
    { TERMINAL_BREAK,     "break"     },
};

const size_t keyword_map_size = sizeof(keyword_map) / sizeof(keyword_map_t);

void terminal_from_token(const token_t *token, terminal_t *terminal)
{
    size_t i;
    assert(token != NULL && terminal != NULL);

    terminal->ptr = token->ptr;
    terminal->len = token->len;

    switch (token->data.type) {
    case TOKEN_NAME_OR_KEYWORD:
        for (i = 0; i < keyword_map_size; i++) {
            if (strcmp(token->ptr, keyword_map[i].string) == 0) {
                terminal->data.type = keyword_map[i].terminal;
                return;
            }
        }

        terminal->data.type = TERMINAL_NAME;
        return;

    case TOKEN_NUMBER:
        terminal->data.number.value = strtol(token->ptr, NULL, 10);
        if (errno == ERANGE || terminal->data.number.value > 32767) {
            /* error */
        }

        terminal->data.type = TERMINAL_NUMBER;
        return;

    case TOKEN_STRING:
        if (!token->data.string.terminated) {
            /* error */
        }

        terminal->data.type = TERMINAL_STRING;
        terminal->data.string.ptr = terminal->ptr + 1;
        terminal->data.string.len = terminal->len - 2;
        return;

    case TOKEN_BRACES_COMMENT:
        if (!token->data.braces_comment.terminated) {
            /* error */
        }

        terminal->data.type = TERMINAL_NONE;
        return;

    case TOKEN_CSTYLE_COMMENT:
        if (!token->data.cstyle_comment.terminated) {
            /* error */
        }

        terminal->data.type = TERMINAL_NONE;
        return;

    case TOKEN_WHITESPACE:
        terminal->data.type = TERMINAL_NONE;
        return;

    case TOKEN_PLUS:
        terminal->data.type = TERMINAL_PLUS;
        return;

    case TOKEN_MINUS:
        terminal->data.type = TERMINAL_MINUS;
        return;

    case TOKEN_STAR:
        terminal->data.type = TERMINAL_STAR;
        return;

    case TOKEN_EQUAL:
        terminal->data.type = TERMINAL_EQUAL;
        return;

    case TOKEN_NOTEQ:
        terminal->data.type = TERMINAL_NOTEQ;
        return;

    case TOKEN_LE:
        terminal->data.type = TERMINAL_LE;
        return;

    case TOKEN_LEEQ:
        terminal->data.type = TERMINAL_LEEQ;
        return;

    case TOKEN_GR:
        terminal->data.type = TERMINAL_GR;
        return;

    case TOKEN_GREQ:
        terminal->data.type = TERMINAL_GREQ;
        return;

    case TOKEN_LPAREN:
        terminal->data.type = TERMINAL_LPAREN;
        return;

    case TOKEN_RPAREN:
        terminal->data.type = TERMINAL_RPAREN;
        return;

    case TOKEN_LSQPAREN:
        terminal->data.type = TERMINAL_LSQPAREN;
        return;

    case TOKEN_RSQPAREN:
        terminal->data.type = TERMINAL_RSQPAREN;
        return;

    case TOKEN_ASSIGN:
        terminal->data.type = TERMINAL_ASSIGN;
        return;

    case TOKEN_DOT:
        terminal->data.type = TERMINAL_DOT;
        return;

    case TOKEN_COMMA:
        terminal->data.type = TERMINAL_COMMA;
        return;

    case TOKEN_COLON:
        terminal->data.type = TERMINAL_COLON;
        return;

    case TOKEN_SEMI:
        terminal->data.type = TERMINAL_SEMI;
        return;

    case TOKEN_EOF:
        terminal->data.type = TERMINAL_EOF;
        return;

    case TOKEN_UNKNOWN:
        /* error */
    }
}
