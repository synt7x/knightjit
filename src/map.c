#include "map.h"
#include "debug.h"
#include <memory.h>
#include <string.h>
#include <stdlib.h>

// FNV-1a mirrored from the PHP source code
map_hash_t hash(const char* key, size_t length) {
    map_hash_t hash = 2166136261;

    for (size_t i = 0; i < length; i++) {
        hash ^= (unsigned char)key[i];
        hash *= 16777619;
    }

    return !hash ? 1 : hash;
}

void map_resize(map_t* map) {
    if (!map) return;

    size_t prev_capacity = map->capacity;
    map_entry_t* prev_entries = map->entries;
    
    map->capacity *= 2;
    map->entries = malloc(sizeof(map_entry_t) * map->capacity);
    if (!map->entries) {
        panic("Failed to allocate memory for resized map entries");
    }
    memset(map->entries, 0, sizeof(map_entry_t) * map->capacity);
    
    map->size = 0;

    for (size_t i = 0; i < prev_capacity; i++) {
        if (prev_entries[i].hash != 0) {
            map_set(map, prev_entries[i].key, strlen(prev_entries[i].key), prev_entries[i].value);
        }
    }

    free(prev_entries);
}

map_t* map_create(size_t capacity) {
    map_t* map = malloc(sizeof(map_t));
    if (!map) {
        panic("Failed to allocate memory for map");
    }

    map->entries = malloc(sizeof(map_entry_t) * capacity);
    if (!map->entries) {
        free(map);
        panic("Failed to allocate memory for map entries");
    }

    map->size = 0;
    map->capacity = capacity;

    memset(map->entries, 0, sizeof(map_entry_t) * capacity);
    return map;
}

void map_destroy(map_t* map) {
    if (!map) return;

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].key) {
            free((void*) map->entries[i].key);
        }
    }

    free(map->entries);
    free(map);
}

map_value_t map_get(map_t* map, const char* key, size_t length) {
    if (!map || map->size == 0) return 0;

    map_hash_t hashed = hash(key, length);
    size_t index = hashed & (map->capacity - 1);

    for (size_t i = 0; i < map->capacity; i++) {
        size_t idx = (index + i) % map->capacity;
        if (map->entries[idx].hash == 0) {
            return 0;
        }
        if (map->entries[idx].hash == hashed && 
            strncmp(map->entries[idx].key, key, length) == 0 &&
            strlen(map->entries[idx].key) == length) {
            return map->entries[idx].value;
        }
    }

    return 0;
}

void map_set(map_t* map, const char* key, size_t length, map_value_t value) {
    if (!map) return;

    if (map->size >= (map->capacity * 3) / 4) {
        map_resize(map);
    }

    map_hash_t hashed = hash(key, length);
    size_t index = hashed % map->capacity;
    
    for (size_t i = 0; i < map->capacity; i++) {
        size_t idx = (index + i) % map->capacity;
                
        if (map->entries[idx].hash == 0) {
            char* cloned = malloc(length + 1);
            memcpy(cloned, key, length);
            cloned[length] = '\0';
            
            map->entries[idx].key = cloned;
            map->entries[idx].value = value;
            map->entries[idx].hash = hashed;
            map->size++;
            
            return;
        } else if (map->entries[idx].hash == hashed && strncmp(map->entries[idx].key, key, length) == 0 && strlen(map->entries[idx].key) == length) {
            map->entries[idx].value = value;
            return;
        }
    }

    panic("Failed to set value in map");
}