# clox: An Implementation of lox in C
#### Working through [Crafting Interpreters](craftinginterpreters.com) by Bob Nystrom

### Features Added
**Chapter 14** 
1. Created foundation for chunks. A chunk is a sequence of instructions, and we've built a dynamic array structure for a Chunk. The Chunk struct now holds an array of bytes, in which we can store OpCodes.
2. Create a disassembler that takes a chunk and prints all the instructions in it. It'll be used by us, lox maintainers, but not lox users. See the debug files
3. Store constants (literals) in a constant pool, ValueArray. It'll be a dynamic array of values. An instruction using a constant looks up the value by index there. Each chunk has its own ValueArray. The opcode OP_CONSTANT produces the literal value. Also add debug funcs so we can print the value of the OP_CONSTANT instruction
4. Store line #'s in a separate field in the chunk. This is simple, and a bit inefficient, but a pro is that it's stored separately from the instructions themselves, which is good b/c we only access it when there's a runtime error. 
5. Add a VM
6. Add infrastructure for a value stack, using stack-based (instead of register based

### Additional features
###### generated from exercises in text
###### Shown with link to another branch if that was deemed necessary
1. Instead of storing line numbers in an array the length of codes, where every element is the line number that code is on, use a run-length encoding scheme. If the original storage produced: 1,1,1,1,2,2,3,3,3,3,4,4, store repeating count,lineNumber pairs instead. So the above would be: 4,1,2,2,3,3,2,4.
Hm, actually, that won't work b/c to get the line number which we need for debugging, we will only have the chunk's index of where the instruction occurs
```
chunk's idx                         lineNumber
of where the        line            ,
instr occurs        number          chunkOffset
0                   1               1, 0
1                   1               
2                   1
3                   1
4                   2               2, 4
5                   2
6                   3               3, 6
7                   3
8                   3
9                   3
10                  4               // 4, 10
11                  4

2nd column was original lines array (int*)
3rd column now store array of lineNumber+offset (Line*)  
```