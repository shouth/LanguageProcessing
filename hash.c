#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "mppl.h"

uint64_t fnv1(const uint8_t *ptr, size_t len)
{
    uint64_t ret   = 0xcbf29ce484222325;
    uint64_t prime = 0x00000100000001b3;
    size_t i;

    for (i = 0; i < len; i++) {
        ret *= prime;
        ret ^= ptr[i];
    }
    return ret;
}

uint64_t fnv1_int(uint64_t value)
{
    return fnv1((uint8_t *) &value, sizeof(value));
}

uint64_t fnv1_ptr(const void *ptr)
{
    return fnv1((uint8_t *) &ptr, sizeof(ptr));
}

#define NBHD_RANGE (sizeof(hash_table_hop_t) * 8)

static void hash_table_init_buckets(hash_table_t *table)
{
    size_t i;
    table->bucket_cnt = table->capacity + NBHD_RANGE;
    table->buckets = new_arr(hash_table_entry_t, table->bucket_cnt);
    for (i = 0; i < table->bucket_cnt; i++) {
        table->buckets[i].hop = 0;
        table->buckets[i].key = NULL;
        table->buckets[i].value = NULL;
    }
}

hash_table_t *new_hash_table(
    hash_table_comparator_t *comparator, hash_table_hasher_t *hasher,
    hash_table_deleter_t *key_deleter, hash_table_deleter_t *value_deleter)
{
    hash_table_t *ret;
    size_t i;
    assert(comparator && hasher);

    ret = new(hash_table_t);
    ret->capacity = 1 << 6;
    ret->size = 0;
    ret->load_factor = 60;
    ret->comparator = comparator;
    ret->hasher = hasher;
    ret->key_deleter = key_deleter;
    ret->value_deleter = value_deleter;
    hash_table_init_buckets(ret);
    return ret;
}

static void hash_table_delete_kv(hash_table_t *table, void *key, void *value)
{
    assert(table && key && value);

    if (table->key_deleter) {
        table->key_deleter(key);
    }
    if (table->value_deleter) {
        table->value_deleter(value);
    }
}

void delete_hash_table(hash_table_t *table)
{
    if (table) {
        size_t i;
        for (i = 0; i < table->bucket_cnt; i++) {
            if (table->buckets[i].key) {
                hash_table_delete_kv(table, table->buckets[i].key, table->buckets[i].value);
            }
        }
        free(table->buckets);
        free(table);
    }
}

static void hash_table_grow(hash_table_t *table, int enforce)
{
    hash_table_entry_t *old_buckets;
    size_t old_bucket_cnt;
    size_t i;
    assert(table);

    if (!enforce && 100 * table->size / table->bucket_cnt < table->load_factor) {
        return;
    }

    old_buckets = table->buckets;
    old_bucket_cnt = table->bucket_cnt;
    table->capacity <<= 1;
    table->size = 0;
    hash_table_init_buckets(table);
    for (i = 0; i < old_bucket_cnt; i++) {
        if (old_buckets[i].key) {
            hash_table_insert(table, old_buckets[i].key, old_buckets[i].value);
        }
    }
    free(old_buckets);
}

static size_t hash_table_index(hash_table_t *table, const void *key)
{
    assert(table && key);
    return table->hasher(key) & (table->capacity - 1);
}

const void *hash_table_find(hash_table_t *table, const void *key)
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
            return home[l].value;
        }
        hop ^= (hash_table_hop_t) 1 << l;
    }
    return NULL;
}

int hash_table_insert(hash_table_t *table, void *key, void *value)
{
    hash_table_entry_t *home, *empty = NULL;
    size_t dist, index;
    size_t i;
    assert(table && key && value);

    if (hash_table_find(table, key)) {
        hash_table_delete_kv(table, key, value);
        return 0;
    }

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
                    next->key = NULL;
                    next->hop ^= (hash_table_hop_t) 1 << l;
                    next->hop ^= (hash_table_hop_t) 1 << (NBHD_RANGE - i - 1);
                    empty = next;
                    dist -= NBHD_RANGE - 1 - i - l;
                    break;
                }
            }
        }
        if (i == NBHD_RANGE) {
            empty = NULL;
        }
    }

    if (!empty) {
        hash_table_grow(table, 1);
        return hash_table_insert(table, key, value);
    }

    empty->key = key;
    empty->value = value;
    home->hop ^= (hash_table_hop_t) 1 << dist;
    table->size++;
    hash_table_grow(table, 0);
    return 1;
}

int hash_table_remove(hash_table_t *table, const void *key)
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
            hash_table_delete_kv(table, home[l].key, home[l].value);
            home[l].key = NULL;
            home->hop ^= (hash_table_hop_t) 1 << l;
            table->size--;
            return 1;
        }
        hop ^= (hash_table_hop_t) 1 << l;
    }
    return 0;
}
