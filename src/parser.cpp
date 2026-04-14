#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace csimple {

// AST helpers

std::string NumberExpr::toString() const { return value; }

std::string IdentExpr::toString() const { return name; }

BinaryExpr::BinaryExpr(char o, ExprPtr l, ExprPtr r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

std::string BinaryExpr::toString() const {
    std::ostringstream ss;
    ss << "(" << op << " " << left->toString() << " " << right->toString() << ")";
    return ss.str();
}

LetStmt::LetStmt(std::string n, ExprPtr e) : name(std::move(n)), expr(std::move(e)) {}

std::string LetStmt::toString() const {
    std::ostringstream ss;
    ss << "(let " << name << " " << expr->toString() << ")";
    return ss.str();
}

ExprStmt::ExprStmt(ExprPtr e) : expr(std::move(e)) {}

std::string ExprStmt::toString() const { return expr->toString(); }

// Parser

Parser::Parser(const std::string &source) : L(source) {
    advance();
}

void Parser::advance() {
    cur = L.next();
}

bool Parser::accept(TokenType t) {
    if (cur.type == t) {
        advance();
        return true;
    }
    return false;
}

bool Parser::acceptNewline() {
    if (cur.type == TokenType::TK_NEWLINE) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType t, const char *msg) {
    if (cur.type != t) {
        std::ostringstream ss;
        ss << msg << " (line=" << cur.line << ") got '" << cur.lexeme << "'";
        throw std::runtime_error(ss.str());
    }
    advance();
}

std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> out;
    while (cur.type != TokenType::TK_EOF) {
        if (cur.type == TokenType::TK_NEWLINE) { advance(); continue; }
        out.push_back(parseStatement());
        // optional newline after statement
        while (acceptNewline()) {}
    }
    return out;
}

StmtPtr Parser::parseStatement() {
    if (cur.type == TokenType::TK_LET) {
        advance();
        if (cur.type != TokenType::TK_IDENT) throw std::runtime_error("expected identifier after let");
        std::string name = cur.lexeme;
        advance();
        expect(TokenType::TK_EQ, "expected '=' after identifier");
        ExprPtr e = parseExpression();
        return std::make_unique<LetStmt>(name, std::move(e));
    }
    // otherwise expression statement
    ExprPtr e = parseExpression();
    return std::make_unique<ExprStmt>(std::move(e));
}

ExprPtr Parser::parseExpression() {
    ExprPtr left = parseTerm();
    while (cur.type == TokenType::TK_PLUS || cur.type == TokenType::TK_MINUS) {
        char op = (cur.type == TokenType::TK_PLUS) ? '+' : '-';
        advance();
        ExprPtr right = parseTerm();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseTerm() {
    ExprPtr left = parseFactor();
    while (cur.type == TokenType::TK_MUL || cur.type == TokenType::TK_DIV) {
        char op = (cur.type == TokenType::TK_MUL) ? '*' : '/';
        advance();
        ExprPtr right = parseFactor();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseFactor() {
    if (cur.type == TokenType::TK_NUMBER) {
        std::string v = cur.value;
        advance();
        return std::make_unique<NumberExpr>(v);
    }
    if (cur.type == TokenType::TK_IDENT) {
        std::string n = cur.lexeme;
        advance();
        return std::make_unique<IdentExpr>(n);
    }
    if (cur.type == TokenType::TK_LPAREN) {
        advance();
        ExprPtr e = parseExpression();
        expect(TokenType::TK_RPAREN, "expected ')' ");
        return e;
    }
    std::ostringstream ss;
    ss << "unexpected factor '" << cur.lexeme << "' (line=" << cur.line << ")";
    throw std::runtime_error(ss.str());
}

} // namespace csimple
