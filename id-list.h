#ifndef ID_LIST_H
#define ID_LIST_H

#include "token-list.h"

struct ID {
    char* name;
    int count;
    struct ID* nextp;
} * idroot;

void init_idtab();

struct ID* search_idtab(char* np);

void id_countup(char* np);

void print_idtab();

void release_idtab();

#endif /* ID_LIST_H */
