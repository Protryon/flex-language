#include <unistd.h>
#include <stdint.h>
#include "arraylist.h"
#include "smem.h"
#include <string.h>


struct arraylist* arraylist_new(size_t initial_capacity, size_t entry_size) {
    struct arraylist* list = scalloc(sizeof(struct arraylist));
    list->entries = smalloc(sizeof(void*));
    list->entries[0] = scalloc(entry_size * initial_capacity);
    list->entry_size = entry_size;
    list->capacity = initial_capacity;
    list->initial_capacity = initial_capacity;
    list->list_entry_count = 1;
    return list;
}

void arraylist_free(struct arraylist* list) {
    if (list == NULL) return;
    for (size_t i = 0; i < list->list_entry_count; i++) {
        free(list->entries[i]);
    }
    free(list->entries);
    free(list);
}

size_t arraylist_arrayify(struct arraylist* list, void** array) {
    *array = scalloc(list->entry_size * list->entry_count);
    size_t ai = 0;
    uint8_t* arr = *array;
    size_t ccap = list->initial_capacity;
    for (size_t i = 0; i < list->list_entry_count; i++) {
        size_t cpl = ccap;
        if (ai + cpl >= list->entry_count) {
            cpl = list->entry_count - ai;
        }
        memcpy(arr + (ai * list->entry_size), list->entries[i], list->entry_size * cpl);
        ai += cpl;
        if (i > 0) ccap *= 2;
    }
    return list->entry_count;
}

uint64_t* arraylist_get(struct arraylist* list, size_t i) {
    if (i >= list->entry_count) {
        return NULL;
    }
    size_t ccap = list->initial_capacity;
    size_t cps = ccap;
    void* ptr = list->entries[0];
    size_t ptri = 0;
    size_t li = i;
    while (li >= cps) {
        if (ptri >= list->list_entry_count - 1) {
            return NULL;
        }
        ptr = list->entries[++ptri];
        li -= cps;
        cps = ccap;
        ccap *= 2;
    }
    if (li < ccap) {
        uint8_t* lptr = (((uint8_t*)ptr) + (list->entry_size * li));
        return (uint64_t*)lptr;
    }
    return NULL;
}

void* arraylist_getptr(struct arraylist* list, size_t i) {
    if (list->entry_size != sizeof(void*)) {
        return NULL;
    }
    uint64_t* loc = arraylist_get(list, i);
    if (loc == NULL) return NULL;
    return *(void**)loc;
}

void arraylist_set(struct arraylist* list, size_t i, uint64_t value) {
    uint64_t* lptr = arraylist_get(list, i);
    if (list->entry_size == 1) {
        *(uint8_t*)lptr = (uint8_t)value;
    } else if (list->entry_size == 2) {
        *(uint16_t*)lptr = (uint16_t)value;
    } else if (list->entry_size == 4) {
        *(uint32_t*)lptr = (uint32_t)value;
    } else if (list->entry_size == 8) {
        *(uint64_t*)lptr = (uint64_t)value;
    } else {
        return;
    }
}

void arraylist_setptr(struct arraylist* list, size_t i, void* value) {
    arraylist_set(list, i, (uint64_t)value);
}

void arraylist_ensurecap(struct arraylist* list, size_t cap) {
    if (list->capacity > cap) {
        return;
    }
    while (list->capacity <= cap) {
        size_t ls = list->capacity;
        list->capacity *= 2;
        list->list_entry_count++;
        list->entries = srealloc(list->entries, list->list_entry_count * sizeof(void*));
        list->entries[list->list_entry_count - 1] = scalloc(ls * list->entry_size);
    }
}

size_t arraylist_add(struct arraylist* list, uint64_t value) {
    size_t i = list->entry_count;
    arraylist_ensurecap(list, i);
    ++list->entry_count;
    arraylist_set(list, i, value);
    return i;
}

size_t arraylist_addptr(struct arraylist* list, void* value) {
    return arraylist_add(list, (uint64_t)value);
}

ssize_t arraylist_index(struct arraylist* list, uint64_t value) {
    for (size_t i = 0; i < list->entry_count; i++) {
        if (*arraylist_get(list, i) == value) {
            return i;
        }
    }
    return -1;
}

ssize_t arraylist_indexptr(struct arraylist* list, void* value) {
    for (size_t i = 0; i < list->entry_count; i++) {
        if (arraylist_getptr(list, i) == value) {
            return i;
        }
    }
    return -1;
}
