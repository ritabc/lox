//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "object.h"

bool compile(const char* source, Chunk* chunk);

#endif //CLOX_COMPILER_H
