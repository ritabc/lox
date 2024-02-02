//
// Created by Rita Bennett-Chew on 2/2/24.
//

#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

/* Token Types are prefixed with TOKEN_ b/c C puts enum names in the top-level namespace.
 * The same as jlox token types, except for the addition of TOKEN_ERROR. This is for errors detected during scanning (which consist of unterminated strings and unrecognized characters).
 * In jlox, the scanner reported those itself.
 * But in clox, the scanner will produce a synthetic 'error' token for the error and pass it to the compiler.
 * So the compiler knows an error occurred and can start error recovery before reporting it.
 */
typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

/*
 * Token is comprised of:
 * - type enum
 * - where the token starts and the length of the token (a replacement for jlox's lexeme)
 * - and the line number
 * jlox Tokens had a lexeme (a Java string)
 * In clox, we pass tokens by value, and since multiple tokens could point to the same lexeme string, ownership would get weird.
 * Instead, we'll use the original source string instead of the lexeme.
 * This does mean the main source needs to outline all the tokens
 */
typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initScanner(const char* source);
Token scanToken();

#endif //CLOX_SCANNER_H
