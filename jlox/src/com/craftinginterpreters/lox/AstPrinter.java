package com.craftinginterpreters.lox;

public class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String>
{
    String print(Expr expr) {
        return expr.accept(this);
    }

    String print(Stmt stmt) {
        return stmt.accept(this);
    }

    @Override
    public String visitTernaryExpr(Expr.Ternary expr) {
        return parenthesize("?:", expr.check, expr.ifExpr, expr.elseExpr);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        if (expr.value instanceof String) return "\"" + expr.value + "\"";
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    // `x` ==> `x`
    @Override
    public String visitVariableExpr(Expr.Variable expr) {
        return expr.name.toString();
    }

    /*
    What types of expression stmts are there, how to handle each?
    // `5 + 3 ;`               ==> `(+ 5 3);\n`
    // `true ? 123 : "unseen"` ==> `(?: true 123 "unseen");\n`
    // `-!!true`               ==> `(- (! (! true)));\n`
    */
    @Override
    public String visitExpressionStmt(Stmt.Expression stmt) {
        return print(stmt.expression) + ";\n";
    }

    /*
    What types of print stmts are there, how to handle each?
    // `print "one";`    ==> (print "one");\n"
    // `print 2 + 1;`    ==> (print (+ 2 1));\n"
    The `(print...` notation makes it appear as an expression, which isn't ideal,
    but since the purpose of keeping this AstPrinter class is to get more familiar with the visitor pattern, that's okay.
    */
    @Override
    public String visitPrintStmt(Stmt.Print stmt) {
        return "(print " + print(stmt.expression) + ");\n";
    }

    // `var x = 5;`  ==> `(var x 5)`
    @Override
    public String visitVarStmt(Stmt.Var stmt) {
        return parenthesize(stmt.name.lexeme, stmt.initializer);
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }

        builder.append(")");

        return builder.toString();
    }
}
