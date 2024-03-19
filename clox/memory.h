//
//

#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(vm, type, count) (type*)reallocate(vm, NULL, 0, sizeof(type) * (count))

// using reallocate here instead of free() helps the VM track how much memory is still being used
#define FREE(vm, type, pointer) reallocate(vm, pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

/*
 * Pretties up and generalizes reallocate
 * - gets the size of the array's element type
 * - casts the resulting void* back to a pointer of the correct type
 */
#define GROW_ARRAY(vm, type, pointer, oldCount, newCount) \
    (type*)reallocate(vm, pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(vm, type, pointer, oldCount) \
    reallocate(vm, pointer, sizeof(type) * (oldCount), 0)

void* reallocate(VM* vm, void* pointer, size_t oldSize, size_t newSize);
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage(VM* vm);

void freeObjects(VM* vm);

#endif //CLOX_MEMORY_H
