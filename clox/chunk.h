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

/* A chunk is a series of instructions, like a program from a file - that'll be loaded into 1 chunk. That chunk will be made of:
 * - codes: array of all the instructions it's comprised of
 * - lines: an array of integers with the same length as codes. 1-1 correlation: The line number of the 1st code is stored at the 1st element in the lines array.
 * - constants is an array where all constant values are stored.
 * Functionality has been added to make it a dynamic array.
 */
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
