//
// Created by Rita Bennett-Chew on 2/7/24.
//

#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdio.h>

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

// is it safe to "downcast" an Obj* to an ObjString* ?
#define IS_STRING(value)    isObjType(value, OBJ_STRING)

#define AS_STRING(value)   ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
};

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJECT_H