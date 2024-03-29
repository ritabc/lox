package com.craftinginterpreters.tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

// A script class for generating expr, statement classes
public class GenerateAst
{
    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("Usage: generate_ast <output directory>");
            System.exit(64); // EX_USAGE (Command used incorrectly)
        }

        String outputDir = args[0];

        defineAst(outputDir, "Expr", Arrays.asList(
                "Assign    : Token name, Expr value",
                "Binary    : Expr left, Token operator, Expr right",
                "Call      : Expr callee, Token paren, List<Expr> arguments",
                "Get       : Expr object, Token name",
                "Grouping  : Expr expression",
                "Literal   : Object value",
                "Set       : Expr object, Token name, Expr value",
                "Super     : Token keyword, Token method",
                "This      : Token keyword",
                "Logical   : Expr left, Token operator, Expr right",
                "Unary     : Token operator, Expr right",
                "Ternary   : Expr check, Expr ifExpr, Expr elseExpr",
                "Variable  : Token name"
        ));

        defineAst(outputDir, "Stmt", Arrays.asList(
                "Block      : List<Stmt> statements",
                "Class      : Token name, Expr.Variable superclass, List<Stmt.Function> methods",
                "Expression : Expr expression",
                "Function   : Token name, List<Token> params, List<Stmt> body",
                "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
                "Print      : Expr expression",
                "Return     : Token keyword, Expr value",
                "Var        : Token name, Expr initializer",
                "While      : Expr condition, Stmt body"
        ));
    }

    private static void defineAst(String outputDir, String baseName, List<String> types) throws  IOException {
        String path = outputDir + "/" + baseName + ".java";
        PrintWriter writer = new PrintWriter(path, StandardCharsets.UTF_8);

        writer.println("// An auto generated file. See GenerateAst.java");
        writer.println();

        writer.println("package com.craftinginterpreters.lox;");
        writer.println();
        writer.println("import java.util.List;");
        writer.println();

        writer.println("abstract class " + baseName + " {");
        writer.println();

        defineVisitor(writer, baseName, types);
        writer.println();

        // The base accept() method.
        writer.println("  abstract <R> R accept(Visitor<R> visitor);");

        // The AST classes
        for (String type : types) {
            writer.println();
            String className = type.split(":")[0].trim();
            String fields = type.split(":")[1].trim();
            defineType(writer, baseName, className, fields);
        }

        writer.println();
        writer.println("}");
        writer.close();
    }

    private static void defineVisitor(PrintWriter writer, String baseName, List<String> types) {
        writer.println("  interface Visitor<R> {");
        for (String type : types) {
            String typeName = type.split(":")[0].trim();
            writer.println("    R visit" + typeName + baseName + "(" + typeName + " " + baseName.toLowerCase() + ");");
        }
        writer.println("  }");
    }

    // Writes a nested class for className, which extends baseName
    private static void defineType(PrintWriter writer, String baseName, String className, String fieldList) {
        writer.println("  static class " + className + " extends " + baseName + " {");

        // Constructor
        writer.println("    " + className + "(" + fieldList + ")" + "{");

        // Store params in fields
        String[] fields = fieldList.split(", ");
        for (String field : fields) {
            String name = field.split(" ")[1];
            writer.println("     this." + name + " = " + name + ";");
        }

        writer.println("    }");

        // Visitor pattern
        writer.println();
        writer.println("    @Override");
        writer.println("    <R> R accept(Visitor<R> visitor) {");
        writer.println("      return visitor.visit" + className + baseName + "(this);");
        writer.println("    }");

        // Fields
        writer.println();
        for (String field : fields) {
            writer.println("    final " + field + ";");
        }

        writer.println("  }");
    }
}

/*
abstract class Expr {

  interface Visitor<R> {
    R visitBinaryExpr(Binary expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitUnaryExpr(Unary expr);
  }
  abstract <R> R accept(Visitor<R> visitor);

  static class Binary extends Expr {
    Binary(Expr left, Token operator, Expr right){
     this.left = left;
     this.operator = operator;
     this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }

    final Expr left;
    final Token operator;
    final Expr right;
  }

  static class Grouping extends Expr {
    Grouping(Expr expression){
     this.expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }

    final Expr expression;
  }

  static class Literal extends Expr {
    Literal(Object value){
     this.value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }

    final Object value;
  }

  static class Unary extends Expr {
    Unary(Token operator, Expr right){
     this.operator = operator;
     this.right = right;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }

    final Token operator;
    final Expr right;
  }
* */