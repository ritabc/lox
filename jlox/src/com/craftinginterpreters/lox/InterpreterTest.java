package com.craftinginterpreters.lox;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class InterpreterTest {

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

    // testing by creating the Expr from scratch was deemed too tedious. end-2-end testing here instead.
    @Test
    void visitBinaryExpr() {
        Object val1 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("1 < 2").scanTokens()).parse());
        assertInstanceOf(Boolean.class, val1);
        assertTrue((Boolean) val1);

        Object val2 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("1 > 2").scanTokens()).parse());
        assertFalse((Boolean) val2);

        Object val3 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("1 >= 1").scanTokens()).parse());
        assertTrue((Boolean) val3);

        Object val4 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("1 <= 0").scanTokens()).parse());
        assertFalse((Boolean) val4);

        Object val5 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("5 - 4").scanTokens()).parse());
        assertInstanceOf(Double.class, val5);
        assertEquals(1.0, val5);

        Object val6 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("6 + 4").scanTokens()).parse());
        assertInstanceOf(Double.class, val6);
        assertEquals(10.0, val6);

        Object val7 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("\"six\" + \"four\"").scanTokens()).parse());
        assertInstanceOf(String.class, val7);
        assertEquals("sixfour", val7);

        Object val8 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("6 / 2").scanTokens()).parse());
        assertInstanceOf(Double.class, val8);
        assertEquals(3.0, val8);

        Object val9 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("6 * 2").scanTokens()).parse());
        assertInstanceOf(Double.class, val9);
        assertEquals(12.0, val9);

        Object val10 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("6 != 6").scanTokens()).parse());
        assertInstanceOf(Boolean.class, val10);
        assertFalse((boolean) val10);

        Object val11 = new Interpreter().visitBinaryExpr((Expr.Binary) new Parser(new Scanner("6 == 6").scanTokens()).parse());
        assertInstanceOf(Boolean.class, val11);
        assertTrue((boolean) val11);
    }

    @Test
    void visitTernaryExpr() {
        Object valIf = new Interpreter().visitTernaryExpr((Expr.Ternary) new Parser(new Scanner("true ? 123 : \"unseenString\"").scanTokens()).parse());
        assertInstanceOf(Double.class, valIf);
        assertEquals(123.0, valIf);

        Object valElse = new Interpreter().visitTernaryExpr((Expr.Ternary) new Parser(new Scanner("false ? 123 : \"seenString\"").scanTokens()).parse());
        assertInstanceOf(String.class, valElse);
        assertEquals("seenString", valElse);
    }
}