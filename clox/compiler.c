//
// Created by Rita Bennett-Chew on 2/2/24.
//

#include <stdio.h>

#include "compiler.h"
#include "common.h"
#include "scanner.h"

void compile(const char* source) {
    initScanner(source);

    int line = -1;
    for (;;) {
        Token token = scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            print("   | ");
        }
        // print token type and then the 1st token.length chars of token.start
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}