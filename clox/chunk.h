/*
A chunk is a sequence of bytecode
 */

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef struct VM VM;

// An Operation Code, or opcode, is 1 byte.
// It tells us what kind of instruction we're working with
// Each instruction will have an opcode
typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
} OpCode;

/* A chunk is a series of instructions, like a program from a file - that'll be loaded into 1 chunk. That chunk will be made of:
 * - codes: array of all the instructions it's comprised of
 * - constants is an array where all constant values are stored.
 * Functionality has been added to make it a dynamic array.
 */
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(VM* vm, Chunk* chunk);
void writeChunk(VM* vm, Chunk* chunk, uint8_t byte, int lineNumber);
int addConstant(VM* vm, Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
