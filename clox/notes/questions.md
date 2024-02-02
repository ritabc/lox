# Questions

### C Related
QUES: macros can take types as arguments?
```C
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
```
ANSW: Yes, they are just text replacers with parameter functionality. Which means you can also pass operators, like we do in vm.c -> run() -> BINARY_OP

QUES: is there a convention in C/C++ where if you have an array of ints stored in a (int*), then you name that as a singular? Why not call `uint8_t* code` field in Chunk struct `codes` instead?

ANSW: No

QUES: What does the following mean?
``` In clox, we pass tokens by value, and since multiple tokens could point to the same lexeme string, ownership would get weird (from https://craftinginterpreters.com/scanning-on-demand.html#a-token-at-a-time)
```