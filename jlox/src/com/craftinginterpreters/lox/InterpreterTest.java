package com.craftinginterpreters.lox;

import org.junit.jupiter.api.*;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.file.LinkPermission;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

class InterpreterTest {

    private static final ByteArrayOutputStream stdOutCapture = new ByteArrayOutputStream();
    private static final PrintStream oriStdOut = System.out;

    @BeforeAll
    public static void setupStdOutStream() {
        System.setOut(new PrintStream(stdOutCapture));
    }

    @AfterAll
    public static void restoreStdOutStream() {
        System.setOut(oriStdOut);
    }

    @Test
    void visitLiteralExpr() {
        Expr.Literal exprDouble = new Expr.Literal(123.4);
        Object valueDouble = new Interpreter().visitLiteralExpr(exprDouble);
        assertInstanceOf(Double.class, valueDouble);
        assertEquals(123.4, valueDouble);

        Expr.Literal exprString = new Expr.Literal("helloWorld");
        Object valueString = new Interpreter().visitLiteralExpr(exprString);
        assertInstanceOf(String.class, valueString);
        assertEquals("helloWorld", valueString);

        Expr.Literal exprNil = new Expr.Literal(null);
        Object valueNil = new Interpreter().visitLiteralExpr(exprNil);
        assertNull(valueNil);

        Expr.Literal exprTrue = new Expr.Literal(true);
        Object valueTrue = new Interpreter().visitLiteralExpr(exprTrue);
        assertInstanceOf(Boolean.class, valueTrue);
        assertEquals(true, valueTrue);

        Expr.Literal exprFalse = new Expr.Literal(false);
        Object valueFalse = new Interpreter().visitLiteralExpr(exprFalse);
        assertInstanceOf(Boolean.class, valueFalse);
        assertEquals(false, valueFalse);
    }

    @Test
    void visitUnaryExpr() {
        Expr.Unary exprBang = new Expr.Unary(new Token(TokenType.BANG, "!", null, 1), new Expr.Literal("abc"));
        Object valueBang = new Interpreter().visitUnaryExpr(exprBang);
        assertInstanceOf(Boolean.class, valueBang);
        assertFalse((boolean) valueBang);

        Expr.Unary exprMinus = new Expr.Unary(new Token(TokenType.MINUS, "-", null, 1), new Expr.Literal(123.0));
        Object valueMinus = new Interpreter().visitUnaryExpr(exprMinus);
        assertInstanceOf(Double.class, valueMinus);
        assertEquals(-123.0, valueMinus);

    }

    @Test
    void visitGroupingExpr() {
        Expr.Grouping exprGroup = new Expr.Grouping(new Expr.Binary(new Expr.Literal(4.0), new Token(TokenType.STAR, "*", null, 1), new Expr.Literal(3.0)));
        Object valueGroup = new Interpreter().visitGroupingExpr(exprGroup);

        assertInstanceOf(Double.class, valueGroup);
        assertEquals(12.0, valueGroup);
    }

    @Test
    void visitBinaryExpr() {
        Expr.Binary expr1 = new Expr.Binary(new Expr.Literal(1.0), new Token(TokenType.LESS, "<", null, 1), new Expr.Literal(2.0));
        Object val1 = new Interpreter().visitBinaryExpr(expr1);
        assertInstanceOf(Boolean.class, val1);
        assertTrue((Boolean) val1);

        Expr.Binary expr2 = new Expr.Binary(new Expr.Literal(6.0), new Token(TokenType.PLUS, "+", null, 1), new Expr.Literal(4.0));
        Object val2 = new Interpreter().visitBinaryExpr(expr2);
        assertInstanceOf(Double.class, val2);
        assertEquals(10.0, val2);

        Expr.Binary expr3 = new Expr.Binary(new Expr.Literal("six"), new Token(TokenType.PLUS, "+", null, 1), new Expr.Literal("four"));
        Object val3 = new Interpreter().visitBinaryExpr(expr3);
        assertInstanceOf(String.class, val3);
        assertEquals("sixfour", val3);

        Expr.Binary expr4 = new Expr.Binary(new Expr.Literal(6.0), new Token(TokenType.SLASH, "/", null, 1), new Expr.Literal(2.0));
        Object val4 = new Interpreter().visitBinaryExpr(expr4);
        assertInstanceOf(Double.class, val4);
        assertEquals(3.0, val4);

        Expr.Binary expr5 = new Expr.Binary(new Expr.Literal(6.0), new Token(TokenType.STAR, "*", null, 1), new Expr.Literal(2.0));
        Object val5 = new Interpreter().visitBinaryExpr(expr5);
        assertInstanceOf(Double.class, val5);
        assertEquals(12.0, val5);

        Expr.Binary expr6 = new Expr.Binary(new Expr.Literal(6.0), new Token(TokenType.BANG_EQUAL, "!=", null, 1), new Expr.Literal(6.0));
        Object val6 = new Interpreter().visitBinaryExpr(expr6);
        assertInstanceOf(Boolean.class, val6);
        assertFalse((boolean) val6);

        Expr.Binary expr7 = new Expr.Binary(new Expr.Literal(6.0), new Token(TokenType.EQUAL_EQUAL, "==", null, 1), new Expr.Literal(6.0));
        Object val7 = new Interpreter().visitBinaryExpr(expr7);
        assertInstanceOf(Boolean.class, val7);
        assertTrue((boolean) val7);
    }

