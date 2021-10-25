#include "token-list.h"

struct ID {
    char* name;
    int count;
    struct ID* nextp;
} * idroot;

void init_idtab()
{ /* Initialise the table */
    idroot = NULL;
}

struct ID* search_idtab(char* np)
{ /* search the name pointed by np */
    struct ID* p;

    for (p = idroot; p != NULL; p = p->nextp) {
        if (strcmp(np, p->name) == 0)
            return (p);
    }
    return (NULL);
}

void id_countup(char* np)
{ /* Register and count up the name pointed by np */
    struct ID* p;
    char* cp;

    if ((p = search_idtab(np)) != NULL)
        p->count++;
    else {
        if ((p = (struct ID*)malloc(sizeof(struct ID))) == NULL) {
            printf("can not malloc in id_countup\n");
            return;
        }
        if ((cp = (char*)malloc(strlen(np) + 1)) == NULL) {
            printf("can not malloc-2 in id_countup\n");
            return;
        }
        strcpy(cp, np);
        p->name = cp;
        p->count = 1;
        p->nextp = idroot;
        idroot = p;
    }
}

void print_idtab()
{ /* Output the registered data */
    struct ID* p;

    for (p = idroot; p != NULL; p = p->nextp) {
        if (p->count != 0)
            printf("Identifier / %-10s : %5d\n", p->name, p->count);
    }
}

void release_idtab()
{ /* Release tha data structure */
    struct ID *p, *q;

    for (p = idroot; p != NULL; p = q) {
        free(p->name);
        q = p->nextp;
        free(p);
    }
    init_idtab();
}
