//
// Created by Rita Bennett-Chew on 2/2/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "common.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError; // did any errors occur during compilation?

    // if in panicMode, suppress any other errors that get detected, but keep compiling
    // panic mode ends when parser reaches a synchronization point (a statement boundary in Lox)
    bool panicMode;
} Parser;

// ie, a call has a higher precedence than a unary
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(VM* vm, bool canAssign);

/* In the rules table,
 * each row is a ParseRule.
 * We can look up a row by TOKEN_TYPE.
 * Each TOKEN_TYPE, `type`, corresponds to a ParseRule (row):
 *  - prefix: a function used to parse/compile a prefix expression which starts with a token of type `type`
 *  - infix: a function used to parse/compile an infix expression which starts with a left operand, then has a token of type `type`
 *  - precedence: of an infix expr that'll use that token as an operator.
 */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;

    // the scope depth of the block where the local var was declared
    // 0 - global scope
    // highest - the innermost scope
    int depth;

    // Tells if a given local is captured by a closure
    // true if the local is captured by any later neested func declaration
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index; // the local slot the upvalue captures
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
    // Create a linked list of functions enclosing each Compiler
    struct Compiler* enclosing;

    // a reference to the function object being built
    ObjFunction* function;
    // type tells the compiler when it's compiling top-level code vs. a func body
    FunctionType type;

    // keeps track of which stack slots are associated with which local variables or temps
    Local locals[UINT8_COUNT];

    // how many locals are in scope
    // aka how many of the array slots of locals are in use
    int localCount;

    // Use upvalues for implementing closures
    Upvalue upvalues[UINT8_COUNT];

    // the number of blocks surrounding the current bit of code we're compiling
    int scopeDepth;

    VM* vm;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler* current = NULL; // this global variable isn't the best solution, especially if we were running in a multi-threaded app with multiple compilers running in parallel
ClassCompiler* currentClass = NULL;

// returns the address of the chunk owned by the function we're compiling atm
static Chunk* currentChunk() {
    return &current->function->chunk;
}

static void errorAt(VM* vm, Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(vm->ferr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(vm->ferr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(vm->ferr, " at '%.*s'", token->length, token->start);
    }

    fprintf(vm->ferr, ": %s\n", message);
    parser.hadError = true;
}

static void error(VM* vm, const char* message) {
    errorAt(vm, &parser.previous, message);
}

static void errorAtCurrent(VM* vm, const char* message) {
    errorAt(vm, &parser.current, message);
}

static void advance(VM* vm) {
    parser.previous = parser.current;

    // keep looping and reporting errors
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(vm, parser.current.start);
    }
}

// advances, but also validates whether the token has an expected type
static void consume(VM* vm, TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance(vm);
        return;
    }

    errorAtCurrent(vm, message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(VM* vm, TokenType type) {
    if (!check(type)) return false;
    advance(vm);
    return true;
}

static void emitByte(VM* vm, uint8_t byte) {
    writeChunk(vm, currentChunk(), byte, parser.previous.line);
}

static void emitBytes(VM* vm, uint8_t byte1, uint8_t byte2) {
    emitByte(vm, byte1);
    emitByte(vm, byte2);
}

static void emitLoop(VM* vm, int loopStart) {
    emitByte(vm, OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error(vm, "Loop body too large.");

    emitByte(vm, (offset >> 8) & 0xff);
    emitByte(vm, offset & 0xff);
}

static int emitJump(VM* vm, uint8_t instruction) {
    emitByte(vm, instruction); // we'll return the offset of this instr
    emitByte(vm, 0xff);
    emitByte(vm, 0xff);
    return currentChunk()->count-2;
}

static void emitReturn(VM* vm) {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(vm, OP_GET_LOCAL, 0);
    } else {
        emitByte(vm, OP_NIL);
    }
    emitByte(vm, OP_RETURN);
}

static uint8_t makeConstant(VM* vm, Value value) {
    int constant = addConstant(vm, currentChunk(), value);
    if (constant > UINT8_MAX) {
        error(vm, "Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(VM* vm, Value value) {
    emitBytes(vm, OP_CONSTANT, makeConstant(vm, value));
}

static void patchJump(VM* vm, int offset) {
    // -2 to adjust for the bytecode for the jump offset itself
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error(vm, "Too much code to jump over.");
    }

    // replaces the temp op code bytes with the jumped amount
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset+1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler, FunctionType type, VM* vm) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction(vm);
    compiler->vm = vm;
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(vm, parser.previous.start, parser.previous.length);
    }

    // compiler implicitly claims stack slot zero for the VM's own internal use
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction* endCompiler(VM* vm) {
    emitReturn(vm);
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

// called before/after compiling the body of a block
static void beginScope() {
    current->scopeDepth++;
}
static void endScope(VM* vm) {
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount-1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(vm, OP_CLOSE_UPVALUE);
        } else {
            emitByte(vm, OP_POP);
        }
        current->localCount--;
    }
}


