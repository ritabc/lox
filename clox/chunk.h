/*
A chunk is a sequence of bytecode
 */

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

// An Operation Code, or opcode, is 1 byte.
// It tells us what kind of instruction we're working with
// Each instruction will have an opcode
typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

// A chunk is a series of instructions
// for now, it's just a wrapper around an array of bytes. // TODO: keep comment updated
// Functionality has been added to make it a dynamic array.
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
