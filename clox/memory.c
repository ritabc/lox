//
// Created by Rita Bennett-Chew on 1/31/24.
//

#include <stdlib.h>

#include "memory.h"
#include "vm.h"

/*
 * Reallocates any block stored at pointer.
 * Converts the block from oldSize bytes large to newSize bytes large.
 * Returns a pointer to new block.
 *
 * Handles 4 cases:
 * 1. 0       -> nonZero        : allocate a new block
 * 2. nonZero -> 0              : free allocation
 * 3. nonZero -> smaller block  : shrink existing allocation
 * 4. nonZero -> larger block   : grow existing allocation
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
   if (newSize == 0) {
       free(pointer);
       return NULL;
   }

   void* result = realloc(pointer, newSize);

   // fail if there isn't enough memory
   if (result == NULL) exit(1);

   return result;
}

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length+1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects(VM* vm) {
    Obj* object = vm->objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}