# Type of Grammar Used
- recursive descent: the simplest way to build a parser. 
- it's a **top-down parser** b/c it starts from the top/outermost grammar rule (expression) & works its way down. INstead of starting with primary expressions and composing them into larger & larger chunks of syntax
- predictive parser: the parser will look ahead at upcoming tokens to decide how to parse. Recursive descent parsers fall into this category

# Parser Errors
**Goals** include to report as many distinct errors as there are, but also minimize cascaded errors. How to do both of these? 
**How to solve:** Whenever the parser detects an error, it enters panic mode. This means at least 1 token doesn't make sense given its current state within it's stack of grammar rules. It must **synchronize**, or align its state and the sequence of upcoming tokens before it can continue parsing - the next token must match the rule being parsed. 
**The Plan:** mark the synchronization point as some rule. The parser can synchronize by jumping out of any nested rules until it gets back to that rule. We will choose statement boundaries. Once the parser gets out to the statement boundary, it'll discard tokens until it reaches one that can appear at that point in the rule. We'll use Java's own call stack to track what the parser is doing. 1 call frame on the stack == 1 rule in the middle of being parsed. To reset, clear those call frames. We'll use exceptions. When we want to synchronize, throw the ParseError object. We'll catch it at statement boundaries. After catching the exception, the parser will be in the correct state. We'll then have to synchronize the tokens.


# Issues to Solve
**Issue** Having a single `expression` rule lets the grammar accept any kind of expression as a subexpression regardless of precedence rules. 
**Solution** Stratify the grammar by defining a separate rule for each precedence level
```
expression  -> ...
equality    -> ...
comparison  -> ...
term        -> ...
factor      -> ...
unary       -> ...
primary     -> ...
```

**Issue:** Nested unary expressions: `-!!true` valid
**Solution** a recursive rule
```
unary   -> ("!" | "-") unary ;
```

**Issue:** The above never terminates
**Solution** Unary must match exprs at its precedence level or higher
```
unary   -> ( "!" | "-" ) unary 
            | primary ;
            
factor  -> factor ( "/" | "*" ) unary
            | unary ;
```
Note: putting the recursive production on the left side makes the factor rule left-associative (`1 * 2 * 3` ==> `(1 * 2) * 3`)

**Issue:** Having the 1st symbol in the body of the rule be the same as the head (like with factor above) means the rule is left-recursive, which is a problem with some parsing techniques, including the one we'll use
**Solution** "Factor" out the self-reference, redefining the `factor` rule to be a flat sequence of mults & divides
```
factor   -> unary ( ( "/" | "*" ) unary )* ; 
```

**Issue** We must disallow declarations (of vars, funcs, classes) in some places. For instance;
```if (monday) var beverage = "espresso";```  
causes confusion: 
- Does beverage persist after the `if` stmt? 
- If so, what is the value on days other than Monday? 
- Does the var even exist on other days?

