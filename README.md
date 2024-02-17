# Lox: [Java](#jlox-an-implementation-of-lox-in-java) and [C](#clox-an-implementation-of-lox-in-c) Implementations
#### Learning Material and Challenges from "Crafting Interpreters" by Bob Nystrom

## jlox: An Implementation of lox in Java
### Features Added
1. Scanning
1. Add script tool to generate AST. 
1. Use visitor pattern, implement Lisp-like pretty printing as example
1. Add parsing for expressions
1. Add evaluator, again using the Visitor pattern
1. Add support for statements, including expression statements and "print expr ;" statements
1. Add support for global vars
1. Add support for assignment
1. Add lexical scoping, (chain of) enclosing environments
1. Add control flow (branching with if & logical and/or, and while & for loops)
1. Add support for function decls and calls, including return values.
-- Add a builtin func, clock(), which is a wrapper around Java's System.currentTimeMillis()
-- throw runtime Lox exception from interpreter when user attempts to call something that isn't callable, like a String. (Instead of allowing a java exception to be seen)
-- For now, we'll always set the function's body's parent env as the top-level global environment. So if an identifier isn't defined in the function itself, the interpreter will look outside the func and in the global scope. However, this means functions nested inside blocks or other functions, aka local functions, and closures are not yet supported.
1. (Around about here, stop updating AstPrinter visitor)
1. Add support for closures.
1. Fix bug adding support for closures introduced, using Resolver - see grammar.md for more info
1. Use resolver to detect errors redeclaring variables twice in same local scope
1. Disallow returnStmt outside of function
1. Add support for classes, including this keyword to access self within a class
1. Add constructors & initializers, allow init() to be invoked directly, like instance.init(), disallow returning a value from init(). But allow empty return statement (which has no value) from init, but return this instead of nil
1. Add inheritance, including of methods, and calling superclass methods with the `super` keyword. 

### Additional features 
###### generated from Challenges in text
#### Shown with link to another branch if that was deemed necessary
1. Testing in Java
1. C-style block comments
1. For more practice with visitor pattern, implement printer for Reverse Polish notation, where `(1 + 2) * (4 - 3)` becomes `1 2 + 4 3 - *` (note there will be ambiguity with negation vs subtraction, so it'll be necessary to redefine `~` as the negation symbol)
1. Add C-style ternary operator
1. Add suppport for break statements (branch: [break-stmts](https://github.com/ritabc/lox/tree/break-stmts))
1. Add support for anonymous functions (branch: [anon-funcs](https://github.com/ritabc/lox/tree/anon-funcs))
1. Add support for "static" class methods using metaclasses. A static class method can be added like so:
```
class Math {
  class square(n) {
    return n * n;
  }
}
print Math.square(3) // 9 
```
A separate metaclass exists for each class. It holds any static (class) methods of the class.
(branch: [class-methods](https://github.com/ritabc/lox/tree/class-methods))

## clox: An Implementation of lox in C
### Features Added
1. Created foundation for chunks. A chunk is a sequence of instructions, and we've built a dynamic array structure for a Chunk. The Chunk struct now holds an array of bytes, in which we can store OpCodes.
2. Create a disassembler that takes a chunk and prints all the instructions in it. It'll be used by us, lox maintainers, but not lox users. See the debug files
3. Store constants (literals) in a constant pool, ValueArray. It'll be a dynamic array of values. An instruction using a constant looks up the value by index there. Each chunk has its own ValueArray. The opcode OP_CONSTANT produces the literal value. Also add debug funcs so we can print the value of the OP_CONSTANT instruction
4. Store line #'s in a separate field in the chunk. This is simple, and a bit inefficient, but a pro is that it's stored separately from the instructions themselves, which is good b/c we only access it when there's a runtime error. 
5. Add a VM
6. Add infrastructure for a value stack, using stack-based (instead of register based)
7. Setup REPL and runFile functions to read user input (source code)
8. setup barebones scanner to take as input stream of source code
9. Implement barebones Pratt parser that can handle grouping, unary, numbers, and some binary ops
10. Add macros and structs for storing lox's dynamic types
11. Support runtime errors and unary negation
12. Support binary arithmetic operators
13. Add nil and bools, not and falsiness
14. add support for strings, as StringObj which "inherit" from Obj, add cleanup funcs that will clean up VM before it exits (although our running VM won't free memory until we write the GC)
15. Add hash tables. Included in the implementation are a tombstone delete method:
 - upon deletion store a tombstone entry (key=NULL, value=TrueValue)
 - findEntry returns entry with key = NULL if no entry found with given key. This'll either be a totally empty entry or the first tombstone (key=NULL, value=TrueValue) encountered 
 - findEntry could go into infinite loop if no empty slots are encountered, so we need keep track of tombstone count when resizing, and ensure the array is never completely full
 -- for load factor, treat tombstones like full buckets to prevent infinite loop 
 -- when deleting an entry, don't reduce the count. Count is not the # of entries in the hash table, but the number of entries + tombstones. 
 -- So increment the count during insertion iff the new entry goes into a totally empty bucket, not a tombstone
 -- when resizing the hash's array, throw out all tombstones (b/c during resize we copy all entries to new table)
16. Use a hash table to do **string interning**: create a collection of 'interned' or 'internal' strings. Strings in the collection are guaranteed to be textually distinct. When encountering a string, look for a matching string in the collection. If found - use the original one. Otherwise, add the string to the collection. (sidenote: this is how ruby symbols are implemented). This will speed up string equality, but also: method calls & instance fields, which are looked up by name at runtime (since Lox is dynamically typed), will be much faster
17. Add support for global variables using a hash table, `globals`, on th vm.
18. Add support for local variables using a stack. The stack offsets will be operands for the bytecode instructions that read & store local vars.
19. Control flow for ifs, elses, and, or, and while statements
20. Add for statements, again by desugaring the for to a while with some extra stuff before it & at the end of the body.

### Additional features
###### generated from Challenges in text
#### Shown with link to another branch if that was deemed necessary
1. Instead of storing line numbers in an array the length of codes, where every element is the line number that code is on, use a run-length encoding scheme. 
1. Add testing framework (interpret_test.c)
2. 