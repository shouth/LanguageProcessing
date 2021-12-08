#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "mppl.h"

token_kind_t lex_space(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(is_space(cursol_first(cur)));

    cursol_next(cur);
    while (is_space(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_WHITESPACE;
}

token_kind_t lex_braces_comment(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(cursol_first(cur) == '{');

    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '}') {
            cursol_next(cur);
            ret->braces_comment.terminated = 1;
            return TOKEN_BRACES_COMMENT;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))) {
            ret->braces_comment.terminated = 0;
            return TOKEN_BRACES_COMMENT;
        }

        cursol_next(cur);
    }
}

token_kind_t lex_cstyle_comment(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(cursol_first(cur) == '/' && cursol_second(cur) == '*');

    cursol_next(cur);
    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '*' && cursol_second(cur) == '/') {
            cursol_next(cur);
            cursol_next(cur);
            ret->cstyle_comment.terminated = 1;
            return TOKEN_CSTYLE_COMMENT;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))) {
            ret->cstyle_comment.terminated = 0;
            return TOKEN_CSTYLE_COMMENT;
        }

        cursol_next(cur);
    }
}

token_kind_t lex_string(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(cursol_first(cur) == '\'');

    ret->string.len = 0;
    ret->string.str_len = 0;
    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '\'') {
            cursol_next(cur);

            if (cursol_first(cur) != '\'') {
                ret->string.terminated = 1;
                return TOKEN_STRING;
            } else {
                ret->string.len++;
            }
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))
            || cursol_first(cur) == '\r' || cursol_first(cur) == '\n')
        {
            ret->string.terminated = 0;
            return TOKEN_STRING;
        }

        ret->string.len++;
        ret->string.str_len++;
        cursol_next(cur);
    }
}

token_kind_t lex_name_or_keyword(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(is_alphabet(cursol_first(cur)));

    cursol_next(cur);
    while (is_alphabet(cursol_first(cur)) || is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_NAME;
}

token_kind_t lex_number(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);
    assert(is_number(cursol_first(cur)));

    cursol_next(cur);
    while (is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_NUMBER;
}

token_kind_t lex_symbol(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);

    switch (cursol_first(cur)) {
    case '+':
        cursol_next(cur);
        return TOKEN_PLUS;
    case '-':
        cursol_next(cur);
        return TOKEN_MINUS;
    case '*':
        cursol_next(cur);
        return TOKEN_STAR;
    case '=':
        cursol_next(cur);
        return TOKEN_EQUAL;
    case '(':
        cursol_next(cur);
        return TOKEN_LPAREN;
    case ')':
        cursol_next(cur);
        return TOKEN_RPAREN;
    case '[':
        cursol_next(cur);
        return TOKEN_LSQPAREN;
    case ']':
        cursol_next(cur);
        return TOKEN_RSQPAREN;
    case '.':
        cursol_next(cur);
        return TOKEN_DOT;
    case ',':
        cursol_next(cur);
        return TOKEN_COMMA;
    case ';':
        cursol_next(cur);
        return TOKEN_SEMI;

    case '<':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '>':
            cursol_next(cur);
            return TOKEN_NOTEQ;
        case '=':
            cursol_next(cur);
            return TOKEN_LEEQ;
        default:
            return TOKEN_LE;
        }

    case '>':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            return TOKEN_GREQ;
        default:
            return TOKEN_GR;
        }

    case ':':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            return TOKEN_ASSIGN;
        default:
            return TOKEN_COLON;
        }

    default:
        cursol_next(cur);
        return TOKEN_UNKNOWN;
    }
}

token_kind_t lex_delimited(cursol_t *cur, token_info_t *ret)
{
    assert(cur && ret);

    if (cursol_eof(cur)) {
        return TOKEN_EOF;
    }

    if (is_space(cursol_first(cur))) {
        return lex_space(cur, ret);
    }

    if (cursol_first(cur) == '{') {
        return lex_braces_comment(cur, ret);
    }

    if (cursol_first(cur) == '/' && cursol_second(cur) == '*') {
        return lex_cstyle_comment(cur, ret);
    }

    if (cursol_first(cur) == '\'') {
        return lex_string(cur, ret);
    }

    if (is_alphabet(cursol_first(cur))) {
        return lex_name_or_keyword(cur, ret);
    }

    if (is_number(cursol_first(cur))) {
        return lex_number(cur, ret);
    }

    return lex_symbol(cur, ret);
}

token_kind_t keywords[] = {
    TOKEN_PROGRAM,
    TOKEN_VAR,
    TOKEN_ARRAY,
    TOKEN_OF,
    TOKEN_BEGIN,
    TOKEN_END,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_PROCEDURE,
    TOKEN_RETURN,
    TOKEN_CALL,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_NOT,
    TOKEN_OR,
    TOKEN_DIV,
    TOKEN_AND,
    TOKEN_CHAR,
    TOKEN_INTEGER,
    TOKEN_BOOLEAN,
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_READLN,
    TOKEN_WRITELN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_BREAK,
};

const size_t keywords_size = sizeof(keywords) / sizeof(*keywords);

