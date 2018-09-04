#ifndef __ARRAYLIST_H__
#define __ARRAYLIST_H__

#include <unistd.h>
#include <stdint.h>

struct arraylist {
    void** entries;
    size_t list_entry_count;
    size_t entry_count;
    size_t entry_size;
    size_t capacity;
    size_t initial_capacity;
};

struct arraylist* arraylist_new(size_t initial_capacity, size_t entry_size);

void arraylist_free(struct arraylist* list);

size_t arraylist_arrayify(struct arraylist* list, void** array);

uint64_t* arraylist_get(struct arraylist* list, size_t i);

void* arraylist_getptr(struct arraylist* list, size_t i);

void arraylist_set(struct arraylist* list, size_t i, uint64_t value);

void arraylist_setptr(struct arraylist* list, size_t i, void* value);

void arraylist_ensurecap(struct arraylist* list, size_t cap);

size_t arraylist_add(struct arraylist* list, uint64_t value);

size_t arraylist_addptr(struct arraylist* list, void* value);

ssize_t arraylist_index(struct arraylist* list, uint64_t value);

ssize_t arraylist_indexptr(struct arraylist* list, void* value);

#endif