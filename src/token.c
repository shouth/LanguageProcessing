#include "token.h"

const char *token_to_str(token_kind_t type)
{
  /* clang-format off */
  switch (type) {
  case TOKEN_IDENT:     return "identifier";
  case TOKEN_PROGRAM:   return "program";
  case TOKEN_VAR:       return "var";
  case TOKEN_ARRAY:     return "array";
  case TOKEN_OF:        return "of";
  case TOKEN_BEGIN:     return "begin";
  case TOKEN_END:       return "end";
  case TOKEN_IF:        return "if";
  case TOKEN_THEN:      return "then";
  case TOKEN_ELSE:      return "else";
  case TOKEN_PROCEDURE: return "procedure";
  case TOKEN_RETURN:    return "return";
  case TOKEN_CALL:      return "call";
  case TOKEN_WHILE:     return "while";
  case TOKEN_DO:        return "do";
  case TOKEN_NOT:       return "not";
  case TOKEN_OR:        return "or";
  case TOKEN_DIV:       return "div";
  case TOKEN_AND:       return "and";
  case TOKEN_CHAR:      return "char";
  case TOKEN_INTEGER:   return "integer";
  case TOKEN_BOOLEAN:   return "boolean";
  case TOKEN_READLN:    return "readln";
  case TOKEN_WRITELN:   return "writeln";
  case TOKEN_TRUE:      return "true";
  case TOKEN_FALSE:     return "false";
  case TOKEN_NUMBER:    return "number";
  case TOKEN_STRING:    return "string";
  case TOKEN_PLUS:      return "+";
  case TOKEN_MINUS:     return "-";
  case TOKEN_STAR:      return "*";
  case TOKEN_EQUAL:     return "=";
  case TOKEN_NOTEQ:     return "<>";
  case TOKEN_LE:        return "<";
  case TOKEN_LEEQ:      return "<=";
  case TOKEN_GR:        return ">";
  case TOKEN_GREQ:      return ">=";
  case TOKEN_LPAREN:    return "(";
  case TOKEN_RPAREN:    return ")";
  case TOKEN_LSQPAREN:  return "[";
  case TOKEN_RSQPAREN:  return "]";
  case TOKEN_ASSIGN:    return ":=";
  case TOKEN_DOT:       return ".";
  case TOKEN_COMMA:     return ",";
  case TOKEN_COLON:     return ":";
  case TOKEN_SEMI:      return ";";
  case TOKEN_READ:      return "read";
  case TOKEN_WRITE:     return "write";
  case TOKEN_BREAK:     return "break";
  case TOKEN_EOF:       return "EOF";

  case TOKEN_UNKNOWN:   return "UNKNOWN";
  case TOKEN_ERROR:     return "ERROR";

  case TOKEN_BRACES_COMMENT:
  case TOKEN_CSTYLE_COMMENT:
  default:              return "";
  }
  /* clang-format on */
}
