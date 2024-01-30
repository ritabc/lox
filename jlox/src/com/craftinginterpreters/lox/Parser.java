package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.Arrays;
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
            if (match(TokenType.CLASS)) return classDeclaration();
            if (match(TokenType.FUN)) return function("function");
            if (match(TokenType.VAR)) return varDeclaration();

            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt classDeclaration() {
        Token name = consume(TokenType.IDENTIFIER, "Expect class name.");
        consume(TokenType.LEFT_BRACE, "Expect '{' before class body.");

        List<Stmt.Function> methods = new ArrayList<>();
        while (!check(TokenType.RIGHT_BRACE) && !isAtEnd()) {
            methods.add(function("method"));
        }

        consume(TokenType.RIGHT_BRACE, "Expect '}' after class body.");

        return new Stmt.Class(name, methods);
    }

    private Stmt.Function function(String kind) {
        Token name = consume(TokenType.IDENTIFIER, "Expect " + kind + " name.");
        consume(TokenType.LEFT_PAREN, "Expect '(' after " + kind + " name.");
        List<Token> parameters = new ArrayList<>();
        if (!check(TokenType.RIGHT_PAREN)) { // handles zero-param case
            do {
                if (parameters.size() >= 255) {
                    error(peek(), "Can't have more than 255 parameters.");
                }

                parameters.add(consume(TokenType.IDENTIFIER, "Expect parameter name." ));
            } while (match(TokenType.COMMA));
        }
        consume(TokenType.RIGHT_PAREN, "Expect ')' after parameters.");

        consume(TokenType.LEFT_BRACE, "Expect '{' before " + kind + " body.");
        List<Stmt> body = block();

        return new Stmt.Function(name, parameters, body);
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
        if (match(TokenType.FOR)) return forStatement();
        if (match(TokenType.IF)) return ifStatement();
        if (match(TokenType.PRINT)) return printStatement();
        if (match(TokenType.RETURN)) return returnStatement();
        if (match(TokenType.WHILE)) return whileStatement();
        if (match(TokenType.LEFT_BRACE)) return new Stmt.Block(block());

        return expressionStatement();
    }

    // Use desugaring (to learn the concept, not necessarily b/c it makes the most sense here)
    // There will be no AST node for a forStatement, instead we'll desugar for loops to while loops and other statements we already handle
    private Stmt forStatement() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after 'for'.");

        // initializer is optional, or it'll be a varDecl or exprSt
        Stmt initializer;
        if (match(TokenType.SEMICOLON)) {
            initializer = null;
        } else if (match(TokenType.VAR)) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        // condition is also optional, or an expr, but either way we must have a ';'
        Expr condition = null;
        if (!check(TokenType.SEMICOLON)) {
            condition = expression();
        }
        consume(TokenType.SEMICOLON, "Expect ';' after loop condition.");

        Expr increment = null;
        if (!check(TokenType.RIGHT_PAREN)) {
            increment = expression();
        }
        consume(TokenType.RIGHT_PAREN, "Expect ')' after for clauses.");

        Stmt body = statement();

        // if there's an increment, replace the body with a list of stmts containing the original body + the increment
        // do this first so when we encapsulate in while loop, each iteration has the increment
        if (increment != null) {
            body = new Stmt.Block(Arrays.asList(body, new Stmt.Expression(increment)));
        }

        // take condition and make a while loop with that, and the body
        // without an explicit condition, make it an infinite loop
        if (condition == null) condition = new Expr.Literal(true);
        body = new Stmt.While(condition, body);

        // replace entire while loop statement with (initializer (if there is one), whileLoop)
        // do this last, so its only executed once (outside of while loop)
        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }

        return body;
    }

    // NB: To handle the dangling else problem of which if stmt does the else belong to in: if (first) if (second) whenTrue; else whenFalse();
    // we just bind the dangling else to the closest if, which the following parser does
    private Stmt ifStatement() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(TokenType.RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt thenBranch = statement();
        Stmt elseBranch = null;
        if (match(TokenType.ELSE)) {
            elseBranch = statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    // emit the syntax tree for a print statement
    private Stmt printStatement() {
        Expr value = expression();
        consume(TokenType.SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    private Stmt returnStatement() {
        Token keyword = previous();
        Expr value = null;

        // Easier to check for end of return statement, since any sort of expression could be returned
        if (!check(TokenType.SEMICOLON)) {
            value = expression();
        }

        consume(TokenType.SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
    }

    private Stmt whileStatement() {
        consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(TokenType.RIGHT_PAREN, "Expect ')' after condition.");
        Stmt body = statement();

        return new Stmt.While(condition, body);
    }

    // Returns a list of statements enclosed in a scope block or function body
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();

        while (!check(TokenType.RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(TokenType.RIGHT_BRACE, "Expect '}' after block.");
        return statements;
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
                // convert the expr on the left of the '=' to an l-value by getting its name and creating a new Token
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, value);
            } else if (expr instanceof Expr.Get) {
                Expr.Get get = (Expr.Get)expr;
                return new Expr.Set(get.object, get.name, value);
            }

            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    // right assoc grammar rule: ternary -> equality ("?" ternary ":" ternary )* ; (All but 1st part can be another ternary)
    private Expr ternary() {
        Expr expr = or();

        if (match(TokenType.QUESTION)) {
            Expr ifExpr = ternary();
            consume(TokenType.COLON, "Expect ':' in ternary expression.");
            Expr elseExpr = ternary();
            expr = new Expr.Ternary(expr, ifExpr, elseExpr);
        }

        return expr;
    }

    private Expr or() {
        Expr expr = and();

        while (match(TokenType.OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    private Expr and() {
        Expr expr = equality();

        while (match(TokenType.AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
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
        return call();
    }

    // parse the call expression using the previously parsed expression as the callee
    // return the expr, which'll be the new expr and then we loop to see if that result is called
    private Expr call() {
        Expr expr = primary();

        while (true) {
            if (match(TokenType.LEFT_PAREN)) {
                expr = finishCall(expr);
            } else if (match(TokenType.DOT)) {
                Token name = consume(TokenType.IDENTIFIER, "Expect property name after '.'.");
                expr = new Expr.Get(expr, name);
            } else {
                break;
            }
        }
        return expr;
    }

    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        if (!check(TokenType.RIGHT_PAREN)) {
            do {
                // having a max number of args will simplify our bytecode interpreter later, and we'll do it here too
                if (arguments.size() >= 255) {
                    error(peek(), "Can't have more than 255 arguments.");
                }
                arguments.add(expression());
            } while (match(TokenType.COMMA));
        }

        Token paren = consume(TokenType.RIGHT_PAREN, "Expect ')' after arguments");

        return new Expr.Call(callee, paren, arguments);
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
