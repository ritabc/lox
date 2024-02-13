//
// Created by Rita Bennett-Chew on 2/2/24.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

// This should ideally be a pointer that's passed around
// So the host app can control when and where the VM is allocated,
// and run multiple VMs in parallel.
// But in the interest of keeping things small for the book,
// it's a global variable
//VM vm;

static void resetStack(VM* vm) {
    vm->stackTop = vm->stack;
}

static void runtimeError(VM* vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(vm->ferr, format, args);
    va_end(args);
    fputs("\n", vm->ferr);

    size_t instruction = vm->ip - vm->chunk->code - 1;
    int line = vm->chunk->lineStarts[instruction].lineNumber;
    fprintf(vm->ferr, "[line %d] in script\n", line);
    resetStack(vm);
}

void initVM(VM* vm, FILE* fout, FILE* ferr) {
    vm->fout = fout;
    vm->ferr = ferr;
    resetStack(vm);
    vm->objects = NULL;
    initTable(&vm->globals);
    initTable(&vm->strings);
}

void freeVM(VM* vm) {
    freeTable(&vm->globals);
    freeTable(&vm->strings);
    freeObjects(vm);
}

void push(VM* vm, Value value) {
    *vm->stackTop = value;
    vm->stackTop++;
}

Value pop(VM* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

// returns a Value from the stack
// without pop-ing it
// a distance of 0 => the top of the stack
// a distance of 1 => 1 slot down
// etc
static Value peek(VM* vm, int distance) {
    return vm->stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(VM* vm) {
    ObjString* b = AS_STRING(pop(vm));
    ObjString* a = AS_STRING(pop(vm));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length+1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(vm, chars, length);
    push(vm, OBJ_VAL(result));
}

static InterpretResult run(VM* vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                               \
    do {                                                       \
        if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {      \
            runtimeError(vm, "Operands must be numbers.");         \
            return INTERPRET_RUNTIME_ERROR;                    \
        }                                                      \
        double b = AS_NUMBER(pop(vm));                           \
        double a = AS_NUMBER(pop(vm));                           \
        push(vm, valueType(a op b));                               \
    } while (false) // in a do...while loop to capture all of it and for semicolon wrangling reasons

for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    // show current contents of the stack
    fprintf(vm->fout, "          ");
    for (Value *slot = vm->stack; slot < vm->stackTop; slot++) {
        fprintf(vm->fout, "[ ");
        printValue(*slot, vm->fout);
        fprintf(vm->fout, " ]");
    }
    fprintf(vm->fout, "\n");

    // print disassembled instruction
    disassembleInstruction(vm->chunk, (int) (vm->ip - vm->chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(vm, constant);
            break;
        }
        case OP_NIL:
            push(vm, NIL_VAL);
            break;
        case OP_TRUE:
            push(vm, BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(vm, BOOL_VAL(false));
            break;
        case OP_POP: pop(vm); break;
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(vm, vm->stack[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            // Takes the assigned value from the top of the stack & stores it in the stack slot corresponding to the local variable
            // Does not pop the value since assignment is an expression. Expressions produce a value, which should be at the top of the stack after
            uint8_t slot = READ_BYTE();
            vm->stack[slot] = peek(vm, 0);
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString* name = READ_STRING();
            Value value;
            if (!tableGet(&vm->globals, name, &value)) {
                runtimeError(vm,"Undefinied variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(vm, value);
            break;
        }
        case OP_DEFINE_GLOBAL: {
            ObjString* name = READ_STRING();
            tableSet(&vm->globals, name, peek(vm, 0));
            pop(vm);
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (tableSet(&vm->globals, name, peek(vm, 0))) {
                // it's a new key (hasn't been defined yet - it's a runtime error to try & assign to it)
                tableDelete(&vm->globals, name);
                runtimeError(vm,"Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_EQUAL: {
            Value b = pop(vm);
            Value a = pop(vm);
            push(vm, BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD: {
            if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1))) {
                concatenate(vm);
            } else if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 1))) {
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(a + b));
            } else {
                runtimeError(vm, "Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(vm, BOOL_VAL(isFalsey(pop(vm))));
            break;
        case OP_NEGATE:
            if (IS_NUMBER(peek(vm, 0))) {
                runtimeError(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
            break;
        case OP_PRINT: {
            printValue(pop(vm), vm->fout);
            fprintf(vm->fout, "\n");
            break;
        }
        case OP_RETURN: {
            // Exit interpreter
            return INTERPRET_OK;
        }
    }
}
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(VM* vm, const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(vm, source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);

    freeChunk(&chunk);
    return result;
}