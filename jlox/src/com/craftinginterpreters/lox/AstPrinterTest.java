package com.craftinginterpreters.lox;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class AstPrinterTest {

    @Test
    void prettyPrint() {
        Expr expression =
                new Expr.Binary(
                    new Expr.Unary(
                        new Token(TokenType.MINUS, "-", null, 1),
                        new Expr.Literal(123)),
                    new Token(TokenType.STAR, "*", null, 1),
                    new Expr.Grouping(
                        new Expr.Ternary(
                            new Expr.Literal(true),
                            new Expr.Literal(123),
                            new Expr.Literal("not reached"))));

        assertEquals("(* (- 123) (group (?: true 123 not reached)))", new AstPrinter().print(expression));
    }
}