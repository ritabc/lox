# Questions

### C Related
QUES: macros can take types as arguments?
```C
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))
```
ANSW: Yes, they are just text replacers with parameter functionality