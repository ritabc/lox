package com.craftinginterpreters.lox;

import java.util.Arrays;
import java.util.List;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class ParserTest {

    @Test
    void parseGroup() {
        List<Token> tokens = new Scanner("4 + (5 * 2)").scanTokens();
        Expr expression = new Parser(tokens).parse();

        assertEquals("(+ 4.0 (group (* 5.0 2.0)))" ,new AstPrinter().print(expression));
    }

    @Test
    void checkFactorPrecedenceIsHigherThanTerm() {
        List<Token> tokens = new Scanner("4 + 5 * 2").scanTokens();
        Expr expression = new Parser(tokens).parse();

        assertEquals("(+ 4.0 (* 5.0 2.0))" ,new AstPrinter().print(expression));
    }

    @Test
    void minusANegative() {
        List<Token> tokens = new Scanner("10 - -2").scanTokens();
        Expr expression = new Parser(tokens).parse();

        assertEquals("(- 10.0 (- 2.0))" ,new AstPrinter().print(expression));
    }

       @Test
    void recursiveUnary() {
        List<Token> tokens = new Scanner("-!!123").scanTokens();
        Expr expression = new Parser(tokens).parse();

        assertEquals("(- (! (! 123.0)))", new AstPrinter().print(expression));
    }
}