//
// Created by Rita Bennett-Chew on 1/31/24.
//


#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(VM* vm, ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(vm, Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(vm, Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value, FILE* fd) {
    switch (value.type) {
        case VAL_BOOL:
            fprintf(fd, AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL: fprintf(fd, "nil"); break;
        case VAL_NUMBER: fprintf(fd, "%g", AS_NUMBER(value)); break;
        case VAL_OBJ: printObject(value, fd); break;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:       return true;
        case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b);
        default:            return false; // Unreachable
    }
}