//
// Created by Rita Bennett-Chew on 2/11/24.
//
/* run integration test with source .lox code hard-coded here:
 */

#include <stdio.h>
#include <dirent.h>

#include "utest.h"
#include "vm.h"
#include "memory.h"

#define TEST_FILE_COUNTS 25

typedef struct {
    char* bufp;
    size_t size;
    FILE* fptr;
} MemBuf;

void initMemBuf(MemBuf* memBuf) {
    memBuf->bufp = NULL;
    memBuf->size = 0;
    memBuf->fptr = NULL;
}

typedef enum {
    EXP_OUTPUT,
    EXP_ERROR,
    EXP_LINE,
    EXP_RUNTIME_ERROR,
    EXP_SYNTAX_ERROR,
    EXP_STACK_TRACE_ERROR
} ExpectedType;

typedef struct {
    ExpectedType type;
    char* output;
} ExpectedLine;

// An ExpectedArray is a dynamic array of expected lines. One is generated for each file.
typedef struct {
    uint32_t capacity;
    uint32_t count;
    ExpectedLine* outLines;
} ExpectedArray;

// Use globals so we can assign from main, use in tests.
VM vm;
MemBuf outActual, errActual;
MemBuf outExpected, errExpected;
int numTests = 0;

// data for each file
struct FixtureData {
    ExpectedArray* expectedsForCurrFile;
};

// dynamically add an ExpectedLine to ExpectedArray->outLines
void addToExpecteds(ExpectedArray* exps, ExpectedLine* expLine) {
    if (exps->capacity < exps->count+1) {
        int oldCapacity = exps->capacity;
        exps->capacity = GROW_CAPACITY(oldCapacity);
        exps->outLines = GROW_ARRAY(ExpectedLine, exps->outLines, oldCapacity, exps->capacity);
    }
    exps->outLines[exps->count].output = expLine->output;
    exps->outLines[exps->count++].type = expLine->type;
}

// Create a new ExpectedLine from a type, comment[start..len]
ExpectedLine* newExpectedLine(ExpectedType type, char* comment, uint32_t startAt, uint32_t finalOutputLen) {
    ExpectedLine* ret = malloc(sizeof(ExpectedLine*));
    ret->type = type;
    // ret->output is a char*, which will be freed in getExpectedStrings
    ret->output = malloc(sizeof(char*) * (finalOutputLen-startAt+1));
    strlcpy(ret->output, &comment[startAt], finalOutputLen+1);
    return ret;
}

// Parse sourceLine and add to exps if we encounter an Expect line like a value or error.
// ExpectedArray is dynamic: see chunk.c implementation of chunk->code for how to write to it
static void getExpectedIfAnyFromSourceLine(char* sourceLine, ExpectedArray* exps) {
    // get pos of a comment
    // if there is no comment, return
    // if the comment matches expect value: do stuff
    // if the comment matches expect error: do stuff
    // if the comment matches ...
    char* commentPrefix = "// ";
    int commentPrefixLen = strlen(commentPrefix);
    char* comment = strcasestr(sourceLine, commentPrefix);
    if (comment == NULL) {
        // no output (expect) comment found
        return;
    }

    // Eg: "expect runtime error: msg" --> want prefix + msg (entire comment)
    char* expectRuntimeErrorPrefix = "expect runtime error: ";
    uint32_t expectPrefixLen = strlen(expectRuntimeErrorPrefix) + 3; // plus 3 for preceding '// '
    char* expectRuntimeError = strcasestr(comment, expectRuntimeErrorPrefix);
    if (expectRuntimeError != NULL) {
        ExpectedLine* expected = newExpectedLine(EXP_RUNTIME_ERROR, comment, expectPrefixLen, strlen(comment));
        addToExpecteds(exps, expected);
        return;
    }

    // Eg: "[line 3] Error at '{': Expect expression." --> want prefix and msg (entire comment)
    char* expectLineErrorAtPrefix = "[line ";
    expectPrefixLen = strlen(expectLineErrorAtPrefix);
    char* expectLineErrorAt = strcasestr(comment, expectLineErrorAtPrefix);
    if (expectLineErrorAt != NULL) {
        ExpectedLine* expected = newExpectedLine(EXP_LINE, comment, 0, strlen(comment));
        addToExpecteds(exps, expected);
        return;
    }

    // Eg: "Error at 'return': msg" --> want prefix & msg (entire comment)
    char* expectErrorAtPrefix = "Error at ";
    expectPrefixLen = strlen(expectErrorAtPrefix);
    char* expectErrorAt = strcasestr(comment, expectErrorAtPrefix);
    if (expectErrorAt != NULL) {
        ExpectedLine* expected = newExpectedLine(EXP_ERROR, comment, 0, strlen(comment));
        addToExpecteds(exps, expected);
        return;
    }

    // Eg: "expect 1" --> just want value
    char* expectPrefix = "expect: ";
    expectPrefixLen = strlen(expectPrefix);
    char* expectValue = strcasestr(comment, expectPrefix);
    if (expectValue != NULL) {
        ExpectedLine* expected = newExpectedLine(EXP_OUTPUT, comment, commentPrefixLen+expectPrefixLen, strlen(comment)-commentPrefixLen-expectPrefixLen);
        addToExpecteds(exps, expected);
        return;
    }
}

