package com.craftinginterpreters.lox;

import java.util.List;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

class ParserTest {

    @Test
    void parseGroup() {
        List<Token> tokens = new Scanner("4 + (5 * 2);").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(+ 4.0 (group (* 5.0 2.0)));\n" ,new AstPrinter().print(statements.get(0)));
    }

    @Test
    void checkFactorPrecedenceIsHigherThanTerm() {
        List<Token> tokens = new Scanner("4 + 5 * 2;").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(+ 4.0 (* 5.0 2.0));\n" ,new AstPrinter().print(statements.get(0)));
    }

    @Test
    void minusANegative() {
        List<Token> tokens = new Scanner("10 - -2;").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(- 10.0 (- 2.0));\n" ,new AstPrinter().print(statements.get(0)));
    }

    @Test
    void recursiveUnary() {
        List<Token> tokens = new Scanner("-!!123;").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(- (! (! 123.0)));\n", new AstPrinter().print(statements.get(0)));
    }

    @Test
    void testEquality() {
        List<Token> tokens = new Scanner("true == false;").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(== true false);\n", new AstPrinter().print(statements.get(0)));

    }

    @Test
    void ternary() {
        List<Token> tokens = new Scanner("true == false ? \"unseenString\" : 123;").scanTokens();
        List<Stmt> statements = new Parser(tokens).parse();

        assertEquals("(?: (== true false) \"unseenString\" 123.0);\n", new AstPrinter().print(statements.get(0)));
    }

    @Test
    void printStatments() {
        List<Token> tokens1 = new Scanner("print \"one\";").scanTokens();
        List<Stmt> stmts1 = new Parser(tokens1).parse();

        assertEquals("(print \"one\");\n", new AstPrinter().print(stmts1.get(0)));

        List<Token> tokens2 = new Scanner("print 2 + 1;").scanTokens();
        List<Stmt> stmts2 = new Parser(tokens2).parse();

        assertEquals("(print (+ 2.0 1.0));\n", new AstPrinter().print(stmts2.get(0)));
    }
}