Additionally, a func decl would not make sense either:
```if (monday) func drinkCaffeine(beverage) {consume(beverage)}```
**Solution** Borrow the 'precedence' concept. Block statements have the 'highest precedence', then general statements. Declarations have a 'lower precedence'. Basically, although declarations are technically a type of statement, since they are not allowed in some places, we stratify them. When we allow a "statement", as defined below, we actually cannot use a declaration for that. (Although the opposite is true)
``` 
program     -> declaration* EOF ;
declaration -> varDecl | statement ; 
statement   -> exprStmt | printStmt ;
```
**Issue** Lox uses lexical aka static scope. Static scope is defined as: **A variable usage refers to the preceding declaration with the same name in the innermost scope that encloses the expression where the variable is used.**, where 'use' means variables as expressions AND assignments. Per that definition, the following code should print 'global' twice:
```
var a = "global";
{
  fun showA() {
    print a;
  }

  showA();
  var a = "block";
  showA();
}
```
because with the second call to `showA()`, the /usage/ of `a` is still happens in the `showA` declaration, and the preceding decl with the same name in the innermost enclosing scope would mean `a` evaluates to 'global'. The code above however, prints 'global' then 'block'. 
This is because of the way our environments are implemented. 
What happens currently:
- When we declare `showA`
- the declaration captures the current environment 
- Then, we mutate the environment with `var a = "block";`
- when we call `showA()` the second time, we use the mutated environment
A better way to think about lexically scoped langs, aka how they theoretically **should** work (although we'll fix this closure bug in lox differently, as this approach was deemed to require rewriting too much code)
- When we declare `showA`
- the declaration captures the current environment 
- Then, when we go to add something to the env, we first make a **copy** of the current environment, then add to that
- when we call `showA()` the second time, we use that new environment inspired by the original.
**Solution** Using Semantic Analysis. Semantic Analysis is like a parser, but instead of only telling us if the program is grammatically correct (syntactic analysis), this will start to figure out what pieces of the program actually mean. We will write semantic analysis code to resolve variable bindings (we'll know this expression is a variable, and which variable is it?). It will resolve the variable bindings by walking the environment chain the same number of times for each resolution of a specific variable expression. The parser would be a good place for this, but for practice we'll make a separate pass. 

# Other notes
- Usually, a named function declaration is actually 2 distinct steps (1) creating a new func obj, and (2) binding a new variable to it. This requires anonymous funcs, like:
```
var add = fun (a, b) {
print a + b;
};
```
but we will just always use named functions

# Syntactical Grammar
```

program      -> declaration* EOF ;

declaration  -> classDecl | funDecl | varDecl | statment ;

classDecl    -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}" ;

funDecl      -> "fun" function ;

function     -> IDENTIFIER "(" parameters? ")" block ;

parameters   -> IDENTIFIER ( "," IDENTIFIER )* ;

statement    -> exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block ;

exprStmt     -> expression ";" ;

forStmt      -> "for" "(" (varDecl | exprStmt | ";" ) expression? ";" expression? ")" statement ;

ifStmt       -> "if" "(" expression ")" statement ( "else" statement )? ;

printStmt    -> "print" expression ";" ;

returnStmt   -> "return" expression? ";" ;

whileStmt    -> "while" "(" expression ")" statement ;

block        -> "{" declaration* "}" ;


expression   -> assignment ;
assignment   -> (call ".")? IDENTIFIER "=" assignment | ternary ;        // right-assoc
ternary      -> logic_or ("?" ternary ":" ternary )* ;     // right-assoc
logic_or     -> logic_and ( "or" logic_and )* ;
logic_and    -> equality ( "and" equality )* ;
equality     -> comparison ( ( "!=" | "==" ) comparison )* ; 
comparison   -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term         -> factor ( ( "-" | "+" ) factor )* ;
factor       -> unary ( ( "/" | "*" ) unary )* ;
unary        -> (( "!" "-" ) unary) | call                // right-assoc
call         -> primary ( ( "(" arguments? ")" ) | ( "." IDENTIFIER ) )* ;          // allows us to do currying like fn(1)(2)(3), where each func only takes 1 arg but returns a func that takes another arg
arguments    -> expression ( "," expression )* ;
primary      -> NUMBER | STRING | "true" | "false" | "nil" 
             | IDENTIFIER | "(" expression ")" | ( "super" "." IDENTIFIER ) ; 
             
varDecl      -> "var" IDENTIFIER ( "=" expression )? ";" ;
```

# Lexical Grammar
```
NUMBER         → DIGIT+ ( "." DIGIT+ )? ;
STRING         → "\"" <any char except "\"">* "\"" ;
IDENTIFIER     → ALPHA ( ALPHA | DIGIT )* ;
ALPHA          → "a" ... "z" | "A" ... "Z" | "_" ;
DIGIT          → "0" ... "9" ;
```
