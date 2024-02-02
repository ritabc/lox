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
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

/* Stores line number data, as well as the chunk index offset at which the line number starts. For instance if lineNumber = 3 & offset = 6, that means line number 3 contains chunk[6], and so on until we reach say, Line with lineNumber 4, offset 10. Line 3 in this case will contain chunk[6..10] (exclusive of 10)
 */
typedef struct {
    int lineNumber;
    int offset;
} LineStart;

/* A chunk is a series of instructions, like a program from a file - that'll be loaded into 1 chunk. That chunk will be made of:
 * - codes: array of all the instructions it's comprised of
 * - lineStarts: an Array of run-length encoded Lines, which each contain a lineNumber and the chunk code's idx of where that line number begins. Length is equal to number of lines.
 * - constants is an array where all constant values are stored.
 * Functionality has been added to make it a dynamic array.
 */
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int lineStartsCount;
    int lineStartsCapacity;
    LineStart* lineStarts;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int lineNumber);
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H
