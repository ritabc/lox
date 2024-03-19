//
// Created by Rita Bennett-Chew on 1/31/24.
//

#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

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
void* reallocate(VM* vm, void* pointer, size_t oldSize, size_t newSize) {
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage(vm);
#endif
    }
   if (newSize == 0) {
       free(pointer);
       return NULL;
   }

   void* result = realloc(pointer, newSize);

   // fail if there isn't enough memory
   if (result == NULL) exit(1);

   return result;
}

void markObject(Obj* object) {
    if (object == NULL) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object), stdout);
    printf("\n");
#endif
    object->isMarked = true;
}

// GC doesn't need to mark numbers, bools, nil as they require no heap allocation
void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void freeObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*) object;
            FREE_ARRAY(vm, ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(vm, ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(vm, &function->chunk);
            FREE(vm, ObjFunction, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(vm, ObjNative, object);
            break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(vm, char, string->chars, string->length+1);
            FREE(vm, ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(vm, ObjUpvalue, object);
            break;
    }
}

static void markRoots(VM* vm) {
    // locals
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
        markValue(*slot);
    }

    // closures and upvalues
    for (int i = 0; i < vm->frameCount; i++) {
        markObject((Obj*)vm->frames[i].closure);
    }
    for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    // globals
    markTable(&vm->globals);

    // compiler has/uses roots too
    markCompilerRoots();
}

void collectGarbage(VM* vm) {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

    markRoots(vm);

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

void freeObjects(VM* vm) {
    Obj* object = vm->objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(vm, object);
        object = next;
    }
}