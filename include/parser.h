#pragma once

#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

namespace csimple {

enum class ValueType {
    VT_INT,
    VT_STRING,
};

struct ParamDecl {
    ValueType type;
    std::string name;
};

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

struct StringExpr : Expr {
    std::string value;
    explicit StringExpr(std::string v) : value(std::move(v)) {}
    std::string toString() const override;
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n) : name(std::move(n)) {}
    std::string toString() const override;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    CallExpr(std::string c, std::vector<ExprPtr> a);
    std::string toString() const override;
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left;
    ExprPtr right;
    BinaryExpr(std::string o, ExprPtr l, ExprPtr r);
    std::string toString() const override;
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;
    UnaryExpr(std::string o, ExprPtr x);
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

struct AssignStmt : Stmt {
    std::string name;
    ExprPtr expr;
    AssignStmt(std::string n, ExprPtr e);
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
    ValueType returnType;
    std::vector<ParamDecl> params;
    std::vector<StmtPtr> body;
    FunctionStmt(std::string n, ValueType r, std::vector<ParamDecl> p, std::vector<StmtPtr> b);
    std::string toString() const override;
};

struct IfBranch {
    ExprPtr cond;
    std::vector<StmtPtr> body;
    IfBranch(ExprPtr c, std::vector<StmtPtr> b);
};

struct IfStmt : Stmt {
    std::vector<IfBranch> branches;
    std::vector<StmtPtr> elseBody;
    IfStmt(std::vector<IfBranch> b, std::vector<StmtPtr> e);
    std::string toString() const override;
};

class Parser {
public:
    explicit Parser(const std::string &source);
    std::vector<StmtPtr> parseProgram();

private:
    Lexer L;
    Token cur;
    Token nxt;

    void advance();
    bool accept(TokenType t);
    bool acceptNewline();
    void expect(TokenType t, const char *msg="unexpected token");

    StmtPtr parseStatement();
    StmtPtr parseIfStatement();
    ValueType parseTypeName();
    std::vector<StmtPtr> parseBlock();
    ExprPtr parseExpression();
    ExprPtr parseLogicalOr();
    ExprPtr parseLogicalAnd();
    ExprPtr parseComparison();
    ExprPtr parseAddSub();
    ExprPtr parseTerm();
    ExprPtr parseUnary();
    ExprPtr parseFactor();
};

} // namespace csimple
