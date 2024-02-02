//
// Created by Rita Bennett-Chew on 2/2/24.
//

#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "common.h"


/*
 * A scanner takes source code as input and outputs Tokens
 * Each scanner is made of 2 pointers
 * - start marks the beginning of the current lexeme being scanned
 * - current points to the current char being looked at (which we haven't scanned yet)
 * and the line number for error reporting
 */
typedef struct {
    const char*  start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    return token;
}

Token scanToken() {
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    return errorToken("Unexpected character.");
}