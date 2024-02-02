//
//

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lineStartsCount = 0;
    chunk->lineStartsCapacity = 0;
    chunk->lineStarts = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineStart, chunk->lineStarts, chunk->lineStartsCapacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/*
 * Writes a byte to the chunk, expanding the chunk if necessary.
 * The byte written can be an opcode, or an operand (like a ValueArray constant aka an index to a constant value)
 * Takes pointer to the chunk, the byte to write, and the line number the bytecode (instruction) comes from
 */
void writeChunk(Chunk* chunk, uint8_t byte, int lineNumber) {
    // grow code capacity if necessary
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    // write byte to code
    chunk->code[chunk->count] = byte;
    chunk->count++;

    // Next, handle line info:
    // if this is a new line (including the first line), we must add to lines
    // - to add to lines, does lines have enough capacity?
    // -- If so, just add lineNumber & offset (chunk's count BEFORE (since the offset is index-based, not count-based) it's incremented)

    // If the byte came from a lineNumber that's the same as the last line number, do nothing with chunk->lines.
    if (chunk->lineStartsCount > 0 && lineNumber == chunk->lineStarts[chunk->lineStartsCount-1].lineNumber) {
        return;
    }

    // if here, we'll need to add new LineStart
    // grow lineStarts capacity if necessary
    if (chunk->lineStartsCapacity < chunk->lineStartsCount + 1) {
        int oldCapacity = chunk->lineStartsCapacity;
        chunk->lineStartsCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lineStarts = GROW_ARRAY(LineStart, chunk->lineStarts, oldCapacity, chunk->capacity);
    }

    // add new LineStart
    LineStart* lineStart = &(chunk->lineStarts[chunk->lineStartsCount]);
    chunk->lineStartsCount++;
    lineStart->offset = chunk->count - 1;
    lineStart->lineNumber = lineNumber;
}

/*
 * A convenience method to add a new constant to the chunk('s ValueArray)
 * Return the index of the added constant
 */
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}