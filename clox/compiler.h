//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "object.h"
#include "vm.h"

ObjFunction* compile(VM* vm, const char* source);
void markCompilerRoots();

#endif //CLOX_COMPILER_H
