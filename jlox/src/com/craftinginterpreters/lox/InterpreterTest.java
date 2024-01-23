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
}