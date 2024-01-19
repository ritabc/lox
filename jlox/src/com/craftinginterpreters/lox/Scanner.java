package com.craftinginterpreters.lox;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


public class Scanner
{
    private final String source;
    private final List<Token> tokens = new ArrayList<>();

    // fields to keep track of where the scanner is
    private int start = 0; // idx into the string being scanned, is the first character of the current lexeme
    private int current = 0; // idx into the string being scanned, is the current character of the current lexeme
    private int line = 1; // which source line are we on?

    private static final Map<String, TokenType> keywords;
    static {
        keywords = new HashMap<>();
        keywords.put("and", TokenType.AND);
        keywords.put("class", TokenType.CLASS);
        keywords.put("else", TokenType.ELSE);
        keywords.put("false", TokenType.FALSE);
        keywords.put("for", TokenType.FOR);
        keywords.put("fun", TokenType.FUN);
        keywords.put("if", TokenType.IF);
        keywords.put("nil", TokenType.NIL);
        keywords.put("or", TokenType.OR);
        keywords.put("print", TokenType.PRINT);
        keywords.put("return", TokenType.RETURN);
        keywords.put("super", TokenType.SUPER);
        keywords.put("this", TokenType.THIS);
        keywords.put("true", TokenType.TRUE);
        keywords.put("var", TokenType.VAR);
        keywords.put("while", TokenType.WHILE);
    }

    Scanner(String source) {
        this.source = source;
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            // We're at the beginning of the next lexeme
            start = current;
            scanToken();
        }

        tokens.add(new Token(TokenType.EOF, "", null, line));
        return tokens;
    }

    private void scanToken() {
        char c = advance();
        switch (c) {

            case '(': addToken(TokenType.LEFT_PAREN); break;
            case ')': addToken(TokenType.RIGHT_PAREN); break;
            case '{': addToken(TokenType.LEFT_BRACE); break;
            case '}': addToken(TokenType.RIGHT_BRACE); break;
            case ',': addToken(TokenType.COMMA); break;
            case '.': addToken(TokenType.DOT); break;
            case '-': addToken(TokenType.MINUS); break;
            case '+': addToken(TokenType.PLUS); break;
            case ';': addToken(TokenType.SEMICOLON); break;
            case '*': addToken(TokenType.STAR); break;

            case '!':
                addToken(match('=') ? TokenType.BANG_EQUAL : TokenType.BANG);
                break;
            case '=':
                addToken(match('=') ? TokenType.EQUAL_EQUAL : TokenType.EQUAL);
                break;
            case '<':
                addToken(match('=') ? TokenType.LESS_EQUAL : TokenType.LESS);
                break;
            case '>':
                addToken(match('=') ? TokenType.GREATER_EQUAL : TokenType.GREATER);
                break;
            case '/':
                if (match('/')) {
                    // these types of comment go til the end of the line
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else if (match('*')) {
                    multiLineComment();
                }
                else {
                    addToken(TokenType.SLASH);
                }
                break;

            // Ignore whitespace
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                line++;
                break;

            // Strings
            case '"': string(); break;

            // handle numbers, identifiers (and reserved words), and errors in default
            default:
                if (isDigit(c)) {
                    number();
                } else if (isAlpha(c)) { // identifiers and reserved words begin with a letter or '_'
                    identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                }
            break;
        }
    }

    // consume the whole string
    private void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }

        // Handle the closing "
        advance();

        // trim the surrounding quotes
        String value = source.substring(start+1, current-1);
        addToken(TokenType.STRING, value);
    }

    // consume the entire comment
    private void multiLineComment() {
        // while we still have more to process, and we haven't reached the end of our comment
        while ((peek() != '*' || (peek() == '*' && peekNext() != '/')) && !isAtEnd()) {
            if (peek() == '\n') line ++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated multi-line comment.");
            return;
        }

        if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
        }
    }

    // consume the whole number
    private void number() {
        while (isDigit(peek())) advance();

        // Look for a fractional part
        // consisting of '.' then another digit
        if (peek() == '.' && isDigit(peekNext())) {
            // consume the '.'
            advance();

            while (isDigit(peek())) advance();
        }

        addToken(TokenType.NUMBER, Double.parseDouble(source.substring(start, current)));
    }

    private void identifier() {
        while (isAlphaNumeric(peek())) advance();

        String text = source.substring(start, current);
        TokenType type = keywords.get(text);
        if (type == null) type = TokenType.IDENTIFIER;
        addToken(type);
    }

    // bread & butter helper for consuming the input string
    // returns current char then advances it
    private char advance() {
        return source.charAt(current++);
    }

     // look ahead at current char
     // without consuming it (aka w/o advancing current)
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    // peekNext allows peeking 2 ahead w/o consuming
    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    // match asks: is the next char 'expected'?
    // if so, consume it (aka advance current pointer)
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) != expected) return false;

        current++;
        return true;
    }


    // helpers for producing output (Tokens)
    private void addToken(TokenType type) {
        addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }

    private boolean isAtEnd() {
        return current >= source.length();
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
}
