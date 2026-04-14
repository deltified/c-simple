#pragma once

#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

namespace csimple {

struct Expr {
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct NumberExpr : Expr {
    std::string value;
    explicit NumberExpr(std::string v) : value(std::move(v)) {}
    std::string toString() const override;
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n) : name(std::move(n)) {}
    std::string toString() const override;
};

struct BinaryExpr : Expr {
    char op;
    ExprPtr left;
    ExprPtr right;
    BinaryExpr(char o, ExprPtr l, ExprPtr r);
    std::string toString() const override;
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string toString() const = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct LetStmt : Stmt {
    std::string name;
    ExprPtr expr;
    LetStmt(std::string n, ExprPtr e);
    std::string toString() const override;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e);
    std::string toString() const override;
};

struct ReturnStmt : Stmt {
    ExprPtr expr;
    explicit ReturnStmt(ExprPtr e);
    std::string toString() const override;
};

struct FunctionStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    FunctionStmt(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b);
    std::string toString() const override;
};

class Parser {
public:
    explicit Parser(const std::string &source);
    std::vector<StmtPtr> parseProgram();

private:
    Lexer L;
    Token cur;

    void advance();
    bool accept(TokenType t);
    bool acceptNewline();
    void expect(TokenType t, const char *msg="unexpected token");

    StmtPtr parseStatement();
    std::vector<StmtPtr> parseBlock();
    ExprPtr parseExpression();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
};

} // namespace csimple
