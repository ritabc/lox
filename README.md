# Lox: Implementations in Java and C
#### Working through [Crafting Interpreters](craftinginterpreters.com) by Bob Nystrom


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
#### Shown with link to another branch if that was deemed necessary
1. Testing in Java
1. C-style block comments
1. For more practice with visitor pattern, implement printer for Reverse Polish notation, where `(1 + 2) * (4 - 3)` becomes `1 2 + 4 3 - *` (note there will be ambiguity with negation vs subtraction, so it'll be necessary to redefine `~` as the negation symbol)
1. Add C-style ternary operator
1. Add suppport for break statements (branch: break-stmts)
1. Add support for anonymous functions (branch: anon-funcs)
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
(branch: class-methods)
