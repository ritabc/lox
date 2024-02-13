//
// Created by Rita Bennett-Chew on 2/11/24.
//
/* TODO: run integration test with source .lox code hard-coded here:
    - need to create a fd to which the printed output will be sent, after interpreting lox print statements.
    - additionally, need to create a fd to which lox errors will be sent
    use open_memstream
 */


// QUestion: is there a new outbuf for every test run?
// QUESTIon: Do compiling all in teardown? Does that make sense?
// I could, in each test, run through the code and do the 'first' pass: add to expecteds array (see OneNote notes)
// Would it make sense to do all compiling in teardown? Does that run once per test or once per suite (aka once for this entire file) or once per input file?
// let's assume once for this entire file
// what would a workflow look like?
// need to keep line numbers seprate for each file in nystrom's test dir
// because of, for ie: https://github.com/munificent/craftinginterpreters/blob/master/test/unexpected_character.lox
// So, doing compilation/interpreting all in teardown doesn't make the most sense (?)
//// humor self - what if it did?
//// it would mean storing the files in a fileData array, each element of which would have {sourceLines array, expectedLines array} struct. This would be created using a first pass parsing to get expectedLines. ExpectedLines would also have to track the src line it was generated from. It would mean a lot of memory, and we'd have to open & close each file once. So each test would open (&close) the .lox file. It would add to sourceLines dynamicArray, expectedLines dynamicArray. Then, in teardown we'd have a giant list of FileData, and we'd go through each file, each line; interpret. If there's an expectedLine at that line number, assertEquals.
// That seems like a lot of mental-model overhead/complexity.
// Alternatively: each test opens (& closes each file). It parses the first pass line-by-line, like repl in main(), but doesn't interpret. If it encounters an expect comment, store that in an ExpectLine struct {lineNo, valueString, outputType (runtimeError, value, errorAt,etc)}. Then add that ExpectLine to an expectedLines dynamic array - one exists per file. (Don't forget to free it). (or maybe, a regular array with same # of lines as source file. Store nil if no expectValue)
// Then, pass 2: interpret line-by-line. For lines that produce no output to vm->fout nor to vm->ferr, assert there is no ExpectLine value for that line No. If output is produced, assert that it matches.
// I think this is a better way to go

#include <stdio.h>

#include "utest.h"
#include "vm.h"

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

struct InterpretTest {
    VM* vm;
    MemBuf out;
    MemBuf err;
};

// Use globals so we can assign from main, use in tests.
// TODO: using a setup function, fixtures is probably better? How to do?
// One option: 1/2 & 1/2: keep global variables, but init and free in setup / teardown // TODO
VM vm;
MemBuf out, err;

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
    int line;
    Value val;
} Expected;

typedef struct {
    int capacity;
    int count;
    Expected* expected;
} ExpectedArray;

void initExpectedArray(ExpectedArray* array) {
    array->expected = NULL;
    array->capacity = 0;
    array->count = 0;
}

static char* parseFileForTesting(const char* path, ExpectedArray* expecteds) {
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
    int lineNumber = 1;
    size_t sourceBufferSize = 0;
    sourceBuffer[0] = '\0';

    while ((read = getline(&line, &lineCapp, file)) != -1) {
//        gatherExpected(line, expecteds, lineNumber++);
        // strlcat takes null-terminated baseString, null-term toAppendString, and total size of new string (base size + toAppend size + 1 for null term)
        sourceBufferSize = strlcat(sourceBuffer, line, read + sourceBufferSize + 1);
    }

    fclose(file);
    free(line);

    return sourceBuffer;
}

UTEST(InterpretTest, testHardCodedFile) {
    ExpectedArray expecteds;
    initExpectedArray(&expecteds);
    char* source = parseFileForTesting("./test/equality.lox", &expecteds);

    interpret(&vm, source);
    fflush(out.fptr);
    fflush(err.fptr);
    EXPECT_STREQ("true\n", out.bufp);
    EXPECT_STREQ("", err.bufp);
}

UTEST_STATE();

int main(int argc, const char* argv[]) {
    // Overall Setup (setup and teardown methods of utest run after/before each (assuming they follow googletest's semantics)
    initMemBuf(&out);
    initMemBuf(&err);
    out.fptr = open_memstream(&out.bufp, &out.size);
    err.fptr = open_memstream(&err.bufp, &err.size);

    initVM(&vm, out.fptr, err.fptr);

    utest_main(argc, argv);

    // Teardown
    fclose(out.fptr);
    fclose(err.fptr);
    free(out.bufp);
    free(err.bufp);
}