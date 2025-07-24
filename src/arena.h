#ifndef ARENA_H
#define ARENA_H

#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t size;
    size_t capacity;
    void** nodes;
} arena_t;

arena_t* arena_create(size_t initial_capacity);
void arena_free(arena_t* arena);
void* arena_alloc(arena_t* arena, size_t size);
void* arena_realloc(arena_t* arena, void* ptr, size_t size);

#endif