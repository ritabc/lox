package com.craftinginterpreters.lox;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestFactory;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

class ScannerTest
{
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

    @Test
    void testVarDeclaration() {
        Scanner scanner = new Scanner("var language = \"lox\"");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.VAR, TokenType.IDENTIFIER, TokenType.EQUAL, TokenType.STRING, TokenType.EOF);
        String expectedString = "lox";

        checkTokenTypes(tokens, expectedTokenTypes);
        assertEquals(tokens.get(3).literal, "lox");
    }

    @Test
    void testSingleCharLexemes() {
        Scanner scanner = new Scanner("/(){},.-+;*");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.SLASH, TokenType.LEFT_PAREN, TokenType.RIGHT_PAREN, TokenType.LEFT_BRACE, TokenType.RIGHT_BRACE, TokenType.COMMA, TokenType.DOT, TokenType.MINUS, TokenType.PLUS, TokenType.SEMICOLON, TokenType.STAR, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);
    }

    @Test
    void testDoubleCharLexemes() {
        Scanner scanner = new Scanner("!===<=>==!<>");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL, TokenType.LESS_EQUAL, TokenType.GREATER_EQUAL, TokenType.EQUAL, TokenType.BANG, TokenType.LESS, TokenType.GREATER, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);
    }

    @Test
    void testLineComment() {
        Scanner scanner = new Scanner("== // a comment here");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.EQUAL_EQUAL, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);
    }

    @Test
    void testWhitespace() {
        Scanner scanner = new Scanner("(   {\r\t}\n)");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.LEFT_PAREN, TokenType.LEFT_BRACE, TokenType.RIGHT_BRACE, TokenType.RIGHT_PAREN, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);
        assertEquals(tokens.get(2).line, 1);
        assertEquals(tokens.get(3).line, 2);
    }

    @Test
    void multiLineStringLiteral() {
        Scanner scanner = new Scanner("\"multiline\nstring\"");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.STRING, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);

        assertEquals(tokens.get(0).literal, "multiline\nstring");
    }

    @Test
    void errorDueToUnterminatedString() {
        Scanner scanner = new Scanner("\"abc");
        List<Token> tokens = scanner.scanTokens();

        assertEquals("[line 1] Error: Unterminated string.\n", errContent.toString());
    }

    @Test
    void numberLiteral() {
        Scanner scanner = new Scanner("1234 + 12.34");
        List<Token> tokens = scanner.scanTokens();

        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.NUMBER, TokenType.PLUS, TokenType.NUMBER, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);

        assertEquals(1234.0, tokens.get(0).literal);
        assertEquals(12.34, tokens.get(2).literal);
    }

    @Test
    void numbersWithLeadingTrailingDecimal() {
        // should be recognized as a number, and the dot should be a separate token
        Scanner scannerTrailing = new Scanner("1234.");
        List<Token> tokensTrailing = scannerTrailing.scanTokens();

        List<TokenType> expectedTokenTypesTrailing = Arrays.asList(TokenType.NUMBER, TokenType.DOT, TokenType.EOF);

        checkTokenTypes(tokensTrailing, expectedTokenTypesTrailing);

        Scanner scannerLeading = new Scanner(".1234");
        List<Token> tokensLeading = scannerLeading.scanTokens();

        List<TokenType> expectedTokenTypesLeading = Arrays.asList(TokenType.DOT, TokenType.NUMBER, TokenType.EOF);

        checkTokenTypes(tokensLeading, expectedTokenTypesLeading);
    }

    @Test
    void keywords() {
        Scanner scanner = new Scanner("and class else false for fun if nil or print return super this true var while");
        List<Token> tokens = scanner.scanTokens();
        List<TokenType> expectedTokenTypes = Arrays.asList(TokenType.AND, TokenType.CLASS, TokenType.ELSE, TokenType.FALSE, TokenType.FOR, TokenType.FUN, TokenType.IF, TokenType.NIL, TokenType.OR, TokenType.PRINT, TokenType.RETURN, TokenType.SUPER, TokenType.THIS, TokenType.TRUE, TokenType.VAR, TokenType.WHILE, TokenType.EOF);

        checkTokenTypes(tokens, expectedTokenTypes);
    }

    @Test
    void blockComments() {
        // one line
        Scanner oneLineScanner = new Scanner("var x = 123 /* */ + 456");
        List<Token> oneLineTokens = oneLineScanner.scanTokens();

        List<TokenType> expectedTokenTypesOneLine = Arrays.asList(TokenType.VAR, TokenType.IDENTIFIER, TokenType.EQUAL, TokenType.NUMBER, TokenType.PLUS, TokenType.NUMBER, TokenType.EOF);

        checkTokenTypes(oneLineTokens, expectedTokenTypesOneLine);
        assertEquals(oneLineTokens.get(0).line, 1);
        assertEquals(oneLineTokens.get(oneLineTokens.size()-1).line, 1);

        // multiple lines
        Scanner multiLineScanner = new Scanner("var x = 123 /*\n\n*/ + 456");
        List<Token> multiLineTokens = multiLineScanner.scanTokens();

        List<TokenType> expectedTokenTypesMultiLine = Arrays.asList(TokenType.VAR, TokenType.IDENTIFIER, TokenType.EQUAL, TokenType.NUMBER, TokenType.PLUS, TokenType.NUMBER, TokenType.EOF);

        checkTokenTypes(multiLineTokens, expectedTokenTypesMultiLine);
        assertEquals(1, multiLineTokens.get(0).line);
        assertEquals(3, multiLineTokens.get(multiLineTokens.size()-3).line);

        // "/* a comment with an asterisk at the end **/"
        Scanner endsWithAsteriskScanner = new Scanner("123 /* a comment with an asterisk at the end **/");
        List<Token> endsWithAsteriskTokens = endsWithAsteriskScanner.scanTokens();

        List<TokenType> expectedEndsWithAsteriskTokens = Arrays.asList(TokenType.NUMBER, TokenType.EOF);
        checkTokenTypes(endsWithAsteriskTokens, expectedEndsWithAsteriskTokens);

        // "/* an unterminated comment"
        Scanner unterminatedCommentScanner = new Scanner("/* an unterminated comment");
        List<Token> unterminatedCommentTokens = unterminatedCommentScanner.scanTokens();

        assertEquals("[line 1] Error: Unterminated multi-line comment.\n", errContent.toString());
    }



    private void checkTokenTypes(List<Token> tokens, List<TokenType> expectedTypes) {
        assertEquals(tokens.size(), expectedTypes.size());
        for (int i = 0; i < tokens.size(); i++) {
            assertEquals(expectedTypes.get(i), tokens.get(i).type);
        }
    }
}