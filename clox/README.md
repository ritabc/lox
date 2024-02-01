# clox: An Implementation of lox in C
#### Working through [Crafting Interpreters](craftinginterpreters.com) by Bob Nystrom

### Features Added
**Chapter 14** 
1. Created foundation for chunks. A chunk is a sequence of instructions, and we've built a dynamic array structure for a Chunk. The Chunk struct now holds an array of bytes, in which we can store OpCodes.
2. Create a disassembler that takes a chunk and prints all the instructions in it. It'll be used by us, lox maintainers, but not lox users. See the debug files
3. Store constants (literals) in a constant pool, ValueArray. It'll be a dynamic array of values. An instruction using a constant looks up the value by index there. Each chunk has its own ValueArray. The opcode OP_CONSTANT produces the literal value. Also add debug funcs so we can print the value of the OP_CONSTANT instruction


### Additional features
###### generated from exercises in text
###### Shown with link to another branch if that was deemed necessary
