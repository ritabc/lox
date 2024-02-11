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
 * Prints info on the constant: the index in chunk->constants.values and the value itself
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

// called with getting & setting a local. Unfortunately we won't give the local variable's name to the disassembler, which wouldn't be ideal if we implemented a debugger
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset+1];
    printf("%-16s %d\n", name, slot);
    return offset + 2;
}

/* getLine takes a chunk pointer and an offset, and looks up in the chunk's lineStarts which line that code is on
 * // TODO: could convert this to binary search, but b/c we only use it when there's an error, it's also fine as is
 */
int getLineNumber(Chunk* chunk, int offset) {
    for (int i = 0; i < chunk->lineStartsCount-1; i++) {
        LineStart currStart = chunk->lineStarts[i];
        LineStart nextStart = chunk->lineStarts[i+1];
        if (offset >= currStart.offset && offset < nextStart.offset) {
            return currStart.lineNumber;
        }
    }
    // if we get here, offset is on last line
    return chunk->lineStarts[chunk->lineStartsCount-1].lineNumber;
}

/*
 * Dissasemble a single instruction at chunk's offset
 * Print human-readable string for the instruction
 * Since instructions can have different sizes,
 * Return offset of next instruction
 */
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if(offset > 0 && getLineNumber(chunk, offset) == getLineNumber(chunk, offset - 1)) {
        printf("   | "); // same line as above
    } else {
        printf("%4d ", getLineNumber(chunk, offset));
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
