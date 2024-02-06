#### General Notes
* Chunks: refers to a sequence of bytecode. Each 'chunk' is an array.
* Assembler: program which converts human-readable CPU instructions (assembly, for eg) like `ADD` and `MULT` into their binary machine code equivalent
* Disassembler: converts the other way. From binary machine code to textual list of instructions
* Where is the value of a literal stored? immediate instructions is often used for small, fixed sized values like ints, and the value is stored directly in the code stream, just after the opcode. For large or var-sized constants like strings, there is often a separate "constant data" region stored elsewhere. The JVM associates a constant pool with each compiled class. We'll do the same here, except we'll put _all_ constants there, including simple ints
* Bytecode instruction **operands**, which are different than arithmetic operator operands, can be stored with the opcode. Instructions have operands. Operands are stored as binary data immediately after the opcode in the instruction stream. They let us parameterize what the instruction does. For instance, OP_RETURN consists of just the opcode byte. But an OP_CONSTANT must also contain an index to locate the constant in the ValueArray. 
* In the vm.c's run() function, there are more optimal ways to decode the instruction into an opcode, but for our purposes a giant switch statement will be okay. Other techniques incldue "direct threaded code", "jump table, computed goto".
#### clox Structure 
* 3 phases: Scanner, Compiler, VM. 
* Each pair of phases is connected by a datastructure
* to start, source code is input to the scanner
* Tokens flow from scanner->compiler
* chunks of bytecode flow from compiler->VM

* In jlox, we sent the source through a scanner and received a list of tokens. In clox, that'd require a dynamic array and a lot of overhead
* our compiler will only need 1 or 2 tokens at once (our grammer only requires a single token of lookahead), so don't scan until the compiler needs a token
* So the scanner will return the token by value

#### Pratt Parsing
* We'll use a table where we can look up a token type and find a function to parse & compile a prefix expression which starts with a token of that type (or a function to parse & compile an infix expression with left operand, then a token of that type) and the precedence of an infix expr that uses that token as an operator.
* A row in this table will be represented by a ParseRule