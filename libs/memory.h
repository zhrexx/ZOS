#ifndef MEMORY_H
#define MEMORY_H
#include "types.h" 

#define ARENA_CAPACITY 1024 * 1024 * 2

typedef struct {
    char buffer[ARENA_CAPACITY];
    size_t size;
} AArena;

static AArena arena = {0};

void *aarena_alloc(AArena *arena, size_t size) {
    if (arena->size + size > ARENA_CAPACITY) {
        return NULL;
    }
    void *ptr = &arena->buffer[arena->size];
    arena->size += size;
    return ptr;
}

void aarena_reset(AArena *arena) {
    arena->size = 0;
}


#endif // MEMORY_H
