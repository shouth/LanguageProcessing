#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "message.h"
#include "terminal.h"
#include "token.h"
#include "util.h"

const struct {
    terminal_type_t terminal;
    const char *string;
} keyword_map[] = {
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

const size_t keyword_map_size = sizeof(keyword_map) / sizeof(*keyword_map);

/*
 * Instead of using strncmp, I implemented comparator function
 * because token_t's string is not null-terminated.
 */
int is_token_keyword(const token_t *token, const char *keyword)
{
    size_t i;
    for (i = 0; i < token->len; i++) {
        if (token->ptr[i] != keyword[i]) {
            return 0;
        }
    }
    return keyword[i] == '\0';
}

void terminal_from_token(terminal_t *terminal, const token_t *token)
{
    size_t i;
    msg_t *msg;
    assert(token != NULL && terminal != NULL);

    terminal->ptr = token->ptr;
    terminal->len = token->len;
    terminal->src = token->src;
    terminal->pos = token->pos;

    switch (token->data.type) {
    case TOKEN_NAME_OR_KEYWORD:
        for (i = 0; i < keyword_map_size; i++) {
            if (is_token_keyword(token, keyword_map[i].string)) {
                terminal->data.type = keyword_map[i].terminal;
                return;
            }
        }

        terminal->data.type = TERMINAL_NAME;
        return;

    case TOKEN_NUMBER:
        terminal->data.data.number.value = strtoul(token->ptr, NULL, 10);
        if (errno == ERANGE || terminal->data.data.number.value > 32767) {
            msg = msg_new(token->src, token->pos, token->len, MSG_ERROR, "number is too large");
            msg_add_inline_entry(msg, token->pos, token->len, "number needs to be less than 32768");
            msg_emit(msg);
            exit(1);
        }

        terminal->data.type = TERMINAL_NUMBER;
        return;

    case TOKEN_STRING:
        if (!token->data.data.string.terminated) {
            msg = msg_new(token->src, token->pos, token->len, MSG_ERROR, "string is unterminated");
            msg_emit(msg);
            exit(1);
        }

        terminal->data.type = TERMINAL_STRING;
        terminal->data.data.string.ptr = terminal->ptr + 1;
        terminal->data.data.string.len = terminal->len - 2;
        return;

    case TOKEN_BRACES_COMMENT:
        if (!token->data.data.braces_comment.terminated) {
            msg = msg_new(token->src, token->pos, 1, MSG_ERROR, "comment is unterminated");
            msg_emit(msg);
            exit(1);
        }

        terminal->data.type = TERMINAL_NONE;
        return;

    case TOKEN_CSTYLE_COMMENT:
        if (!token->data.data.cstyle_comment.terminated) {
            msg = msg_new(token->src, token->pos, 2, MSG_ERROR, "comment is unterminated");
            msg_emit(msg);
            exit(1);
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
        if (is_graphical(token->ptr[0])) {
            msg = msg_new(token->src, token->pos, token->len, MSG_ERROR, "stray `%c` in program", token->ptr[0]);
        } else {
            msg = msg_new(token->src, token->pos, token->len, MSG_ERROR, "stray \\%03o in program", (unsigned char) token->ptr[0]);
        }
        msg_emit(msg);
        exit(1);
    }
}

const char *terminal_to_str(terminal_type_t type)
{
    switch (type) {
    case TERMINAL_NAME:
        return "NAME";
    case TERMINAL_PROGRAM:
        return "program";
    case TERMINAL_VAR:
        return "var";
    case TERMINAL_ARRAY:
        return "array";
    case TERMINAL_OF:
        return "of";
    case TERMINAL_BEGIN:
        return "begin";
    case TERMINAL_END:
        return "end";
    case TERMINAL_IF:
        return "if";
    case TERMINAL_THEN:
        return "then";
    case TERMINAL_ELSE:
        return "else";
    case TERMINAL_PROCEDURE:
        return "procedure";
    case TERMINAL_RETURN:
        return "return";
    case TERMINAL_CALL:
        return "call";
    case TERMINAL_WHILE:
        return "while";
    case TERMINAL_DO:
        return "do";
    case TERMINAL_NOT:
        return "not";
    case TERMINAL_OR:
        return "or";
    case TERMINAL_DIV:
        return "div";
    case TERMINAL_AND:
        return "and";
    case TERMINAL_CHAR:
        return "char";
    case TERMINAL_INTEGER:
        return "integer";
    case TERMINAL_BOOLEAN:
        return "boolean";
    case TERMINAL_READLN:
        return "readln";
    case TERMINAL_WRITELN:
        return "writeln";
    case TERMINAL_TRUE:
        return "true";
    case TERMINAL_FALSE:
        return "false";
    case TERMINAL_NUMBER:
        return "number";
    case TERMINAL_STRING:
        return "STRING";
    case TERMINAL_PLUS:
        return "+";
    case TERMINAL_MINUS:
        return "-";
    case TERMINAL_STAR:
        return "*";
    case TERMINAL_EQUAL:
        return "=";
    case TERMINAL_NOTEQ:
        return "<>";
    case TERMINAL_LE:
        return "<";
    case TERMINAL_LEEQ:
        return "<=";
    case TERMINAL_GR:
        return ">";
    case TERMINAL_GREQ:
        return ">=";
    case TERMINAL_LPAREN:
        return "(";
    case TERMINAL_RPAREN:
        return ")";
    case TERMINAL_LSQPAREN:
        return "[";
    case TERMINAL_RSQPAREN:
        return "]";
    case TERMINAL_ASSIGN:
        return ":=";
    case TERMINAL_DOT:
        return ".";
    case TERMINAL_COMMA:
        return ",";
    case TERMINAL_COLON:
        return ":";
    case TERMINAL_SEMI:
        return ";";
    case TERMINAL_READ:
        return "read";
    case TERMINAL_WRITE:
        return "write";
    case TERMINAL_BREAK:
        return "break";
    case TERMINAL_NONE:
        return "";
    case TERMINAL_EOF:
        return "EOF";
    }

    return "";
}
