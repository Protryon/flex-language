#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>
#include <unistd.h>

struct hashmap_bucket_entry {
    uint32_t umod_hash;
    char* key;
    void* data;
    struct hashmap_bucket_entry* next;
};

struct hashset_bucket_entry {
    uint32_t umod_hash;
    char* key;
    struct hashset_bucket_entry* next;
};

struct hashmap {
    size_t entry_count;
    size_t bucket_count;
    struct hashmap_bucket_entry** buckets;
};

struct hashset {
    size_t entry_count;
    size_t bucket_count;
    struct hashset_bucket_entry** buckets;
};

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

#endif