    @Test
    void visitTernaryExpr() {
        Expr.Ternary exprIf = new Expr.Ternary(new Expr.Literal(true), new Expr.Literal(123.0), new Expr.Literal("unseenString"));
        Object valIf = new Interpreter().visitTernaryExpr(exprIf);
        assertInstanceOf(Double.class, valIf);
        assertEquals(123.0, valIf);

        Expr.Ternary exprElse = new Expr.Ternary(new Expr.Literal(false), new Expr.Literal(123.0), new Expr.Literal("seenString"));
        Object valElse = new Interpreter().visitTernaryExpr(exprElse);
        assertInstanceOf(String.class, valElse);
        assertEquals("seenString", valElse);
    }

    @Test
    void visitMultipleTernsInOneStmt() {

        // another ternary replaces else (third part)
        // x == 1 ? "one" x == 2 ? "two" : else;
        Interpreter interpreter = new Interpreter();
        List<Stmt> stmts = new Parser(new Scanner("var x = 1;\nprint x == 1 ? \"one\" : x == 2 ? \"two\" : \"else\";").scanTokens()).parse();
        interpreter.interpret(stmts);
        assertEquals("one\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts2 = new Parser(new Scanner("var x = 2;\nprint x == 1 ? \"one\" : x == 2 ? \"two\" : \"else\";").scanTokens()).parse();
        interpreter.interpret(stmts2);
        assertEquals("two\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts3 = new Parser(new Scanner("var x = 0;\nprint x == 1 ? \"one\" : x == 2 ? \"two\" : \"else\";").scanTokens()).parse();
        interpreter.interpret(stmts3);
        assertEquals("else\n", stdOutCapture.toString());
        stdOutCapture.reset();

        // another ternary replaces if (2nd part)
        // bool1 ? bool2 ? "oneAndTwo" : "oneNotTwo" : "notOne"
        // aka
        // bool1 ? (bool2 ? "oneAndTwo" : "oneNotTwo") : "notOne"
        List<Stmt> stmts4 = new Parser(new Scanner("print true ? true ? \"oneAndTwo\" : \"oneNotTwo\" : \"notOne\";").scanTokens()).parse();
        interpreter.interpret(stmts4);
        assertEquals("oneAndTwo\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts5 = new Parser(new Scanner("print true ? false ? \"oneAndTwo\" : \"oneNotTwo\" : \"notOne\";").scanTokens()).parse();
        interpreter.interpret(stmts5);
        assertEquals("oneNotTwo\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts6 = new Parser(new Scanner("print false ? true ? \"oneAndTwo\" : \"oneNotTwo\" : \"notOne\";").scanTokens()).parse();
        interpreter.interpret(stmts6);
        assertEquals("notOne\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void visitMultipleAssignsOneStmt() {
        List<Stmt> stmts = new Parser(new Scanner("var a;\nvar two = 2;\n var three = 3;\nprint a = two = three;\n").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("3\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void blockStatements() {
        List<Stmt> stmts = new Parser(new Scanner("var global = \"outside\";\n" +
                "{\n" +
                "  var local = \"inside\";\n" +
                "  print global + local;\n" +
                "}").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("outsideinside\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts2 = new Parser(new Scanner("var a = \"global a\";\n" +
                "var b = \"global b\";\n" +
                "var c = \"global c\";\n" +
                "{\n" +
                "  var a = \"outer a\";\n" +
                "  var b = \"outer b\";\n" +
                "  {\n" +
                "    var a = \"inner a\";\n" +
                "    print a;\n" +
                "    print b;\n" +
                "    print c;\n" +
                "  }\n" +
                "  print a;\n" +
                "  print b;\n" +
                "  print c;\n" +
                "}\n" +
                "print a;\n" +
                "print b;\n" +
                "print c;").scanTokens()).parse();
        new Interpreter().interpret(stmts2);
        assertEquals("inner a\nouter b\nglobal c\nouter a\nouter b\nglobal c\nglobal a\nglobal b\nglobal c\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void testIfStatement() {
        List<Stmt> stmts = new Parser(new Scanner("var x;\nif (true)\nx = 5;\nelse x = 2; print x;").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("5\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts2 = new Parser(new Scanner("var x;\nif (false)\nx = 5;\nelse x = 2; print x;").scanTokens()).parse();
        new Interpreter().interpret(stmts2);
        assertEquals("2\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts3 = new Parser(new Scanner("var x;\nif (true)\nx = 5;\nprint x;").scanTokens()).parse();
        new Interpreter().interpret(stmts3);
        assertEquals("5\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void testLogicalExprs() {
        List<Stmt> stmts = new Parser(new Scanner("print true and true and false;").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("false\n", stdOutCapture.toString());
        stdOutCapture.reset();

        List<Stmt> stmts2 = new Parser(new Scanner("print true or true or false;").scanTokens()).parse();
        new Interpreter().interpret(stmts2);
        assertEquals("true\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void whileStmt() {
        List<Stmt> stmts = new Parser(new Scanner("var five = 1;\nwhile(five < 5)five = five + 1;print five;").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("5\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void forStmt() { // fib #'s under 10000
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner("var a = 0; var temp; for (var b = 1; a < 10000; b = temp + b) { print a; temp = a; a = b; }").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("0\n" +
                "1\n" +
                "1\n" +
                "2\n" +
                "3\n" +
                "5\n" +
                "8\n" +
                "13\n" +
                "21\n" +
                "34\n" +
                "55\n" +
                "89\n" +
                "144\n" +
                "233\n" +
                "377\n" +
                "610\n" +
                "987\n" +
                "1597\n" +
                "2584\n" +
                "4181\n" +
                "6765\n", stdOutCapture.toString());
        stdOutCapture.reset();

    }

    @Test
    void funcDeclStmtAndCallExpr() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner("fun fib(n) { if (n <= 1) return n; return fib(n-2) + fib(n-1); }\nfor (var i = 0; i < 20; i = i+1) { print fib(i); }\n").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("0\n1\n1\n2\n3\n5\n8\n13\n21\n34\n55\n89\n144\n233\n377\n610\n987\n1597\n2584\n4181\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void funcClosures() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner("fun makeCounter() { var i = 0; fun count() { i = i + 1; print i; } return count; }\n" +
                "var counter = makeCounter(); counter(); counter();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("1\n2\n", stdOutCapture.toString());
        stdOutCapture.reset();
    }

    @Test
    void funcThis() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Cake { " +
                        "   taste() {" +
                        "       var adjective = \"delicious\"; " +
                        "       print \"The \" + this.flavor + \" cake is \" + adjective + \"!\";" +
                        "   }" +
                        "} " +
                        "var cake = Cake(); " +
                        "cake.flavor = \"German chocolate\"; " +
                        "cake.taste();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("The German chocolate cake is delicious!\n", stdOutCapture.toString());
    }

    @Test
    void storeMethodAsVariable() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Cake {howMuchFrosting() {print \"Lots!\";}}" +
                        "var cake = Cake();" +
                        "var frostingMethod = cake.howMuchFrosting;" +
                        "frostingMethod();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("Lots!\n", stdOutCapture.toString());
    }

    @Test
    void storeFunctionInInstanceField() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Box {}" +
                        "fun notMethod(argument) {" +
                            "print \"called function with \" + argument;" +
                        "}" +
                        "var box = Box();" +
                        "box.function = notMethod;" +
                        "box.function(\"argument\");").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("called function with argument\n", stdOutCapture.toString());
    }

    @Test
    void thisBoundToOriginalInstanceWhenMethodFirstGrabbed() {
        Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Person {" +
                        "   sayName() {" +
                        "       print this.name;" +
                        "   }" +
                        "}" +
                        "var jane = Person();" +
                        "jane.name = \"Jane\";" +
                        "" +
                        "var bill = Person();" +
                        "bill.name = \"Bill\";" +
                        "" +
                        "bill.sayName = jane.sayName;" +
                        "bill.sayName();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("Jane\n", stdOutCapture.toString());
    }

    @Test
    void inheritingMethods() {
                Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Doughnut {" +
                        "   cook() {" +
                        "       print \"Fry until golden brown.\";" +
                        "   }" +
                        "}" +
                        "class BostonCream < Doughnut {}" +
                        "BostonCream().cook();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("Fry until golden brown.\n", stdOutCapture.toString());
    }

    @Test
    void callingSuperMethods() {
                Interpreter interpreter = new Interpreter();
        Resolver resolver = new Resolver(interpreter);
        List<Stmt> stmts = new Parser(new Scanner(
                "class Doughnut {" +
                        "   cook() {" +
                        "       print \"Fry until golden brown.\";" +
                        "   }" +
                        "}" +
                        "class BostonCream < Doughnut {" +
                        "   cook() {" +
                        "       super.cook();" +
                        "       print \"Pipe full of custard and coat with chocolate.\";" +
                        "   }" +
                        "}" +
                        "BostonCream().cook();").scanTokens()).parse();
        resolver.resolve(stmts);
        interpreter.interpret(stmts);
        assertEquals("Fry until golden brown.\n" +
                "Pipe full of custard and coat with chocolate.\n", stdOutCapture.toString());
    }
}