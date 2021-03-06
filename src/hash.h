#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>
#include <unistd.h>

struct hashmap_bucket_entry {
    uint64_t umod_hash;
    char* key;
    void* data;
    struct hashmap_bucket_entry* next;
};

struct hashset_bucket_entry {
    uint64_t umod_hash;
    char* key;
    struct hashset_bucket_entry* next;
};

struct hashmap {
    size_t entry_count;
    size_t bucket_count;
    struct hashmap_bucket_entry** buckets;
};

#define ITER_MAP(map) {for (size_t bucket_i = 0; bucket_i < map->bucket_count; bucket_i++) { for (struct hashmap_bucket_entry* bucket_entry = map->buckets[bucket_i]; bucket_entry != NULL; bucket_entry = bucket_entry->next) { char* str_key = bucket_entry->key; void* ptr_key = (void*)bucket_entry->umod_hash; void* value = bucket_entry->data;

#define ITER_MAP_END() }}}

struct hashset {
    size_t entry_count;
    size_t bucket_count;
    struct hashset_bucket_entry** buckets;
};

#define ITER_SET(set) {for (size_t bucket_i = 0; bucket_i < set->bucket_count; bucket_i++) { for (struct hashset_bucket_entry* bucket_entry = set->buckets[bucket_i]; bucket_entry != NULL; bucket_entry = bucket_entry->next) { char* str_key = bucket_entry->key; void* ptr_key = (void*)bucket_entry->umod_hash;

#define ITER_SET_END() }}}

struct hashmap* new_hashmap(size_t init_cap);

struct hashset* new_hashset(size_t init_cap);

void free_hashmap(struct hashmap* hashmap);

void free_hashset(struct hashset* set);

void* hashmap_get(struct hashmap* hashmap, char* key);

void* hashmap_getptr(struct hashmap* hashmap, void* key);

int hashset_has(struct hashset* set, char* key);

int hashset_hasptr(struct hashset* set, void* key);

void hashmap_put(struct hashmap* hashmap, char* key, void* data);

void hashmap_putptr(struct hashmap* hashmap, void* key, void* data);

void hashset_add(struct hashset* set, char* key);

void hashset_addptr(struct hashset* set, void* key);

struct hashmap* hashmap_clone(struct hashmap* hashmap);

#endif