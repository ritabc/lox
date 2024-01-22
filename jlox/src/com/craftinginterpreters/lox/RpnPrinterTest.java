package com.craftinginterpreters.lox;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class RpnPrinterTest {
    @Test
    void RpnPrint() {
        Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1),
        new Expr.Grouping(
            new Expr.Literal("str")));

        assertEquals(new RpnPrinter().print(expression), "123 ~ str *");

        Expr ternExpr = new Expr.Ternary(
                new Expr.Literal(true), new Expr.Literal(123), new Expr.Literal("unseenString")
        );

        assertEquals(new RpnPrinter().print(ternExpr), "true 123 unseenString ?:");
    }
}
