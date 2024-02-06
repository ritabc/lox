//
// Created by Rita Bennett-Chew on 1/31/24.
//

#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// when defined, use "debug" module to print out the chunk's bytecode
#define DEBUG_PRINT_CODE

// when defined, our VM will disassemble (debug) each instruction before it's run
#define DEBUG_TRACE_EXECUTION

#endif //CLOX_COMMON_H
