#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "mppl.h"

#define NBHD_RANGE (sizeof(hash_table_hop_t) * 8)

static void hash_table_init_buckets(hash_table_t *table)
{
    size_t i;
    table->size = 0;
    table->bucket_cnt = table->capacity + NBHD_RANGE;
    table->buckets = new_arr(hash_table_entry_t, table->bucket_cnt);
    for (i = 0; i < table->bucket_cnt; i++) {
        table->buckets[i].hop = 0;
        table->buckets[i].key = NULL;
        table->buckets[i].value = NULL;
    }
}

hash_table_t *new_hash_table(hash_table_comparator_t *comparator, hash_table_hasher_t *hasher)
{
    hash_table_t *ret;
    assert(comparator && hasher);

    ret = new(hash_table_t);
    ret->capacity = 1 << 6;
    ret->load_factor = 60;
    ret->comparator = comparator;
    ret->hasher = hasher;
    hash_table_init_buckets(ret);
    return ret;
}

void delete_hash_table(hash_table_t *table, hash_table_deleter_t *key_deleter, hash_table_deleter_t *value_deleter)
{
    size_t i;
    if (!table) {
        return;
    }

    for (i = 0; i < table->bucket_cnt; i++) {
        if (table->buckets[i].key) {
            if (key_deleter) {
                key_deleter(table->buckets[i].key);
            }
            if (value_deleter) {
                value_deleter(table->buckets[i].value);
            }
        }
    }
    free(table->buckets);
    free(table);
}

static void hash_table_grow(hash_table_t *table)
{
    hash_table_entry_t *old_buckets;
    size_t old_bucket_cnt;
    size_t i;
    assert(table);

    old_buckets = table->buckets;
    old_bucket_cnt = table->bucket_cnt;
    table->capacity <<= 1;
    hash_table_init_buckets(table);
    for (i = 0; i < old_bucket_cnt; i++) {
        if (old_buckets[i].key) {
            hash_table_insert_unchecked(table, old_buckets[i].key, old_buckets[i].value);
        }
    }
    free(old_buckets);
}

static size_t hash_table_index(hash_table_t *table, const void *key)
{
    assert(table && key);
    return table->hasher(key) & (table->capacity - 1);
}

const hash_table_entry_t *hash_table_find(hash_table_t *table, const void *key)
{
    hash_table_entry_t *home;
    hash_table_hop_t hop;
    size_t index;
    assert(table && key);

    index = hash_table_index(table, key);
    home = table->buckets + index;
    hop = home->hop;
    while (hop) {
        uint8_t t = trailing0(hop);
        if (table->comparator(key, home[t].key)) {
            return home + t;
        }
        hop ^= (hash_table_hop_t) 1 << t;
    }
    return NULL;
}

void hash_table_insert_unchecked(hash_table_t *table, void *key, void *value)
{
    hash_table_entry_t *home, *empty = NULL;
    size_t dist, index;
    size_t i;
    assert(table && key && value);

    index = hash_table_index(table, key);
    home = table->buckets + index;
    for (dist = 0; dist < NBHD_RANGE * 8; dist++) {
        if (!home[dist].key) {
            empty = home + dist;
            break;
        }
    }

    while (empty && dist >= NBHD_RANGE) {
        hash_table_entry_t *entry = empty - NBHD_RANGE + 1;
        for (i = 0; i < NBHD_RANGE; i++) {
            hash_table_hop_t hop = entry[i].hop;
            if (hop) {
                uint8_t t = trailing0(hop);
                if (i + t < NBHD_RANGE) {
                    hash_table_entry_t *next = entry + i + t;
                    empty->key = next->key;
                    empty->value = next->value;
                    next->hop ^= (hash_table_hop_t) 1 << t;
                    next->hop ^= (hash_table_hop_t) 1 << (NBHD_RANGE - i - 1);
                    empty = next;
                    dist -= NBHD_RANGE - 1 - i - t;
                    break;
                }
            }
        }
        if (i == NBHD_RANGE) {
            empty->key = NULL;
            empty = NULL;
        }
    }

    if (!empty) {
        hash_table_grow(table);
        hash_table_insert_unchecked(table, key, value);
    } else {
        empty->key = key;
        empty->value = value;
        home->hop ^= (hash_table_hop_t) 1 << dist;
        table->size++;
        if (100 * table->size / table->bucket_cnt >= table->load_factor) {
            hash_table_grow(table);
        }
    }
}

hash_table_entry_t *hash_table_insert(hash_table_t *table, void *key, void *value)
{
    hash_table_entry_t *ret;
    assert(table && key && value);

    ret = hash_table_remove(table, key);
    hash_table_insert_unchecked(table, key, value);
    return ret;
}

hash_table_entry_t *hash_table_remove(hash_table_t *table, const void *key)
{
    hash_table_entry_t *home;
    hash_table_hop_t hop;
    size_t index;
    assert(table && key);

    index = hash_table_index(table, key);
    home = table->buckets + index;
    hop = home->hop;
    while (hop) {
        uint8_t t = trailing0(hop);
        if (table->comparator(key, home[t].key)) {
            table->removed.key = home[t].key;
            table->removed.value = home[t].value;
            home[t].key = NULL;
            home->hop ^= (hash_table_hop_t) 1 << t;
            table->size--;
            return &table->removed;
        }
        hop ^= (hash_table_hop_t) 1 << t;
    }
    return NULL;
}

static int use_ansi = 0;

void console_ansi(int flag)
{ use_ansi = flag; }

void console_set(sgr_t code)
{
    if (use_ansi) {
        printf("\033[%dm", code);
    }
}

void console_reset()
{ console_set(SGR_RESET); }

void console_24bit(color_t color)
{
    if (use_ansi) {
        printf("\033[38;2;%ld;%ld;%ldm", (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
    }
}
