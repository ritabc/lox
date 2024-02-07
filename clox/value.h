//
// Created by Rita Bennett-Chew on 1/31/24.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

// Check a Value's type
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)

// Takes a Value of the correct type
// unwrap it and return the corresponding raw C value
// NB. We must check the type first before calling these!
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// Take a C value of the appropriate type,
// returns a Value that has the correct type tag and contains the underlying value
#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL            ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);


#endif //CLOX_VALUE_H
