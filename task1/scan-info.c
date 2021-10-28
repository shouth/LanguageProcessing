#include "scan-info.h"
#include <stddef.h>
#include <stdio.h>

int scan_info_init(scan_info_t *si, char *filename)
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

int scan_info_free(scan_info_t *si)
{
    return fclose(si->file);
}

void scan_info_advance(scan_info_t *si)
{
    si->top = si->next;
    si->next = fgetc(si->file);
    si->col_num++;
}

void scan_info_advance_line(scan_info_t *si)
{
    si->line_num++;
    si->col_num = 1;
}

int scan_info_top(scan_info_t *si)
{
    return si->top;
}

int scan_info_next(scan_info_t *si)
{
    return si->next;
}

int scan_info_line_number(scan_info_t *si)
{
    return si->line_num;
}

int scan_info_col_number(scan_info_t *si)
{
    return si->col_num;
}
