//
// Created by Rita Bennett-Chew on 2/2/24.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

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
//VM vm; // commented out b/c not needed after refactor for testing

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}

static void resetStack(VM* vm) {
    vm->stackTop = vm->stack;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
}

static void runtimeError(VM* vm, const char* format, ...) {

    // First, print the error msg itself
    va_list args;
    va_start(args, format);
    vfprintf(vm->ferr, format, args);
    va_end(args);
    fputs("\n", vm->ferr);

    // Then, show the entire stack when a runtime error is generated
    // walk the call stack from the top aka the most recently called func
    // to the bottom (the top-level code)
    // for each, find the line number that corresponds to the current ip inside that frame's function
    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm->frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(vm->ferr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(vm->ferr, "script\n");
        } else {
            fprintf(vm->ferr, "%s()\n", function->name->chars);
        }
    }

    resetStack(vm);
}

static void defineNative(VM* vm, const char* name, NativeFn function) {
    push(vm, OBJ_VAL(copyString(vm, name, (int) strlen(name))));
    push(vm, OBJ_VAL(newNative(vm, function)));
    tableSet(vm, &vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
    pop(vm);
    pop(vm);
}

void initVM(VM* vm, FILE* fout, FILE* ferr) {
    vm->fout = fout;
    vm->ferr = ferr;
    resetStack(vm);
    vm->objects = NULL;
    vm->bytesAllocated = 0;
    vm->nextGC = 1024 * 1024;

    vm->grayCount = 0;
    vm->grayCapacity = 0;
    vm->grayStack = NULL;
    initTable(&vm->globals);
    initTable(&vm->strings);

    defineNative(vm, "clock", clockNative);
}

void freeVM(VM* vm) {
    freeTable(vm, &vm->globals);
    freeTable(vm, &vm->strings);
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

// Initializes the next CallFrame on the stack
// stores a pointer to the function being called
// points the frame's ip to the beginning of the functio's bytecode
// sets up slots pointer to give the frame its window into the stack,
// ensuring the arguments on the stack already line up with the function's parameters
static bool call(VM* vm, ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError(vm, "Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    // error on a too-deep call chain
    if (vm->frameCount == FRAMES_MAX) {
        runtimeError(vm, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm->stackTop - argCount - 1;
    return true;
}

static bool callValue(VM* vm, Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return call(vm, AS_CLOSURE(callee), argCount);
            case OBJ_FUNCTION:
                return call(vm, AS_FUNCTION(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm->stackTop - argCount);
                vm->stackTop -= argCount + 1;
                push(vm, result);
                return true;
            }
            default:
                break;
        }
    }
    // prevent user from calling non funcs, non classes like: true();
    runtimeError(vm, "Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(VM* vm, Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm->openUpvalues;

    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(vm, local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(VM* vm, Value* last) {
    while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
}


static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(VM* vm) {
    ObjString* b = AS_STRING(peek(vm, 0));
    ObjString* a = AS_STRING(peek(vm, 1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(vm, char, length+1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(vm, chars, length);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
}

static InterpretResult run(VM* vm) {
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
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
    } while (false) // in a do...while loop to capture all of it and for semicolrun(on wrangling reasons

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
    disassembleInstruction(&frame->closure->function->chunk, (int) (frame->ip - frame->closure->function->chunk.code));
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
            push(vm, frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            // Takes the assigned value from the top of the stack & stores it in the stack slot corresponding to the local variable
            // Does not pop the value since assignment is an expression. Expressions produce a value, which should be at the top of the stack after
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(vm, 0);
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
            tableSet(vm, &vm->globals, name, peek(vm, 0));
            pop(vm);
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (tableSet(vm, &vm->globals, name, peek(vm, 0))) {
                // it's a new key (hasn't been defined yet - it's a runtime error to try & assign to it)
                tableDelete(&vm->globals, name);
                runtimeError(vm,"Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(vm, *frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(vm, 0);
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
        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(vm, 0))) frame->ip += offset;
            break;
        }
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_CALL: {
            int argCount = READ_BYTE();
            if (!callValue(vm, peek(vm, argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm->frames[vm->frameCount - 1];
            break;
        }
        case OP_CLOSURE: {
            ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure* closure = newClosure(vm, function);
            push(vm,OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(vm, frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OP_CLOSE_UPVALUE:
            closeUpvalues(vm, vm->stackTop - 1);
            pop(vm);
            break;
        case OP_RETURN: {

            // When a function returns a value, that value will be on top of the stack.
            // Since we're about to discard the called function's entire stack window, pop the return value off & save it
            // Then, Discard the Callframe for the returning function
            // If that was the last CallFrame, we're done with the entire program
            // Otherwise, discard all the slots the callee was using for its params & local variables
            // Push the return value at the new location
            // Finally, update the run() function's cached pointer to the current frame.

            Value result = pop(vm);
            closeUpvalues(vm, frame->slots);
            vm->frameCount--;
            if (vm->frameCount == 0) {
                pop(vm);
                return INTERPRET_OK;
            }

            vm->stackTop = frame->slots;
            push(vm, result);
            frame = &vm->frames[vm->frameCount - 1];
            break;
        }
    }
}
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(VM* vm, const char* source) {
//    Chunk chunk;
//    initChunk(&chunk);
//
//    if (!compile(vm, source)) {
//        freeChunk(&chunk);
//        return INTERPRET_COMPILE_ERROR;
//    }
//
//    vm->chunk = &chunk;
//    vm->ip = vm->chunk->code;

    ObjFunction* function = compile(vm, source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(vm, OBJ_VAL(function));
    ObjClosure* closure = newClosure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    call(vm, closure, 0);

    return run(vm);
}