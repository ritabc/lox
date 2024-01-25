# Lox: Implementations in Java and C
#### Learning Material and Exercises from [Crafting Interpreters](craftinginterpreters.com) by Bob Nystrom


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

### Additional features
#### Shown with link to another branch if that was deemed necessary
1. Testing in Java
1. C-style block comments
1. For more practice with visitor pattern, implement printer for Reverse Polish notation, where `(1 + 2) * (4 - 3)` becomes `1 2 + 4 3 - *` (note there will be ambiguity with negation vs subtraction, so it'll be necessary to redefine `~` as the negation symbol)
1. Add C-style ternary operator
1. Add support for break statements (branch: break-stmts)