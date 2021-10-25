#ifndef STR_BUF_H
#define STR_BUF_H

#include "token-list.h"

typedef struct {
    char buffer[MAXSTRSIZE];
    size_t end;
} str_buf_t;

void str_buf_init(str_buf_t *sb);

int str_buf_push(str_buf_t *sb, char c);

char str_buf_pop(str_buf_t *sb);

const char *str_buf_data(str_buf_t *sb);

#endif /* STR_BUF_H */
