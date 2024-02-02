# Questions

### C Related
QUES: macros can take types as arguments?
```C
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
```
ANSW: Yes, they are just text replacers with parameter functionality

QUES: is there a convention in C/C++ where if you have an array of ints stored in a (int*), then you name that as a singular? Why not call `uint8_t* code` field in Chunk struct `codes` instead?

ANSW: No