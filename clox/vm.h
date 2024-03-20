//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"
#include "value.h"


#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// Forward declarations
struct ObjFunction;
struct ObjClosure;
struct ObjUpvalue;

// A CallFrame represents a single ongoing function call
// A CallFrame for each live function invocation aka each call that hasn't been returned yet.
// Tracks where on the stack that function's locals are, where the caller should resume
typedef struct {
    // pointer to the function being called.
    // Used to look up constants, etc
    struct ObjClosure* closure;

    // the ip of the caller's CallFrame, for resuming after returning from this function (pointer to instruction about to be executed) TODO: verify
    uint8_t* ip;

    // points into the VM's value stack at the first slot this function can use
    Value* slots;
} CallFrame;

typedef struct VM {
    FILE* fout;
    FILE* ferr;

    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];

    // ptr to stack element just past the element containing the top value on the stack, aka where the next value to be pushed will go
    Value* stackTop;

    Table globals;

    // A table to store strings for string interning (deduplication, as in, not duplicated) purposes
    Table strings;

    ObjString* initString;

    struct ObjUpvalue* openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;

    // a linked list of heap-allocated Objs, for freeing purposes
    Obj* objects;

    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

//extern VM vm;

void initVM(VM* vm, FILE* fout, FILE* ferr);
void freeVM(VM* vm);
InterpretResult interpret(VM* vm, const char* source);
void push(VM* vm, Value value);
Value pop(VM* vm);

#endif //CLOX_VM_H
