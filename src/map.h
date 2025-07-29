#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stddef.h>

typedef const char* map_key_t;
typedef uintptr_t map_value_t;
typedef uint64_t map_hash_t;

typedef struct map_entry {
    map_key_t key;
    map_value_t value;
    map_hash_t hash;
} map_entry_t;

typedef struct map {
    map_entry_t* entries;
    size_t size;
    size_t capacity;
} map_t;

map_t* map_create(size_t capacity);
void map_destroy(map_t* map);
map_value_t map_get(map_t* map, const char* key, size_t length);
void map_set(map_t* map, const char* key, size_t length, map_value_t value);

#endif