void lex_token(cursol_t *cursol, token_t *ret)
{
    token_info_t info;
    msg_t *msg;
    size_t i, j;
    assert(cursol && ret);

    ret->ptr = cursol->ptr;
    ret->pos = cursol_position(cursol);
    ret->type = lex_delimited(cursol, &info);
    ret->len = cursol_position(cursol) - ret->pos;

    switch (ret->type) {
    case TOKEN_NAME:
        for (i = 0; i < keywords_size; i++) {
            const char *ptr = token_to_str(keywords[i]);
            for (j = 0; j < ret->len; j++) {
                if (ret->ptr[j] != ptr[j]) {
                    break;
                }
            }
            if (j == ret->len && ptr[j] == '\0') {
                ret->type = keywords[i];
                break;
            }
        }
        return;

    case TOKEN_NUMBER:
        errno = 0;
        ret->data.number.value = strtoul(ret->ptr, NULL, 10);
        if (errno == ERANGE || ret->data.number.value > 32767) {
            msg = new_msg(cursol->src, ret->pos, ret->len, MSG_ERROR, "number is too large");
            msg_add_inline_entry(msg, ret->pos, ret->len, "number needs to be less than 32768");
            msg_emit(msg);
            ret->type = TOKEN_ERROR;
        }
        return;

    case TOKEN_STRING:
        if (!info.string.terminated) {
            if (cursol_eof(cursol)) {
                msg = new_msg(cursol->src, ret->pos, ret->len, MSG_ERROR, "string is unterminated");
            } else {
                msg = new_msg(cursol->src, cursol_position(cursol), 1, MSG_ERROR, "nongraphical character");
            }
            msg_emit(msg);
            ret->type = TOKEN_ERROR;
        } else {
            ret->data.string.str_len = info.string.str_len;
            ret->data.string.ptr = ret->ptr + 1;
            ret->data.string.len = info.string.len;
        }
        return;

    case TOKEN_BRACES_COMMENT:
        if (!info.braces_comment.terminated) {
            if (cursol_eof(cursol)) {
                msg = new_msg(cursol->src, ret->pos, 1, MSG_ERROR, "comment is unterminated");
            } else {
                msg = new_msg(cursol->src, cursol_position(cursol), 1, MSG_ERROR, "nongraphical character");
            }
            msg_emit(msg);
            ret->type = TOKEN_ERROR;
        }
        return;

    case TOKEN_CSTYLE_COMMENT:
        if (!info.cstyle_comment.terminated) {
            if (cursol_eof(cursol)) {
                msg = new_msg(cursol->src, ret->pos, 2, MSG_ERROR, "comment is unterminated");
            } else {
                msg = new_msg(cursol->src, cursol_position(cursol), 1, MSG_ERROR, "nongraphical character");
            }
            msg_emit(msg);
            ret->type = TOKEN_ERROR;
        }
        return;

    case TOKEN_UNKNOWN:
        if (is_graphical(ret->ptr[0])) {
            msg = new_msg(cursol->src, ret->pos, ret->len,
                MSG_ERROR, "stray `%c` in program", ret->ptr[0]);
        } else {
            msg = new_msg(cursol->src, ret->pos, ret->len,
                MSG_ERROR, "stray \\%03o in program", (unsigned char) ret->ptr[0]);
        }
        msg_emit(msg);
        return;
    }
}

const char *token_to_str(token_kind_t type)
{
    switch (type) {
    case TOKEN_NAME:
        return "NAME";
    case TOKEN_PROGRAM:
        return "program";
    case TOKEN_VAR:
        return "var";
    case TOKEN_ARRAY:
        return "array";
    case TOKEN_OF:
        return "of";
    case TOKEN_BEGIN:
        return "begin";
    case TOKEN_END:
        return "end";
    case TOKEN_IF:
        return "if";
    case TOKEN_THEN:
        return "then";
    case TOKEN_ELSE:
        return "else";
    case TOKEN_PROCEDURE:
        return "procedure";
    case TOKEN_RETURN:
        return "return";
    case TOKEN_CALL:
        return "call";
    case TOKEN_WHILE:
        return "while";
    case TOKEN_DO:
        return "do";
    case TOKEN_NOT:
        return "not";
    case TOKEN_OR:
        return "or";
    case TOKEN_DIV:
        return "div";
    case TOKEN_AND:
        return "and";
    case TOKEN_CHAR:
        return "char";
    case TOKEN_INTEGER:
        return "integer";
    case TOKEN_BOOLEAN:
        return "boolean";
    case TOKEN_READLN:
        return "readln";
    case TOKEN_WRITELN:
        return "writeln";
    case TOKEN_TRUE:
        return "true";
    case TOKEN_FALSE:
        return "false";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_EQUAL:
        return "=";
    case TOKEN_NOTEQ:
        return "<>";
    case TOKEN_LE:
        return "<";
    case TOKEN_LEEQ:
        return "<=";
    case TOKEN_GR:
        return ">";
    case TOKEN_GREQ:
        return ">=";
    case TOKEN_LPAREN:
        return "(";
    case TOKEN_RPAREN:
        return ")";
    case TOKEN_LSQPAREN:
        return "[";
    case TOKEN_RSQPAREN:
        return "]";
    case TOKEN_ASSIGN:
        return ":=";
    case TOKEN_DOT:
        return ".";
    case TOKEN_COMMA:
        return ",";
    case TOKEN_COLON:
        return ":";
    case TOKEN_SEMI:
        return ";";
    case TOKEN_READ:
        return "read";
    case TOKEN_WRITE:
        return "write";
    case TOKEN_BREAK:
        return "break";
    case TOKEN_EOF:
        return "EOF";

    case TOKEN_UNKNOWN:
        return "UNKNOWN";
    case TOKEN_ERROR:
        return "ERROR";
    }

    return "";
}
