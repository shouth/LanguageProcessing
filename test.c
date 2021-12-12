#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include "mppl.h"

#define SIZE 10000000

static int comp(const void *lhs, const void *rhs)
{
    const int *l = lhs, *r = rhs;
    return *l == *r;
}

static uint64_t hasher(const void *value)
{
    const int *x = value;
    return fnv1_int(*x);
}
/*
int main()
{
    size_t i;
    hash_table_t *table = new_hash_table(comp, hasher);

    struct timeval start, end;
    uint64_t t = 0;
    int *keys = new_arr(int, SIZE);
    for (i = 0; i < SIZE; i++) {
        keys[i] = i;
    }

    gettimeofday(&start, NULL);
    for (i = 0; i < SIZE; i++) {
        hash_table_insert(table, keys + i, keys + i);
    }
    gettimeofday(&end, NULL);
    t = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    printf("all insert: %ld (us), per insert: %ld (ns)\n", t, t * 1000 / SIZE);
    fflush(stdout);

    gettimeofday(&start, NULL);
    for (i = 0; i < SIZE; i++) {
        const hash_table_entry_t *entry = hash_table_find(table, keys + i);
    }
    gettimeofday(&end, NULL);
    t = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    printf("all find: %ld (us), per find: %ld (ns)\n", t, t * 1000 / SIZE);
    fflush(stdout);

    delete_hash_table(table, NULL, NULL);
}
 */
