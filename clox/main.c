#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "vm.h"
#include "utest.h"

static void repl() {
    VM vm;
    initVM(&vm, stdout, stderr);

    char line[1024];
    for (;;) {
        printf("> ");

        // it'd be nice for the REPL to handle multiple lines gracefully, but this is okay for our purposes
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(&vm, line);
    }
    freeVM(&vm);

}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }


    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);

    VM vm;
    initVM(&vm, stdout, stderr);

    InterpretResult result = interpret(&vm, source);
    free(source);
    freeVM(&vm);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    return 0;
}