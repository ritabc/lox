package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

public class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {

    private enum FunctionType {
        NONE,
        FUNCTION,
        METHOD
    }

    private enum ClassType {
        NONE, CLASS
    }

    private final Interpreter interpreter;

    // A field representing a stack of (local) scopes. Each stack element is a map which represents a single block scope. Keys are variable names. Values are Boolean which represent whether it's been intialized and is ready for use
    // Global scopes are not tracked here - they're more dynamic. If we can't find a variable here, we assume it's global
    private final Stack<Map<String, Boolean>> scopes = new Stack();

    private FunctionType currentFunction = FunctionType.NONE;

    // Tells us if we are currently in a class declaration - will help to disallaw 'this' exprs outside of where appropriate
    private ClassType currentClass = ClassType.NONE;

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    // Assignment expressions need to have their variables resolved.
    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);

        for (Expr argument : expr.arguments) {
            resolve(argument);
        }
        return null;
    }

    // Object properties are looked up dynamically so they don't get resolved
    @Override
    public Void visitGetExpr(Expr.Get expr) {
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }

    @Override
    public Void visitSetExpr(Expr.Set expr) {
        resolve(expr.value);
        resolve(expr.object);
        return null;
    }

    // this works like a variable here - use 'this' as the 'variable' name
    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (currentClass == ClassType.NONE) {
            Lox.error(expr.keyword, "Can't use 'this' outside of a class.");
            return null;
        }
        resolveLocal(expr, expr.keyword);
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitTernaryExpr(Expr.Ternary expr) {
        resolve(expr.check);
        resolve(expr.ifExpr);
        resolve(expr.elseExpr);
        return null;
    }

    // Variable expressions need to have their variables resolved.
    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty() && scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
            Lox.error(expr.name, "Can't read local variable in its own initializer.");
        }
        resolveLocal(expr, expr.name);
        return null;
    }

    // A block statement introduces a new scope for the statements it contains
    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    // permit declaring a class as a local variable, which is uncommon but should be handled correctly
    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        ClassType enclosingClass = currentClass;
        currentClass = ClassType.CLASS;

        declare(stmt.name);
        define(stmt.name);

        // In order for 'this' to resolve to a local variable when it's encountered in a method:
        // Before stepping in and resolving method bodies, push a new scope
        beginScope();
        // and declare & define 'this', as if it were a variable, in the scope
        scopes.peek().put("this", true);

        for (Stmt.Function method : stmt.methods) {
            FunctionType declaration = FunctionType.METHOD;
            resolveFunction(method, declaration);
        }

        endScope();
        currentClass = enclosingClass;

        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    // A function declaration introduces a new scope for its body and binds its parameters in that scope.
    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        if (stmt.name != null) {
            declare(stmt.name);
            define(stmt.name); // a function, unlike variables, may recursively refer to itself in its own body;
        };
        resolveFunction(stmt, FunctionType.FUNCTION);
        return null;
    }

    // either *could* be reached at runtime, so resolve both
    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) resolve(stmt.elseBranch);
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Can't return from top-level code.");
        }
        if (stmt.value != null) {
            resolve(stmt.value);
        }
        return null;
    }

    // A variable declaration adds a new variable to the current scope
    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolve(stmt.condition);
        resolve(stmt.body);
        return null;
    }

    void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }

    private void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    private void resolve(Expr expr) {
        expr.accept(this);
    }

    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    private void endScope() {
        scopes.pop();
    }

    /* Split declaring and defining into 2 steps to handle weird edge cases (which we won't actually allow) like:
    var a = "outer";
    { var a = a; }
     */
    // declaration adds the var to the innermost scope so that it shadows any outer one, and so that we know the var exists. It's marked as 'not ready yet' with a false boolean since the initializer hasn't been resolved
    private void declare(Token name) {
        if (scopes.isEmpty()) return;

        Map<String, Boolean> scope = scopes.peek();

        // Disallow re-declaring a variable in the same local scope
        if (scope.containsKey(name.lexeme)) {
            Lox.error(name, "Already a variable with this name in this scope.");
        }

        scope.put(name.lexeme, false);
    }

    // After declaring, we resolve its initializer expression, and define it, marking it as ready
    private void define(Token name) {
        if (scopes.isEmpty()) return;
        scopes.peek().put(name.lexeme, true);
    }

    // Start at innermost scope, work outwards, looking in each map for a matching name. If var is found, pass resolve it and pass in the number of scopes between the current innermost scope and the scope where the var was found.
    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes.get(i).containsKey(name.lexeme)) {
                interpreter.resolve(expr, scopes.size() - 1 - i);
                return;
            }
        }
    }

    // creates a new scope for the body and bind variables for each of the function's parameters
    // Takes functionType so we can tell whether we're inside a func declaration, which we need to know when resolving a return stmt
    private void resolveFunction(Stmt.Function function, FunctionType type) {
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;
        beginScope();
        for (Token param : function.params) {
            declare(param);
            define(param);
        }
        resolve(function.body);
        endScope();
        currentFunction = enclosingFunction;
    }
}