// A recursive function that gathers names of all .lox test files within enclosingDirPath, adds them to testFiles
static void getTestFilepaths(char* enclosingDirPath, char** loxFilePaths) {
    DIR* dirp;
    struct dirent* dir;
    dirp = opendir(enclosingDirPath);
    if (dirp == NULL) {
        fprintf(stderr, "Could not open directory \"%s\".\n", enclosingDirPath);
    }
    while ((dir = readdir(dirp)) != NULL) {
        int enclosingDirPathLen = strlen(enclosingDirPath);

        char* dot = strrchr(dir->d_name, '.'); // strrchr will get the rightmost '.'
        if (dot && strcmp(dot, ".lox") == 0) {
            // Add .lox files to loxFiles
            // plus one for separating slash, one for null term
            char testFile[enclosingDirPathLen + 2];
            // make copy of enclosingDirPath b/c we want to use it in its unaltered state
            // for other .lox file iterations of this loop & recursive searching.
            strlcpy(testFile, enclosingDirPath, enclosingDirPathLen + 1);
            testFile[enclosingDirPathLen] = '/';
            testFile[enclosingDirPathLen+1] = '\0';

            // TODO (remove) we're good above here, assume
            // TODO (remove) what is the goal with the following lines?
            // copy next filename (not path) from dir
            uint32_t testFileLen = strlcat(testFile, dir->d_name, enclosingDirPathLen + 1 /*for ending '/'*/ + strlen(dir->d_name) + 1 /* for NUL*/);
            char* loxFilePathPtr = malloc(testFileLen*sizeof(char*));
            strlcpy(loxFilePathPtr, testFile, testFileLen + 1);
            loxFilePaths[numTests++] = loxFilePathPtr;
        } else if (dir->d_type == DT_DIR) {
            // recursively search any directories found here
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".DS_Store") == 0) {
                continue;
            }
            // concat new dir name to original enclosingDirPath
            // but first make copy of enclosingDirPath so original is unaltered
            char newDirpath[enclosingDirPathLen + 2];
            strlcpy(newDirpath, enclosingDirPath, enclosingDirPathLen+1);
            newDirpath[enclosingDirPathLen] = '/';
            strlcat(newDirpath, dir->d_name, enclosingDirPathLen + strlen(dir->d_name) + 2);
            getTestFilepaths(newDirpath, loxFilePaths);
        }
    }
}

// Reads file line by line. For each line, gather any expected values/errors, add to source buffer, which will be returned.
static char* parseFileForTesting(const char* path, ExpectedArray* expectedsForThisFile) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    // get size of file, use it to allocate sourceBuffer
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* sourceBuffer = (char*)malloc(fileSize + 1);
    if (sourceBuffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    char* line = NULL;
    size_t lineCapp = 0;
    ssize_t read; // will be -1 at eof, or at error
    size_t sourceBufferSize = 0;
    sourceBuffer[0] = '\0';

    while ((read = getline(&line, &lineCapp, file)) != -1) {
        getExpectedIfAnyFromSourceLine(line, expectedsForThisFile);
        // strlcat takes null-terminated baseString, null-term toAppendString, and total size of new string (base size + toAppend size + 1 for null term)
        sourceBufferSize = strlcat(sourceBuffer, line, read + sourceBufferSize + 1);
    }

    fclose(file);
    free(line);

    return sourceBuffer;
}

