#include "arena.h"
#include "debug.h"

arena_t* arena_create(size_t initial_capacity) {
    arena_t* arena = malloc(sizeof(arena_t));
    if (!arena) panic("Failed to allocate memory for arena");

    arena->size = 0;
    arena->capacity = initial_capacity;
    arena->nodes = malloc(sizeof(void*) * initial_capacity);

    if (!arena->nodes) {
        free(arena);
        panic("Failed to allocate memory for arena");
    }

    return arena;
}

void arena_free(arena_t* arena) {
    if (!arena) return;

    for (size_t i = 0; i < arena->size; ++i) {
        if (!arena->nodes[i]) continue;
        free(arena->nodes[i]);
    }

    free(arena->nodes);
    free(arena);
}

void* arena_alloc(arena_t* arena, size_t size) {
    if (arena->size == arena->capacity) {
        size_t cap = arena->capacity * 2;
        void** nodes = realloc(arena->nodes, sizeof(void*) * cap);
        if (!nodes) panic("Failed to grow arena node array");
        
        arena->nodes = nodes;
        arena->capacity = cap;
    }

    void* node = malloc(size);
    if (!node) panic("Failed to allocate node in arena");

    arena->nodes[arena->size++] = node;
    return node;
}

void* arena_realloc(arena_t* arena, void* ptr, size_t size) {
    for (size_t i = 0; i < arena->size; ++i) {
        if (arena->nodes[i] == ptr) {
            void* allocated = realloc(ptr, size);
            if (!allocated) panic("Failed to reallocate node in arena");

            arena->nodes[i] = allocated;
            return allocated;
        }
    }
    
    panic("Pointer not found in arena");
}