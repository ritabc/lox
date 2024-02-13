//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "object.h"
#include "vm.h"

bool compile(VM* vm, const char* source, Chunk* chunk);

#endif //CLOX_COMPILER_H