static void getExpectedStrings(ExpectedArray* exps, MemBuf outExp, MemBuf errExp) {
    // convert exps struct into expected out & expected err strings
    // from exps, compose char* for expectedOutput, char* for expectedErrs
    // loop over all exps, which is for this current file.
    // switch on type of expLine
    uint32_t outLen = 0;
    uint32_t errLen = 0;
    for (int i = 0; i < exps->count; i++) {
        switch (exps->outLines[i].type) {
            case EXP_OUTPUT:
                fprintf(outExp.fptr, "%s", exps->outLines[i].output);
                free(exps->outLines[i].output);
                break;
            case EXP_ERROR:
            case EXP_LINE:
            case EXP_RUNTIME_ERROR:
            case EXP_SYNTAX_ERROR:
            case EXP_STACK_TRACE_ERROR:
                fprintf(errExp.fptr, "%s", exps->outLines[i].output);
                free(exps->outLines[i].output);
                free(&exps->outLines[i]);
                break;
        }
    }
}

static void freeTestPaths(char** testPaths, int count) {
    for (int i = 0; i < count; i++) {
        free((void*) testPaths[i]);
    }
}

void initFileExpecteds(ExpectedArray* exps) {
    // A dynamically growing array since we have no way of knowing how many expectedValues we'll have
    exps->capacity = 0;
    exps->count = 0;
    exps->outLines = NULL;
}

// Another global, the length of which is number of test files
char* testPaths[TEST_FILE_COUNTS];
ExpectedArray exps;

UTEST_I_SETUP(FixtureData) {
    initMemBuf(&outActual);
    initMemBuf(&errActual);
    initMemBuf(&outExpected);
    initMemBuf(&errExpected);
    outActual.fptr = open_memstream(&outActual.bufp, &outActual.size);
    errActual.fptr = open_memstream(&errActual.bufp, &errActual.size);
    outExpected.fptr = open_memstream(&outExpected.bufp, &outExpected.size);
    errExpected.fptr = open_memstream(&errExpected.bufp, &errExpected.size);

    initVM(&vm, outActual.fptr, errActual.fptr);

    initFileExpecteds(&exps); // a struct that holds cap of, count of, and expectedArrays;
    utest_fixture->expectedsForCurrFile = &exps;
    printf("Test: %s (%i)\n", testPaths[utest_index], (u_int32_t) utest_index);
}

UTEST_I_TEARDOWN(FixtureData) {
    char* source = parseFileForTesting(testPaths[utest_index], utest_fixture->expectedsForCurrFile);
    interpret(&vm, source);

    getExpectedStrings(utest_fixture->expectedsForCurrFile, outExpected, errExpected);

    fflush(outActual.fptr);
    fflush(errActual.fptr);
    fflush(outExpected.fptr);
    fflush(errExpected.fptr);
    EXPECT_STREQ(outExpected.bufp, outActual.bufp);
    EXPECT_STREQ(errExpected.bufp, errActual.bufp);

    fclose(outActual.fptr);
    fclose(errActual.fptr);
    fclose(outExpected.fptr);
    fclose(errExpected.fptr);
    free(outActual.bufp);
    free(errActual.bufp);
    free(outExpected.bufp);
    free(errExpected.bufp);
}

UTEST_I(FixtureData, LoxTest, TEST_FILE_COUNTS) {}


UTEST_STATE();

int main(int argc, const char* argv[]) {
    // TODO: Refactor so that MemBufs are used instead of ExpectedArray?

//    // 1. Setup before all tests
    *testPaths = malloc(sizeof(char*) * TEST_FILE_COUNTS);

    // If running/debugging from clion:
    getTestFilepaths("../test", testPaths);
    // If running with leaks: ($ leaks --atExit -- /Users/rita/Documents/Projects-Code/teachYourselfCSdotcom/08_languages/lox/clox/cmake-build-leaks/integrationTests)
//    getTestFilepaths("./test", testPaths);

    // 2. Run all tests
    utest_main(argc, argv);

    // 3. Teardown after all tests
    freeTestPaths(testPaths, numTests);
}