//
// Created by Rita Bennett-Chew on 2/7/24.
//

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, type, objectType) (type*)allocateObject(vm, sizeof(type), objectType)

static Obj* allocateObject(VM* vm, size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    // insert self at head of linked list of objects
    object->next = vm->objects;
    vm->objects = object;
    return object;
}

// create a new ObjString on the heap, initialize its fields
// kinda like an OOP constructor, so first calls 'base class' constructor to init Obj state
// only called for new strings (if they already exist in our vm.strings hash Set, allocateString won't have been called)
static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    // whenever we create a new unique string, add it to the table (more like a strings set - nil values)
    tableSet(&vm->strings, string, NIL_VAL);
    return string;
}

// Using FNV-1a hash function
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// public facing function that assumes it CANNOT take ownership of the chars passed in,
// and thus it conservatively makes a copy of the chars
ObjString* copyString(VM* vm, const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length+1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, heapChars, length, hash);
}

void printObject(Value value, FILE* fd) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            fprintf(fd, "%s", AS_CSTRING(value));
            break;
    }
}

// public facing function used for concatenation,
// for concatenating, we've already dynamically allocated a char array on the heap
// of the summed size,
// so takeString claims ownership of the string you give it
// meaning that if we actually already have the string's text in our vm.strings hash Set, we must free the string that was passed in (b/c we 'own' it)
ObjString* takeString(VM* vm, char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);

    if (interned != NULL) {
        FREE_ARRAY(char, chars, length+1);
        return interned;
    }

    return allocateString(vm, chars, length, hash);
}
