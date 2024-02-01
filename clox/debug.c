//
// Created by Rita Bennett-Chew on 1/31/24.
//

#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

/*
 * Prints
 */
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset+1];
    printf("%-16s %4d '", name, constant); // print the constant index
    printValue(chunk->constants.values[constant]); // also print the constant value
    printf("'\n");
    return offset + 2;
}

/*
 * Prints the opcode name,
 * increments the offset by 1, then returns that int
 */
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/*
 * Dissasemble a single instruction at chunk's offset
 * Print human-readable string for the instruction
 * Since instructions can have different sizes,
 * Return offset of next instruction
 */
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if(offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | "); // same line as above
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
