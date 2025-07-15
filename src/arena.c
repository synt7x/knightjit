#include "arena.h"
#include "debug.h"

arena_t* arena_create(size_t initial_capacity) {
    arena_t* arena = malloc(sizeof(arena_t));
    if (!arena) panic("Failed to allocate memory for arena");

    arena->size = 0;
    arena->capacity = initial_capacity;
    arena->data = malloc(initial_capacity);

    if (!arena->data) {
        free(arena);
        panic("Failed to allocate memory for arena");
    }

    return arena;
}

void arena_free(arena_t* arena) {
    if (!arena) return;

    free(arena->data);
    free(arena);
}

void* arena_alloc(arena_t* arena, size_t size) {
    if (arena->size + size > arena->capacity) {
        size_t cap;

        do {
            cap = arena->capacity ? arena->capacity * 2 : 64;
        } while (cap < arena->size + size);


        void* data = realloc(arena->data, cap);

        if (!data) {
            panic("Failed to allocate memory in arena");
        }

        arena->data = data;
        arena->capacity = cap;
    }

    void* ptr = (char*)arena->data + arena->size;
    arena->size += size;

    return ptr;
}