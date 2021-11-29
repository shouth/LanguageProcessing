#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "message.h"
#include "terminal.h"
#include "lexer.h"
#include "util.h"

terminal_type_t keywords[] = {
    TERMINAL_PROGRAM,
    TERMINAL_VAR,
    TERMINAL_ARRAY,
    TERMINAL_OF,
    TERMINAL_BEGIN,
    TERMINAL_END,
    TERMINAL_IF,
    TERMINAL_THEN,
    TERMINAL_ELSE,
    TERMINAL_PROCEDURE,
    TERMINAL_RETURN,
    TERMINAL_CALL,
    TERMINAL_WHILE,
    TERMINAL_DO,
    TERMINAL_NOT,
    TERMINAL_OR,
    TERMINAL_DIV,
    TERMINAL_AND,
    TERMINAL_CHAR,
    TERMINAL_INTEGER,
    TERMINAL_BOOLEAN,
    TERMINAL_READ,
    TERMINAL_WRITE,
    TERMINAL_READLN,
    TERMINAL_WRITELN,
    TERMINAL_TRUE,
    TERMINAL_FALSE,
    TERMINAL_BREAK,
};

const size_t keywords_size = sizeof(keywords) / sizeof(*keywords);

void terminal_from_token(terminal_t *terminal, const token_t *token)
{
    size_t i, j;
    const char *ptr;
    msg_t *msg;
    assert(token != NULL && terminal != NULL);

    terminal->ptr = token->ptr;
    terminal->len = token->len;
    terminal->src = token->src;
    terminal->pos = token->pos;

    switch (token->type) {
    case TOKEN_NAME_OR_KEYWORD:
        terminal->type = TERMINAL_NAME;
        for (i = 0; i < keywords_size; i++) {
            ptr = terminal_to_str(keywords[i]);
            for (j = 0; j < token->len; j++) {
                if (token->ptr[j] != ptr[j]) {
                    break;
                }
            }
            if (j == token->len && ptr[j] == '\0') {
                terminal->type = keywords[i];
                break;
            }
        }
        return;

    case TOKEN_NUMBER:
        terminal->data.number.value = strtoul(token->ptr, NULL, 10);
        if (errno == ERANGE || terminal->data.number.value > 32767) {
            msg = new_msg(token->src, token->pos, token->len, MSG_ERROR, "number is too large");
            msg_add_inline_entry(msg, token->pos, token->len, "number needs to be less than 32768");
            msg_emit(msg);
            exit(1);
        }
        terminal->type = TERMINAL_NUMBER;
        return;

    case TOKEN_STRING:
        if (!token->data.string.terminated) {
            msg = new_msg(token->src, token->pos, token->len, MSG_ERROR, "string is unterminated");
            msg_emit(msg);
            exit(1);
        }
        terminal->type = TERMINAL_STRING;
        terminal->data.string.str_len = token->data.string.str_len;
        terminal->data.string.ptr = token->ptr + 1;
        terminal->data.string.len = token->data.string.len;
        return;

    case TOKEN_BRACES_COMMENT:
        if (!token->data.braces_comment.terminated) {
            msg = new_msg(token->src, token->pos, 1, MSG_ERROR, "comment is unterminated");
            msg_emit(msg);
            exit(1);
        }
        terminal->type = TERMINAL_NONE;
        return;

    case TOKEN_CSTYLE_COMMENT:
        if (!token->data.cstyle_comment.terminated) {
            msg = new_msg(token->src, token->pos, 2, MSG_ERROR, "comment is unterminated");
            msg_emit(msg);
            exit(1);
        }
        terminal->type = TERMINAL_NONE;
        return;

    case TOKEN_WHITESPACE:
        terminal->type = TERMINAL_NONE;
        return;

    case TOKEN_PLUS:
        terminal->type = TERMINAL_PLUS;
        return;

    case TOKEN_MINUS:
        terminal->type = TERMINAL_MINUS;
        return;

    case TOKEN_STAR:
        terminal->type = TERMINAL_STAR;
        return;

    case TOKEN_EQUAL:
        terminal->type = TERMINAL_EQUAL;
        return;

    case TOKEN_NOTEQ:
        terminal->type = TERMINAL_NOTEQ;
        return;

    case TOKEN_LE:
        terminal->type = TERMINAL_LE;
        return;

    case TOKEN_LEEQ:
        terminal->type = TERMINAL_LEEQ;
        return;

    case TOKEN_GR:
        terminal->type = TERMINAL_GR;
        return;

    case TOKEN_GREQ:
        terminal->type = TERMINAL_GREQ;
        return;

    case TOKEN_LPAREN:
        terminal->type = TERMINAL_LPAREN;
        return;

    case TOKEN_RPAREN:
        terminal->type = TERMINAL_RPAREN;
        return;

    case TOKEN_LSQPAREN:
        terminal->type = TERMINAL_LSQPAREN;
        return;

    case TOKEN_RSQPAREN:
        terminal->type = TERMINAL_RSQPAREN;
        return;

    case TOKEN_ASSIGN:
        terminal->type = TERMINAL_ASSIGN;
        return;

    case TOKEN_DOT:
        terminal->type = TERMINAL_DOT;
        return;

    case TOKEN_COMMA:
        terminal->type = TERMINAL_COMMA;
        return;

    case TOKEN_COLON:
        terminal->type = TERMINAL_COLON;
        return;

    case TOKEN_SEMI:
        terminal->type = TERMINAL_SEMI;
        return;

    case TOKEN_EOF:
        terminal->type = TERMINAL_EOF;
        return;

    case TOKEN_UNKNOWN:
        if (is_graphical(token->ptr[0])) {
            msg = new_msg(token->src, token->pos, token->len,
                MSG_ERROR, "stray `%c` in program", token->ptr[0]);
        } else {
            msg = new_msg(token->src, token->pos, token->len,
                MSG_ERROR, "stray \\%03o in program", (unsigned char) token->ptr[0]);
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
