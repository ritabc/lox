//
// Created by Rita Bennett-Chew on 2/7/24.
//

#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdio.h>

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "vm.h" // Problematic? object includes vm.h

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

// is it safe to "downcast" an Obj* to an ObjString* ?
#define IS_STRING(value)    isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)  isObjType(value, OBJ_FUNCTION)

#define AS_STRING(value)   ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjFunction {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
};
typedef struct ObjFunction ObjFunction;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash; // cache the hash, so we don't have to recompute it - works because ObjStrings are immutable
};

ObjFunction* newFunction(VM* vm);
ObjString* takeString(VM* vm, char* chars, int length);
ObjString* copyString(VM* vm, const char* chars, int length);

void printObject(Value value, FILE* fd);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJECT_H
