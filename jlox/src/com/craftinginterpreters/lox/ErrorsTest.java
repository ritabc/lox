package com.craftinginterpreters.lox;

import org.junit.jupiter.api.*;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class ErrorsTest {
    private final static ByteArrayOutputStream errContent = new ByteArrayOutputStream();
    private final static PrintStream originalErr = System.err;

    @BeforeAll
    public static void setupErrorStream() {
        System.setErr(new PrintStream(errContent));
    }

    @AfterAll
    public static void restoreErrorStream() {
        System.setErr(originalErr);
    }

    @AfterEach
    public void debug() {
        errContent.reset();
    }

    @Test
    void scanUnterminatedString() {
        Scanner scanner = new Scanner("\"abc");
        scanner.scanTokens();

        assertEquals("[line 1] Error: Unterminated string.\n", errContent.toString());
    }

    @Test
    void scanUnterminatedMultiLineComment() {
        Scanner unterminatedCommentScanner = new Scanner("/* an unterminated comment");
        unterminatedCommentScanner.scanTokens();

        assertEquals("[line 1] Error: Unterminated multi-line comment.\n", errContent.toString());
    }

    @Test
    void parseUnterminatedGroupingExpr() {
        List<Token> tokens = new Scanner("1 + ( 2 * 3").scanTokens();
        new Parser(tokens).parse();
        assertEquals("[line 1] Error at end: Expect ')' after expression.\n", errContent.toString());
    }

    @Test
    void parseTernaryExprWithNoColon() {
        List<Token> tokens = new Scanner("true ? 123").scanTokens();
        new Parser(tokens).parse();
        assertEquals("[line 1] Error at end: Expect ':' in ternary expression.\n", errContent.toString());
    }

    @Test
    void evaluateAdditionOfNumberToString() {
        List<Stmt> stmts = new Parser(new Scanner("6 + \"four\";").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("[line 1] Operands must be two numbers or two strings\n", errContent.toString());
    }

    @Test
    void evaluateNegativeBoolean() {
        List<Stmt> stmts = new Parser(new Scanner("-!!true;").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("[line 1] Operand must be a number.\n", errContent.toString());
    }

    @Test
    void evaluateMultiplyStrings() {
        List<Stmt> stmts = new Parser(new Scanner("\"abc\" * \"def\";").scanTokens()).parse();
        new Interpreter().interpret(stmts);
        assertEquals("[line 1] Operands must be numbers.\n", errContent.toString());
    }

}
