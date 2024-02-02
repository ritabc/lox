//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;

    uint8_t* ip; // instruction pointer or program counter. Points to instruction about to be executed

    Value stack[STACK_MAX];
    Value* stackTop; // ptr to stack element just past the element containing the top value on the stack, aka where the next value to be pushed will go
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif //CLOX_VM_H
