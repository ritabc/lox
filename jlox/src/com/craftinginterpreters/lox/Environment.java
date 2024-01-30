package com.craftinginterpreters.lox;


import java.util.HashMap;
import java.util.Map;

// A class which holds the Java Map to store variable bindings
// keys are bare strings, not tokens (identifier names)
// values are variable values
public class Environment {
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    // constructor for global scope's environment
    Environment() {
        enclosing = null;
    }

    // constructor for all other env's
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    /* defines a variable in current (innermost) scope
    & can redefine a variable too
        Meaning:
        var a = "before";
        var b = "after";
        is allowed. Having redefinition be an error would be better in some regards,
        but not in a REPL context where the user shouldn't be expected to keep track of which vars have been used already.
        For now, allow this for global variables
     */
    void define(String name, Object value) {
        values.put(name, value);
    }

    /*
    Get a variable from current scope, or recursively search up the enclosing env chain.
    Runtime error instead of syntax err at compile time.
     If we threw error at compile time, it'd be difficult to define recursive funcs.
     We want to be able to refer to (but not evaluate) a variable before it's defined.
     This will allow us to skip the forward declarations required in C
     Java doesn't require this b/c Java declares all names of declarations first, then looks at the function bodies
     */
    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        if (enclosing != null) return enclosing.get(name);

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    /* Instead of walking up the chain and finding the first (innermost) enclosing-env matching variable, we go exactly the number of chain-link hops we need.
    B/c of our closures bug where a function declaration which references a global variable will not behave in accordance with lexical scope rules if the global var is re-declared (& reassigned), and THEN the func is called.
    To fix this bug, when the func declaration occurs, and a variable expression is used inside of it, the distance in the outwards direction of the appropriate var declaration is stored by the resolver. Every time that variable expression is accessed it'll be in the same scope, thus will have the same depth.
     */
    Object getAt(int distance, String name) {
        return ancestor(distance).values.get(name);
    }

    Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }

        return environment;
    }


    // Like define, except does not allow assignment if the variable does not already exist. Also, recursively check enclosing environments up the chain
    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    void assignAt(int distance, Token name, Object value) {
        ancestor(distance).values.put(name.lexeme, value);
    }
}
