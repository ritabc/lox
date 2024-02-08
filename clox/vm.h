//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;

    uint8_t* ip; // instruction pointer or program counter. Points to instruction about to be executed

    Value stack[STACK_MAX];

    // ptr to stack element just past the element containing the top value on the stack, aka where the next value to be pushed will go
    Value* stackTop;

    // A table to store strings for string interning (deduplication, as in, not duplicated) purposes
    Table strings;

    // a linked list of heap-allocated Objs, for freeing purposes
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif //CLOX_VM_H
