package com.craftinginterpreters.lox;

import javax.swing.*;
import java.util.List;

public class Parser {

    /* used for unwinding the parser
    With recursive descent, the parser's state isn't stored explicitly in fields. If we want to synchronize at each boundary between statements, how do we unwind and reset the parser state (which we'll want to do in order to report as many distinct
     */
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;

    // The next token eagerly waiting to be parsed
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    public Expr parse() {
        try {
            return expression();
        } catch (ParseError error) {
            return null;
        }
    }

    // grammar rule: expression -> equality ;
    private Expr expression() {
        return equality();
    }

    // Equality or anything of higher precedence (comparison, etc)
    // grammar rule: equality -> comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        Expr expr =  comparison();

        while (match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // Comparison or anything of higher precedence (term, etc)
    // grammar rule: comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    private Expr comparison() {
        Expr expr = term();

        while (match(TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // Term or anything of higher precedence (factor, etc)
    // grammar rule: term → factor ( ( "+" | "-" ) factor )* ;
    private Expr term() {
        Expr expr = factor();

        while (match(TokenType.PLUS, TokenType.MINUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // Factor or anything of higher precedence (unary, etc)
    // grammar rule: factor → unary ( ( "*" | "/" ) unary )* ;
    private Expr factor() {
        Expr expr = unary();

        while (match(TokenType.STAR, TokenType.SLASH)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // Unary or primary
    // grammar rule: unary -> (( "!" "-" ) unary) | primary ;
    private Expr unary() {
        if (match(TokenType.BANG, TokenType.MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }
        return primary();
    }

    // Primary
    // grammar rule: primary → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
    private Expr primary() {
        if (match(TokenType.FALSE)) return new Expr.Literal(false);
        if (match(TokenType.TRUE)) return new Expr.Literal(true);
        if (match(TokenType.NIL)) return new Expr.Literal(null);

        if (match(TokenType.NUMBER, TokenType.STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(TokenType.LEFT_PAREN)) {
            Expr expr = expression();
            consume(TokenType.RIGHT_PAREN, "Expect ')' after expression."); // If we don't find a RIGHT_PAREN, we throw an error
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    /* Functions that allow the parser to consume the list of tokens.
    Similar to the scanner utility funcs, expect instead of reading characters, we read tokens. */

    // Is the current token any of the given types? If so, advance
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    // If we're currently at a Token of type, advance. Otherwise, throw error with msg
    private Token consume(TokenType type, String msg) {
        if (check(type)) return advance();

        throw error(peek(), msg);
    }

    // Is the current token the given type?
    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    // Consume the current token (return token at current, then increment current)
    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    // Get current token
    private Token peek() {
        return tokens.get(current);
    }

    // Get previous token
    private Token previous() {
        return tokens.get(current - 1);
    }

    // tell user of the error, create and return a ParseError
    // returns the error instead of throwing it b/c we want to let the calling parser method to decide whether to unwind or not
    private ParseError error(Token token, String msg) {
        Lox.error(token, msg);
        return new ParseError();
    }

    // discard tokens until we're right at the beginning of the next statement
    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == TokenType.SEMICOLON) return;

            switch (peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
            }

            advance();
        }
    }

    // Check whether current token is EOF
    private boolean isAtEnd() {
        return peek().type == TokenType.EOF;
    }
}
