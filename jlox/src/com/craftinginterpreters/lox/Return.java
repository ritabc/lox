package com.craftinginterpreters.lox;

/* An exception to help us jump all the way out of the current context (could be deeply nested stmts, to end the current function it's in.

 */
public class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        super(null, null, false, false);
        this.value = value;
    }
}
