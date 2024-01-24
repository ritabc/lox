package com.craftinginterpreters.lox;

import java.util.ArrayList;
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

    public List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while (!isAtEnd()) {
            statements.add(declaration());
        }
        return statements;
    }

    private Stmt declaration() {
        try {
            if (match(TokenType.VAR)) return varDeclaration();

            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt varDeclaration() {
        Token name = consume(TokenType.IDENTIFIER, "Expect variable name.");

        Expr initializer = null;
        if (match(TokenType.EQUAL)) {
            initializer = expression();
        }

        consume(TokenType.SEMICOLON, "Expect ';' after variable declaration.");

        return new Stmt.Var(name, initializer);
    }

    private Stmt statement() {
        if (match(TokenType.PRINT)) return printStatement();

        return expressionStatement();
    }

    // emit the syntax tree for a print statement
    private Stmt printStatement() {
        Expr value = expression();
        consume(TokenType.SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(TokenType.SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    // grammar rule: expression -> equality ;
    private Expr expression() {
        return assignment();
    }


    /*
    Where l-values & r-values come in to play.
    All our other expr's are r-values.
    l-values are things that can be and are on the left of th '=' in assignment.
    ```
    var a = 0;
    a = 1;
    print a+1 // 2
    ```
    On the 3rd line, the interpreter knows a is an r-value.
    It evaluates (looks up) the value of a.
    On the 2nd line, it needs to know to NOT evaluate a.
    Instead, it needs to know to set a.
    B/c of this, Expr.Assign has a Token for the left hand side, and an Expr on the right.
    TODO add parse test for makeList().head.next = node; (after adding obj fields capability)

     */
    private Expr assignment() {
        Expr expr = ternary();

        if (match(TokenType.EQUAL)) {
            Token equals = previous();
            // since it's right-associative, recursively call self
            // allows us to do things like a = two = three. Where in the end, a will equal 3.
            Expr value = assignment();

            if (expr instanceof Expr.Variable) {
                // convert the expr on the left of the '=' to an l-value by getting it's name and creating a new Token
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, value);
            }

            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    // right assoc grammar rule: ternary -> equality ("?" ternary ":" ternary )* ; (All but 1st part can be another ternary)
    private Expr ternary() {
        Expr expr = equality();

        if (match(TokenType.QUESTION)) {
            Expr ifExpr = ternary();
            consume(TokenType.COLON, "Expect ':' in ternary expression.");
            Expr elseExpr = ternary();
            expr = new Expr.Ternary(expr, ifExpr, elseExpr);
        }

        return expr;
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

        if (match(TokenType.IDENTIFIER)) {
            return new Expr.Variable(previous());
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
