//
// Created by Rita Bennett-Chew on 2/7/24.
//

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    // insert self at head of linked list of objects
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

// create a new ObjString on the heap, initialize its fields
// kinda like an OOP constructor, so first calls 'base class' constructor to init Obj state
static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

// public facing function that assumes it CANNOT take ownership of the chars passed in,
// and thus it conservatively makes a copy of the chars
ObjString* copyString(const char* chars, int length) {
    char* heapChars = ALLOCATE(char, length+1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}

// public facing function used for concatenation,
// for concatting, we've already dynamically allocated a char array on the heap
// of the summed size,
// so takeString claims ownership of the string you give it
ObjString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}
