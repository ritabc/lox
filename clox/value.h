//
// Created by Rita Bennett-Chew on 1/31/24.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <stdio.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct VM VM;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

// Check a Value's type
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

// Takes a Value of the correct type
// unwrap it and return the corresponding raw C value
// NB. We must check the type first before calling these!
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value)    ((value).as.obj)

// Take a C value of the appropriate type,
// returns a Value that has the correct type tag and contains the underlying value
#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL            ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)    ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(VM* vm, ValueArray* array, Value value);
void freeValueArray(VM* vm, ValueArray* array);
void printValue(Value value, FILE* fd);


#endif //CLOX_VALUE_H
