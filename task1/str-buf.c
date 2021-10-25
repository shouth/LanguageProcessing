#include "str-buf.h"
#include "token-list.h"

void str_buf_init(str_buf_t *sb)
{
    sb->buffer[0] = '\0';
    sb->end = 0;
}

int str_buf_push(str_buf_t *sb, char c)
{
    if (sb->end + 1 >= MAXSTRSIZE) {
        return -1;
    }
    sb->buffer[sb->end] = c;
    sb->end++;
    sb->buffer[sb->end] = '\0';
    return 0;
}

char str_buf_pop(str_buf_t *sb)
{
    char ret;
    sb->end--;
    ret = sb->buffer[sb->end];
    sb->buffer[sb->end] = '\0';
    return ret;
}

const char *str_buf_data(str_buf_t *sb)
{
    return sb->buffer;
}
