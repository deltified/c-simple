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

ReturnStmt::ReturnStmt(ExprPtr e) : expr(std::move(e)) {}

std::string ReturnStmt::toString() const { return std::string("(return ") + expr->toString() + ")"; }

FunctionStmt::FunctionStmt(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b)
    : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}

std::string FunctionStmt::toString() const {
    std::ostringstream ss;
    ss << "(fn " << name << " (";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i) ss << " ";
        ss << params[i];
    }
    ss << ") ";
    for (auto &s : body) ss << s->toString() << " ";
    ss << ")";
    return ss.str();
}

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
    if (cur.type == TokenType::TK_FN) {
        advance();
        if (cur.type != TokenType::TK_IDENT) throw std::runtime_error("expected function name after fn");
        std::string name = cur.lexeme;
        advance();
        expect(TokenType::TK_LPAREN, "expected '('");
        std::vector<std::string> params;
        if (cur.type != TokenType::TK_RPAREN) {
            while (true) {
                if (cur.type != TokenType::TK_IDENT) throw std::runtime_error("expected parameter name");
                params.push_back(cur.lexeme);
                advance();
                if (cur.type == TokenType::TK_COMMA) { advance(); continue; }
                break;
            }
        }
        expect(TokenType::TK_RPAREN, "expected ')'");
        expect(TokenType::TK_COLON, "expected ':' after function signature");
        // require newline then indented block
        expect(TokenType::TK_NEWLINE, "expected newline after ':'");
        std::vector<StmtPtr> body = parseBlock();
        return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
    }
    if (cur.type == TokenType::TK_RETURN) {
        advance();
        ExprPtr e = parseExpression();
        return std::make_unique<ReturnStmt>(std::move(e));
    }
    // otherwise expression statement
    ExprPtr e = parseExpression();
    return std::make_unique<ExprStmt>(std::move(e));
}

std::vector<StmtPtr> Parser::parseBlock() {
    std::vector<StmtPtr> out;
    expect(TokenType::TK_INDENT, "expected indent");
    while (cur.type != TokenType::TK_DEDENT && cur.type != TokenType::TK_EOF) {
        if (cur.type == TokenType::TK_NEWLINE) { advance(); continue; }
        out.push_back(parseStatement());
        while (acceptNewline()) {}
    }
    expect(TokenType::TK_DEDENT, "expected dedent");
    return out;
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
