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
    public String visitAssignExpr(Expr.Assign expr) {
        return "(= " + expr.name.lexeme + " " + expr.value.accept(this) + ")";
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
    public String visitCallExpr(Expr.Call expr) {
        StringBuilder sb = new StringBuilder();
        sb.append("(call");
        sb.append(expr.callee.accept(this));
        for (Expr arg : expr.arguments) {
            sb.append(" ").append(arg.accept(this));
        }
        sb.append(")");
        return sb.toString();
    }

    @Override
    public String visitAnonFunExpr(Expr.AnonFun expr) {
        // TODO
        return null;
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

    /*
    val1 and val2 ==> (and val1 val2)
     */
    @Override
    public String visitLogicalExpr(Expr.Logical expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
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

    @Override
    public String visitBlockStmt(Stmt.Block stmt) {
        StringBuilder sb = new StringBuilder();
        sb.append("(block ");
        for (Stmt s : stmt.statements) {
            sb.append(s.accept(this));
        }
        sb.append(")");
        return sb.toString();
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


    /* func sayHi(first, last) {
    print "Hi, " + first + " " + last + "!";
    }   ====> (func (params first last) (body (print "...")))
     */
    @Override
    public String visitFunDeclStmt(Stmt.FunDecl stmt) {
        StringBuilder sb = new StringBuilder();
        sb.append("(func (params ");
        for (Token param : stmt.params) {
            sb.append(" ").append(param.lexeme);
        }
        sb.append(") (body ");
        for (Stmt bodyStmt : stmt.body) {
            sb.append(" ").append(print(bodyStmt));
        }
        sb.append("))");
        return sb.toString();
    }


    /*
    if (check) thenStatement else elseStmt
    ==>
    (if checkExpr thenStmt elseStmt)
     */
    @Override
    public String visitIfStmt(Stmt.If stmt) {
        return "(if " + print(stmt.condition) + " " + print(stmt.thenBranch).strip() + (stmt.elseBranch == null ? "" : " " + print(stmt.elseBranch).strip()) + ");\n";
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

    @Override
    public String visitReturnStmt(Stmt.Return stmt) {
        return parenthesize(stmt.keyword.lexeme, stmt.value);
    }

    // `var x = 5;`  ==> `(var x 5)`
    @Override
    public String visitVarStmt(Stmt.Var stmt) {
        return "(var " + stmt.name.lexeme + " " + stmt.initializer.accept(this) + ");\n";
    }

    @Override
    public String visitWhileStmt(Stmt.While stmt) {
        return "(while " + print(stmt.condition) + " " + print(stmt.body) + ");\n";
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
