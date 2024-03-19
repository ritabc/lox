//
// Created by Rita Bennett-Chew on 2/2/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "common.h"
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

typedef void (*ParseFn)(bool canAssign);

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

Parser parser;
Compiler* current = NULL; // this global variable isn't the best solution, especially if we were running in a multi-threaded app with multiple compilers running in parallel

// returns the address of the chunk owned by the function we're compiling atm
static Chunk* currentChunk() {
    return &current->function->chunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    // keep looping and reporting errors
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// advances, but also validates whether the token has an expected type
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction); // we'll return the offset of this instr
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count-2;
}

static void emitReturn() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
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
    local->name.start = "";
    local->name.length = 0;
}

static ObjFunction* endCompiler() {
    emitReturn();
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
static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount-1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}


static void expression();
static void statement(VM* vm);
static void declaration(VM* vm);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(current->vm, name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                /* prevent the following:
                    { var a = "outer";
                        { var a = a; }}
                 */
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

// Upvalues used for closures
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    // reuse upvalue index if we find an upvalue in the array whose slot index matches the one we're adding
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
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
static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // indicate the variable is 'declared' but 'uninitialized'
    local->isCaptured = false;
}

// the function which tells the compiler to record the existence of the variable
// only do this for locals, since global variables are late bound
static void declareVariable() {
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
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

// consumes the identifier token for the variable name
// if global: adds its lexeme to the chunk's constant table
// if global: returns the constant table index where it was added
// if local: returns 0 (dummy table index)
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();

    // if the variable is local, exit.
    // We don't store local variable identifiers in the constant table
    if (current->scopeDepth > 0) return 0;
    return identifierConstant(&parser.previous);
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
static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        // at runtime, don't create a local variable
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            // Each of the following arg expressions generates code that leavees its value on the stack in preparation for the call
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

// when called, LHS of expression has already been compiled. AT runtime, its value will be on stack top. If it's falsey, we skip the right operand and LHS value is result of entire expression. If it's not falsey, discard the LHS value, evaluate the right operand
static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

// assume the initial '(' has already been consumed
// grouping is purely syntactic and has no runtime semantics and doesn't emit any bytecode on its own
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// when LHS if falsey, do tiny jump to next statement, which is unconditional jump over the code for the right operand.
static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// +1, -2 trims the quotation marks
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(current->vm, parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

// The left hand operator has already been compiled,
// and the infix operator has already been consumed
static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:      emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:     emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:         emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:            emitByte(OP_ADD); break;
        case TOKEN_MINUS:           emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:            emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:           emitByte(OP_DIVIDE); break;
        default: return; // unreachable
    }
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable
    }
}

// evaluate the operator first, then encounter the OP_NEGATE instruction,
// at that point, pop the value, negate it, and push the result
static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
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
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

// Parse until we get to a precedence lower than the given precedence
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block(VM* vm) {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration(vm);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(VM* vm, FunctionType type) {
    // Create a separate Compiler for each function being compiled.
    Compiler compiler;
    initCompiler(&compiler, type, vm);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    // handle Parameters, which are local variables declared in the outermost lexical scope of the func body
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(vm);

    // After reaching the end of the func's block body,
    // call endCompiler, which yields the newly compiled function object
    // which is stored as a constant in the surrounding func's constant table
    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void funDeclaration(VM* vm) {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(vm, TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        // if user doesn't initialize the var, do it for them, init-ing it to nil.
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement(VM* vm) {
    beginScope(); // necessary for var declared in for loop - that var should be scoped to the loop body
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition
    }

    // Since we only make a single pass, we compile the increment clause after we encounter it.
    // We'll jump over the increment, run the body, jump back up to the increment, run it, then go to the next iteration
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }


    statement(vm);
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition
    }

    endScope();
}

static void ifStatement(VM* vm) {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    // Put condition expression on top of stack,
    // so we can use it to determine whether to execute the then branch or skip it, at runtime
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // Use backpatching: emit the half finished jump instruction 1st with a placeholder offset operand (keep track of where that is)
    // emitJump emits the given instruction, then 2 placeholder byte instructions
    int thenJump = emitJump(OP_JUMP_IF_FALSE);

    // when the condition is truthy, pop it right before the code inside the then branch
    emitByte(OP_POP);
    statement(vm);

    int elseJump = emitJump(OP_JUMP);

    // Then, after compiling the then body, we'll know how far to jump, so replace it with the complete instruction
    patchJump(thenJump);

    // if the condition is falsey, pop at the beginning of the else branch
    emitByte(OP_POP);

    if (match(TOKEN_ELSE)) statement(vm);
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement(VM* vm) {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement(vm);

    // jump backwards
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize() {
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

        advance();
    }
}

static void declaration(VM* vm) {
    if (match(TOKEN_FUN)) {
        funDeclaration(vm);
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement(vm);
    }

    if (parser.panicMode) synchronize();
}

static void statement(VM* vm) {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement(vm);
    } else if (match(TOKEN_IF)) {
        ifStatement(vm);
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement(vm);
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block(vm);
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction* compile(VM* vm, const char* source) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT, vm);

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration(vm);
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}