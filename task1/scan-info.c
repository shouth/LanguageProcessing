#include "scan-info.h"
#include <stddef.h>
#include <stdio.h>

int scan_info_init(scan_info_t *si, char *filename)
{
    si->file = fopen(filename, "r");
    if (si->file == NULL) {
        return -1;
    }

    si->c0 = fgetc(si->file);
    si->c1 = fgetc(si->file);
    si->line_number = 1;
    return 0;
}

int scan_info_free(scan_info_t *si)
{
    return fclose(si->file);
}

void scan_info_advance(scan_info_t *si)
{
    si->c0 = si->c1;
    si->c1 = fgetc(si->file);
}

void scan_info_advance_line(scan_info_t *si)
{
    si->line_number++;
}

int scan_info_top(scan_info_t *si)
{
    return si->c0;
}

int scan_info_next(scan_info_t *si)
{
    return si->c1;
}

int scan_info_line_number(scan_info_t *si)
{
    return si->line_number;
}