static void expression(VM* vm);
static void statement(VM* vm);
static void declaration(VM* vm);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(VM* vm, Precedence precedence);

static uint8_t identifierConstant(VM* vm, Token* name) {
    return makeConstant(vm, OBJ_VAL(copyString(current->vm, name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(VM* vm, Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                /* prevent the following:
                    { var a = "outer";
                        { var a = a; }}
                 */
                error(vm, "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

// Upvalues used for closures
static int addUpvalue(VM* vm, Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    // reuse upvalue index if we find an upvalue in the array whose slot index matches the one we're adding
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error(vm, "Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// looks for a local variable with name declared in any of the surrounding functions.
// If found, it returns an upvalue index for that variable
// if not, returns -1
// only called when we know the variable isn't in the current compiler
static int resolveUpvalue(VM* vm, Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(vm, compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(vm, compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(vm, compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(vm, compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(VM* vm, Token name) {
    if (current->localCount == UINT8_COUNT) {
        error(vm, "Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // indicate the variable is 'declared' but 'uninitialized'
    local->isCaptured = false;
}

// the function which tells the compiler to record the existence of the variable
// only do this for locals, since global variables are late bound
static void declareVariable(VM* vm) {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;

    // disallow declaring multiple vars in same scope w/same name. Shadowing (same name, different scope) is okay
    // work backwards, since new vars are declared at the end of the array.
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(vm, "Already a variable with this name in this scope.");
        }
    }

    addLocal(vm, *name);
}

// consumes the identifier token for the variable name
// if global: adds its lexeme to the chunk's constant table
// if global: returns the constant table index where it was added
// if local: returns 0 (dummy table index)
static uint8_t parseVariable(VM* vm, const char* errorMessage) {
    consume(vm, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(vm);

    // if the variable is local, exit.
    // We don't store local variable identifiers in the constant table
    if (current->scopeDepth > 0) return 0;
    return identifierConstant(vm, &parser.previous);
}

// mark the local as avaiable to use (depth was currently -1)
static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount-1].depth = current->scopeDepth;
}

// called when declaring a variable (by varDeclaration()) at runtime
// this function is called at runtime
// the VM's state has already init'ed the variable (with the var's initializer or the implicit nil
// the value sitting on top of the stack is the temporary, and "becomes" the local variable
static void defineVariable(VM* vm, uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        // at runtime, don't create a local variable
        return;
    }

    emitBytes(vm, OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList(VM* vm) {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            // Each of the following arg expressions generates code that leavees its value on the stack in preparation for the call
            expression(vm);
            if (argCount == 255) {
                error(vm, "Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(vm, TOKEN_COMMA));
    }
    consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

// when called, LHS of expression has already been compiled. AT runtime, its value will be on stack top. If it's falsey, we skip the right operand and LHS value is result of entire expression. If it's not falsey, discard the LHS value, evaluate the right operand
static void and_(VM* vm, bool canAssign) {
    int endJump = emitJump(vm, OP_JUMP_IF_FALSE);
    emitByte(vm, OP_POP);
    parsePrecedence(vm, PREC_AND);

    patchJump(vm, endJump);
}

// assume the initial '(' has already been consumed
// grouping is purely syntactic and has no runtime semantics and doesn't emit any bytecode on its own
static void grouping(VM* vm, bool canAssign) {
    expression(vm);
    consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(VM* vm, bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(vm, NUMBER_VAL(value));
}

// when LHS if falsey, do tiny jump to next statement, which is unconditional jump over the code for the right operand.
static void or_(VM* vm, bool canAssign) {
    int elseJump = emitJump(vm, OP_JUMP_IF_FALSE);
    int endJump = emitJump(vm, OP_JUMP);

    patchJump(vm, elseJump);
    emitByte(vm, OP_POP);

    parsePrecedence(vm, PREC_OR);
    patchJump(vm, endJump);
}

// +1, -2 trims the quotation marks
static void string(VM* vm, bool canAssign) {
    emitConstant(vm, OBJ_VAL(copyString(current->vm, parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(VM* vm, Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(vm, current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(vm, current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(vm, &name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(vm, TOKEN_EQUAL)) {
        expression(vm);
        emitBytes(vm, setOp, (uint8_t)arg);
    } else {
        emitBytes(vm, getOp, (uint8_t)arg);
    }
}

static void variable(VM* vm, bool canAssign) {
    namedVariable(vm, parser.previous, canAssign);
}

static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void super_(VM* vm, bool canAssign) {
    if (currentClass == NULL) {
        error(vm, "Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error(vm, "Can't use 'super' in a class with no superclass.");
    }

    consume(vm, TOKEN_DOT, "Expect '.' after 'super'.");
    consume(vm, TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(vm, &parser.previous);

    namedVariable(vm, syntheticToken("this"), false);
    if (match(vm, TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList(vm);
        namedVariable(vm, syntheticToken("super"), false);
        emitBytes(vm, OP_SUPER_INVOKE, name);
        emitByte(vm, argCount);
    } else {
        namedVariable(vm, syntheticToken("super"), false);
        emitBytes(vm, OP_GET_SUPER, name);
    }
}

// treat `this` as a lexically scoped local variable whose value gets magically initialized
static void this_(VM* vm, bool canAssign) {
    if (currentClass == NULL) {
        error(vm, "Can't use 'this' outside of a class.");
        return;
    }

    variable(vm, false);
}

// The left hand operator has already been compiled,
// and the infix operator has already been consumed
static void binary(VM* vm, bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence(vm, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:      emitBytes(vm,OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:     emitByte(vm, OP_EQUAL); break;
        case TOKEN_GREATER:         emitByte(vm, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emitBytes(vm,OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emitByte(vm, OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emitBytes(vm,OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:            emitByte(vm, OP_ADD); break;
        case TOKEN_MINUS:           emitByte(vm, OP_SUBTRACT); break;
        case TOKEN_STAR:            emitByte(vm, OP_MULTIPLY); break;
        case TOKEN_SLASH:           emitByte(vm, OP_DIVIDE); break;
        default: return; // unreachable
    }
}

static void call(VM* vm, bool canAssign) {
    uint8_t argCount = argumentList(vm);
    emitBytes(vm, OP_CALL, argCount);
}

static void dot(VM* vm, bool canAssign) {
    consume(vm, TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(vm, &parser.previous);

    if (canAssign && match(vm, TOKEN_EQUAL)) {
        expression(vm);
        emitBytes(vm, OP_SET_PROPERTY, name);
    } else if (match(vm, TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList(vm);
        emitBytes(vm, OP_INVOKE, name);
        emitByte(vm, argCount);
    } else {
        emitBytes(vm, OP_GET_PROPERTY, name);
    }
}

static void literal(VM* vm, bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(vm, OP_FALSE); break;
        case TOKEN_NIL: emitByte(vm, OP_NIL); break;
        case TOKEN_TRUE: emitByte(vm, OP_TRUE); break;
        default: return; // Unreachable
    }
}

// evaluate the operator first, then encounter the OP_NEGATE instruction,
// at that point, pop the value, negate it, and push the result
static void unary(VM* vm, bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    parsePrecedence(vm, PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType) {
        case TOKEN_BANG: emitByte(vm, OP_NOT); break;
        case TOKEN_MINUS: emitByte(vm, OP_NEGATE); break;
        default: return;
    }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

// Parse until we get to a precedence lower than the given precedence
static void parsePrecedence(VM* vm, Precedence precedence) {
    advance(vm);
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error(vm, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(vm, canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance(vm);
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(vm, canAssign);
    }

    if (canAssign && match(vm, TOKEN_EQUAL)) {
        error(vm, "Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression(VM* vm) {
    parsePrecedence(vm, PREC_ASSIGNMENT);
}

static void block(VM* vm) {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration(vm);
    }

    consume(vm, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(VM* vm, FunctionType type) {
    // Create a separate Compiler for each function being compiled.
    Compiler compiler;
    initCompiler(&compiler, type, vm);
    beginScope();

    consume(vm, TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    // handle Parameters, which are local variables declared in the outermost lexical scope of the func body
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent(vm, "Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable(vm, "Expect parameter name.");
            defineVariable(vm, constant);
        } while (match(vm, TOKEN_COMMA));
    }

    consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(vm, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(vm);

    // After reaching the end of the func's block body,
    // call endCompiler, which yields the newly compiled function object
    // which is stored as a constant in the surrounding func's constant table
    ObjFunction* function = endCompiler(vm);
    emitBytes(vm, OP_CLOSURE, makeConstant(vm, OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(vm, compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(vm, compiler.upvalues[i].index);
    }
}

static void method(VM* vm) {
    // to define a new method, the VM needs 3 things:
    // 1. method name
    // 2. method body closure
    // 3. class to bind the method to
    consume(vm, TOKEN_IDENTIFIER, "Expect method name.");
    Token className = parser.previous;

    uint8_t constant = identifierConstant(vm, &parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(vm, type);


    emitBytes(vm,OP_METHOD, constant);
}

static void classDeclaration(VM* vm) {
    consume(vm, TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(vm, &parser.previous);
    declareVariable(vm);

    emitBytes(vm, OP_CLASS, nameConstant);
    defineVariable(vm, nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (match(vm, TOKEN_LESS)) {
        consume(vm, TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(vm, false);

        if (identifiersEqual(&className, &parser.previous)) {
            error(vm, "A class can't inherit from itself.");
        }

        beginScope();
        addLocal(vm, syntheticToken("super"));
        defineVariable(vm, 0);

        namedVariable(vm, className, false);
        emitByte(vm, OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(vm, className, false);
    consume(vm, TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method(vm);
    }
    consume(vm, TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(vm, OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope(vm);
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration(VM* vm) {
    uint8_t global = parseVariable(vm, "Expect function name.");
    markInitialized();
    function(vm, TYPE_FUNCTION);
    defineVariable(vm, global);
}

static void varDeclaration(VM* vm) {
    uint8_t global = parseVariable(vm, "Expect variable name.");

    if (match(vm, TOKEN_EQUAL)) {
        expression(vm);
    } else {
        // if user doesn't initialize the var, do it for them, init-ing it to nil.
        emitByte(vm, OP_NIL);
    }
    consume(vm, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(vm, global);
}

static void expressionStatement(VM* vm) {
    expression(vm);
    consume(vm, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(vm, OP_POP);
}

static void forStatement(VM* vm) {
    beginScope(); // necessary for var declared in for loop - that var should be scoped to the loop body
    consume(vm, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(vm, TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(vm, TOKEN_VAR)) {
        varDeclaration(vm);
    } else {
        expressionStatement(vm);
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(vm, TOKEN_SEMICOLON)) {
        expression(vm);
        consume(vm, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(vm, OP_JUMP_IF_FALSE);
        emitByte(vm, OP_POP); // Condition
    }

    // Since we only make a single pass, we compile the increment clause after we encounter it.
    // We'll jump over the increment, run the body, jump back up to the increment, run it, then go to the next iteration
    if (!match(vm, TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(vm, OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression(vm);
        emitByte(vm, OP_POP);
        consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(vm, loopStart);
        loopStart = incrementStart;
        patchJump(vm, bodyJump);
    }


    statement(vm);
    emitLoop(vm, loopStart);

    if (exitJump != -1) {
        patchJump(vm, exitJump);
        emitByte(vm, OP_POP); // Condition
    }

    endScope(vm);
}

static void ifStatement(VM* vm) {
    consume(vm, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    // Put condition expression on top of stack,
    // so we can use it to determine whether to execute the then branch or skip it, at runtime
    expression(vm);
    consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // Use backpatching: emit the half finished jump instruction 1st with a placeholder offset operand (keep track of where that is)
    // emitJump emits the given instruction, then 2 placeholder byte instructions
    int thenJump = emitJump(vm, OP_JUMP_IF_FALSE);

    // when the condition is truthy, pop it right before the code inside the then branch
    emitByte(vm, OP_POP);
    statement(vm);

    int elseJump = emitJump(vm, OP_JUMP);

    // Then, after compiling the then body, we'll know how far to jump, so replace it with the complete instruction
    patchJump(vm, thenJump);

    // if the condition is falsey, pop at the beginning of the else branch
    emitByte(vm, OP_POP);

    if (match(vm, TOKEN_ELSE)) statement(vm);
    patchJump(vm, elseJump);
}

static void printStatement(VM* vm) {
    expression(vm);
    consume(vm, TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(vm, OP_PRINT);
}

static void returnStatement(VM* vm) {
    if (current->type == TYPE_SCRIPT) {
        error(vm, "Can't return from top-level code.");
    }
    if (match(vm, TOKEN_SEMICOLON)) {
        emitReturn(vm);
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error(vm, "Can't return a value from an initializer.");
        }
        expression(vm);
        consume(vm, TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(vm, OP_RETURN);
    }
}

static void whileStatement(VM* vm) {
    int loopStart = currentChunk()->count;
    consume(vm, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression(vm);
    consume(vm, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(vm, OP_JUMP_IF_FALSE);
    emitByte(vm, OP_POP);
    statement(vm);

    // jump backwards
    emitLoop(vm, loopStart);

    patchJump(vm, exitJump);
    emitByte(vm, OP_POP);
}

static void synchronize(VM* vm) {
    parser.panicMode = false;

    // Skip tokens until we reach something that looks like a statement boundary - either we just passed a semicolon, or the subsequent token is one that begins a statemetn
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ; // Do nothing
        }

        advance(vm);
    }
}

static void declaration(VM* vm) {
    if (match(vm, TOKEN_CLASS)) {
        classDeclaration(vm);
    } else if (match(vm, TOKEN_FUN)) {
        funDeclaration(vm);
    } else if (match(vm, TOKEN_VAR)) {
        varDeclaration(vm);
    } else {
        statement(vm);
    }

    if (parser.panicMode) synchronize(vm);
}

static void statement(VM* vm) {
    if (match(vm, TOKEN_PRINT)) {
        printStatement(vm);
    } else if (match(vm, TOKEN_FOR)) {
        forStatement(vm);
    } else if (match(vm, TOKEN_IF)) {
        ifStatement(vm);
    } else if (match(vm, TOKEN_RETURN)) {
        returnStatement(vm);
    } else if (match(vm, TOKEN_WHILE)) {
        whileStatement(vm);
    } else if (match(vm, TOKEN_LEFT_BRACE)) {
        beginScope();
        block(vm);
        endScope(vm);
    } else {
        expressionStatement(vm);
    }
}

ObjFunction* compile(VM* vm, const char* source) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT, vm);

    parser.hadError = false;
    parser.panicMode = false;

    advance(vm);

    while (!match(vm, TOKEN_EOF)) {
        declaration(vm);
    }

    ObjFunction* function = endCompiler(vm);
    return parser.hadError ? NULL : function;
}

void markCompilerRoots(VM* vm) {
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject(vm, (Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}