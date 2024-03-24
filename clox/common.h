//
// Created by Rita Bennett-Chew on 1/31/24.
//

#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NAN_BOXING

// when defined, use "debug" module to print out the chunk's bytecode
//#define DEBUG_PRINT_CODE

// when defined, our VM will disassemble (debug) each instruction before it's run
//#define DEBUG_TRACE_EXECUTION

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

#endif //CLOX_COMMON_H
