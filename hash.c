#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "mppl.h"

#define NBHD_RANGE (sizeof(hash_table_hop_t) * 8)

static inline void hash_table_init_buckets(hash_table_t *table)
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
    size_t i;
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

static inline void hash_table_grow(hash_table_t *table)
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

static inline size_t hash_table_index(hash_table_t *table, const void *key)
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
        uint8_t l = lsb(hop);
        if (table->comparator(key, home[l].key)) {
            return home + l;
        }
        hop ^= (hash_table_hop_t) 1 << l;
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
                uint8_t l = lsb(hop);
                if (i + l < NBHD_RANGE) {
                    hash_table_entry_t *next = entry + i + l;
                    empty->key = next->key;
                    empty->value = next->value;
                    next->hop ^= (hash_table_hop_t) 1 << l;
                    next->hop ^= (hash_table_hop_t) 1 << (NBHD_RANGE - i - 1);
                    empty = next;
                    dist -= NBHD_RANGE - 1 - i - l;
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
    assert(table && key && value);

    if (hash_table_remove(table, key)) {
        return &table->removed;
    }
    hash_table_insert_unchecked(table, key, value);
    return NULL;
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
        uint8_t l = lsb(hop);
        if (table->comparator(key, home[l].key)) {
            table->removed.key = home[l].key;
            table->removed.value = home[l].value;
            home[l].key = NULL;
            home->hop ^= (hash_table_hop_t) 1 << l;
            table->size--;
            return &table->removed;
        }
        hop ^= (hash_table_hop_t) 1 << l;
    }
    return NULL;
}
