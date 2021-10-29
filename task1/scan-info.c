#include "scan-info.h"
#include <stddef.h>
#include <stdio.h>

int scanner_init(scanner_t *si, char *filename)
{
    si->file = fopen(filename, "r");
    if (si->file == NULL) {
        return -1;
    }

    si->top = fgetc(si->file);
    si->next = fgetc(si->file);
    si->line_num = 1;
    si->col_num = 1;
    return 0;
}

int scanner_free(scanner_t *si)
{
    return fclose(si->file);
}

void scanner_advance(scanner_t *si)
{
    si->top = si->next;
    si->next = fgetc(si->file);
    si->col_num++;
}

void scanner_advance_line(scanner_t *si)
{
    si->line_num++;
    si->col_num = 1;
}

int scanner_top(scanner_t *si)
{
    return si->top;
}

int scanner_next(scanner_t *si)
{
    return si->next;
}

int scanner_line_number(scanner_t *si)
{
    return si->line_num;
}

int scanner_col_number(scanner_t *si)
{
    return si->col_num;
}
