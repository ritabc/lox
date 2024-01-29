package com.craftinginterpreters.lox;

import java.util.List;

public class LoxFunction implements LoxCallable {

    private final Stmt.FunDecl declaration;

    // closure represents the lexical scope surrounding the function declaration
    private final Environment closure;

    LoxFunction(Stmt.FunDecl declaration, Environment closure) {
        this.declaration = declaration;
        this.closure = closure;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {

        // When we call the function, we use the scope surrounding the func declaration as the call's parent, instead of going all the way out to globals
        // Creates an env chain that goes from the func's declared body, out through all the envs where the func is declared, all the way to global scope.
        Environment environment = new Environment(closure);

        for (int i = 0; i < declaration.params.size(); i++) {
            environment.define(declaration.params.get(i).lexeme, arguments.get(i));
        }

        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }

        // Because Lox is dynamically typed, the compiler doesn't have a way to prevent a user from taking the result of a func call with no return stmt, and storing it somewhere. (With statically typed langs, functions either return "void" or a value.) So, function calls in lox must return something, even if just nil.
        return null;
    }

    @Override
    public int arity() {
        return declaration.params.size();
    }

    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}