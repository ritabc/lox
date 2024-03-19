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
    Obj* object = (Obj*)reallocate(vm,NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    // insert self at head of linked list of objects
    object->next = vm->objects;
    vm->objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif
    return object;
}

ObjClass* newClass(VM* vm, ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
    klass->name = name;
    return klass;
}

ObjClosure* newClosure(VM* vm, ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(vm, ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        // ensure the memory manager never sees uninit'ed memory
        upvalues[i] = NULL;
    }
    ObjClosure* closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction(VM* vm) {
    ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
    // set up with blank state, fill it in later after the function is created
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

// Constructor for native function
// Takes a C function pointer to wrap in an ObjNative
ObjNative* newNative(VM* vm, NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// create a new ObjString on the heap, initialize its fields
// kinda like an OOP constructor, so first calls 'base class' constructor to init Obj state
// only called for new strings (if they already exist in our vm.strings hash Set, allocateString won't have been called)
static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    // push & pop ensures the string is safe while the table is being resized
    push(vm, OBJ_VAL(string));
    // whenever we create a new unique string, add it to the table (more like a strings set - nil values)
    tableSet(vm, &vm->strings, string, NIL_VAL);
    pop(vm);
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

    char* heapChars = ALLOCATE(vm, char, length+1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, heapChars, length, hash);
}

ObjUpvalue * newUpvalue(VM* vm, Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void printFunction(ObjFunction* function, FILE* fd) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    fprintf(fd, "<fn %s>", function->name->chars);
}

void printObject(Value value, FILE* fd) {
    switch (OBJ_TYPE(value)) {
        case OBJ_CLASS:
            fprintf(fd, "%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function, fd);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value), fd);
            break;
        case OBJ_NATIVE:
            fprintf(fd, "<native fn>");
            break;
        case OBJ_STRING:
            fprintf(fd, "%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            fprintf(fd, "upvalue");
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
        FREE_ARRAY(vm, char, chars, length+1);
        return interned;
    }

    return allocateString(vm, chars, length, hash);
}
