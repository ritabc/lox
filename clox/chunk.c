//
//

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(VM* vm, Chunk* chunk) {
    FREE_ARRAY(vm, uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(vm, int, chunk->lines, chunk->capacity);
    freeValueArray(vm, &chunk->constants);
    initChunk(chunk);
}

/*
 * Writes a byte to the chunk, expanding the chunk if necessary.
 * The byte written can be an opcode, or an operand (like a ValueArray constant aka an index to a constant value)
 * Takes pointer to the chunk, the byte to write, and the line number the bytecode (instruction) comes from
 */
void writeChunk(VM* vm, Chunk* chunk, uint8_t byte, int line) {
    // grow code capacity if necessary
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(vm, uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(vm, int, chunk->lines, oldCapacity, chunk->capacity);
    }

    // write byte to code
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

/*
 * A convenience method to add a new constant to the chunk('s ValueArray)
 * Return the index of the added constant
 */
int addConstant(VM* vm, Chunk* chunk, Value value) {
    push(vm, value);
    writeValueArray(vm, &chunk->constants, value);
    pop(vm);
    return chunk->constants.count - 1;
}