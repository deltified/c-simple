#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace csimple {

// AST helpers

std::string NumberExpr::toString() const { return value; }

std::string StringExpr::toString() const { return std::string("\"") + value + "\""; }

std::string IdentExpr::toString() const { return name; }

CallExpr::CallExpr(std::string c, std::vector<ExprPtr> a)
    : callee(std::move(c)), args(std::move(a)) {}

std::string CallExpr::toString() const {
    std::ostringstream ss;
    ss << "(" << callee;
    for (size_t i = 0; i < args.size(); ++i) {
        ss << " " << args[i]->toString();
    }
    ss << ")";
    return ss.str();
}

BinaryExpr::BinaryExpr(std::string o, ExprPtr l, ExprPtr r)
    : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}

std::string BinaryExpr::toString() const {
    std::ostringstream ss;
    ss << "(" << op << " " << left->toString() << " " << right->toString() << ")";
    return ss.str();
}

UnaryExpr::UnaryExpr(std::string o, ExprPtr x)
    : op(std::move(o)), operand(std::move(x)) {}

std::string UnaryExpr::toString() const {
    return std::string("(") + op + " " + operand->toString() + ")";
}

LetStmt::LetStmt(std::string n, ExprPtr e) : name(std::move(n)), expr(std::move(e)) {}

std::string LetStmt::toString() const {
    std::ostringstream ss;
    ss << "(let " << name << " " << expr->toString() << ")";
    return ss.str();
}

AssignStmt::AssignStmt(std::string n, ExprPtr e) : name(std::move(n)), expr(std::move(e)) {}

std::string AssignStmt::toString() const {
    std::ostringstream ss;
    ss << "(assign " << name << " " << expr->toString() << ")";
    return ss.str();
}

ExprStmt::ExprStmt(ExprPtr e) : expr(std::move(e)) {}

std::string ExprStmt::toString() const { return expr->toString(); }

ReturnStmt::ReturnStmt(ExprPtr e) : expr(std::move(e)) {}

std::string ReturnStmt::toString() const { return std::string("(return ") + expr->toString() + ")"; }

FunctionStmt::FunctionStmt(std::string n, ValueType r, std::vector<ParamDecl> p, std::vector<StmtPtr> b)
    : name(std::move(n)), returnType(r), params(std::move(p)), body(std::move(b)) {}

std::string FunctionStmt::toString() const {
    std::ostringstream ss;
    auto typeToString = [](ValueType t) {
        return t == ValueType::VT_STRING ? std::string("str") : std::string("int");
    };

    ss << "(fn " << name << " (";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i) ss << " ";
        ss << typeToString(params[i].type) << " " << params[i].name;
    }
    ss << ") -> " << typeToString(returnType) << " ";
    for (auto &s : body) ss << s->toString() << " ";
    ss << ")";
    return ss.str();
}

IfBranch::IfBranch(ExprPtr c, std::vector<StmtPtr> b)
    : cond(std::move(c)), body(std::move(b)) {}

IfStmt::IfStmt(std::vector<IfBranch> b, std::vector<StmtPtr> e)
    : branches(std::move(b)), elseBody(std::move(e)) {}

std::string IfStmt::toString() const {
    std::ostringstream ss;
    ss << "(if";
    for (const auto &b : branches) {
        ss << " (" << b.cond->toString() << " ";
        for (const auto &s : b.body) ss << s->toString() << " ";
        ss << ")";
    }
    if (!elseBody.empty()) {
        ss << " (else ";
        for (const auto &s : elseBody) ss << s->toString() << " ";
        ss << ")";
    }
    ss << ")";
    return ss.str();
}

// Parser

Parser::Parser(const std::string &source) : L(source) {
    cur = L.next();
    nxt = L.next();
}

void Parser::advance() {
    cur = nxt;
    nxt = L.next();
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

ValueType Parser::parseTypeName() {
    if (cur.type != TokenType::TK_IDENT) {
        throw std::runtime_error("expected type name");
    }
    if (cur.lexeme == "int") {
        advance();
        return ValueType::VT_INT;
    }
    if (cur.lexeme == "str") {
        advance();
        return ValueType::VT_STRING;
    }
    throw std::runtime_error("unknown type name '" + cur.lexeme + "'");
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
        std::vector<ParamDecl> params;
        if (cur.type != TokenType::TK_RPAREN) {
            while (true) {
                ValueType paramType = parseTypeName();
                if (cur.type != TokenType::TK_IDENT) throw std::runtime_error("expected parameter name after type annotation");
                params.push_back(ParamDecl{paramType, cur.lexeme});
                advance();
                if (cur.type == TokenType::TK_COMMA) { advance(); continue; }
                break;
            }
        }
        expect(TokenType::TK_RPAREN, "expected ')'");
        expect(TokenType::TK_ARROW, "expected '->' after function parameters");
        ValueType returnType = parseTypeName();
        expect(TokenType::TK_COLON, "expected ':' after function signature");
        // require newline then indented block
        expect(TokenType::TK_NEWLINE, "expected newline after ':'");
        std::vector<StmtPtr> body = parseBlock();
        return std::make_unique<FunctionStmt>(name, returnType, std::move(params), std::move(body));
    }
    if (cur.type == TokenType::TK_RETURN) {
        advance();
        ExprPtr e = parseExpression();
        return std::make_unique<ReturnStmt>(std::move(e));
    }
    if (cur.type == TokenType::TK_IF) {
        return parseIfStatement();
    }
    if (cur.type == TokenType::TK_IDENT && nxt.type == TokenType::TK_EQ) {
        std::string name = cur.lexeme;
        advance();
        expect(TokenType::TK_EQ, "expected '=' in assignment");
        ExprPtr e = parseExpression();
        return std::make_unique<AssignStmt>(name, std::move(e));
    }
    // otherwise expression statement
    ExprPtr e = parseExpression();
    return std::make_unique<ExprStmt>(std::move(e));
}

StmtPtr Parser::parseIfStatement() {
    expect(TokenType::TK_IF, "expected 'if'");

    ExprPtr cond = parseExpression();
    expect(TokenType::TK_COLON, "expected ':' after if condition");
    expect(TokenType::TK_NEWLINE, "expected newline after ':'");

    std::vector<IfBranch> branches;
    branches.emplace_back(std::move(cond), parseBlock());

    while (cur.type == TokenType::TK_ELIF) {
        advance();
        ExprPtr elifCond = parseExpression();
        expect(TokenType::TK_COLON, "expected ':' after elif condition");
        expect(TokenType::TK_NEWLINE, "expected newline after ':'");
        branches.emplace_back(std::move(elifCond), parseBlock());
    }

    std::vector<StmtPtr> elseBody;
    if (cur.type == TokenType::TK_ELSE) {
        advance();
        expect(TokenType::TK_COLON, "expected ':' after else");
        expect(TokenType::TK_NEWLINE, "expected newline after ':'");
        elseBody = parseBlock();
    }

    return std::make_unique<IfStmt>(std::move(branches), std::move(elseBody));
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
    return parseLogicalOr();
}

ExprPtr Parser::parseLogicalOr() {
    ExprPtr left = parseLogicalAnd();
    while (cur.type == TokenType::TK_OR) {
        advance();
        ExprPtr right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>("||" , std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseLogicalAnd() {
    ExprPtr left = parseComparison();
    while (cur.type == TokenType::TK_AND) {
        advance();
        ExprPtr right = parseComparison();
        left = std::make_unique<BinaryExpr>("&&", std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddSub();
    while (cur.type == TokenType::TK_LT  || cur.type == TokenType::TK_GT  ||
           cur.type == TokenType::TK_LTE || cur.type == TokenType::TK_GTE ||
           cur.type == TokenType::TK_EQEQ || cur.type == TokenType::TK_NEQ) {
        std::string op;
        switch (cur.type) {
            case TokenType::TK_LT:   op = "<";  break;
            case TokenType::TK_GT:   op = ">";  break;
            case TokenType::TK_LTE:  op = "<="; break;
            case TokenType::TK_GTE:  op = ">="; break;
            case TokenType::TK_EQEQ: op = "=="; break;
            case TokenType::TK_NEQ:  op = "!="; break;
            default: break;
        }
        advance();
        ExprPtr right = parseAddSub();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseAddSub() {
    ExprPtr left = parseTerm();
    while (cur.type == TokenType::TK_PLUS || cur.type == TokenType::TK_MINUS) {
        std::string op = (cur.type == TokenType::TK_PLUS) ? "+" : "-";
        advance();
        ExprPtr right = parseTerm();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseTerm() {
    ExprPtr left = parseUnary();
    while (cur.type == TokenType::TK_MUL || cur.type == TokenType::TK_DIV) {
        std::string op = (cur.type == TokenType::TK_MUL) ? "*" : "/";
        advance();
        ExprPtr right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (cur.type == TokenType::TK_NOT) {
        advance();
        ExprPtr operand = parseUnary();
        return std::make_unique<UnaryExpr>("!", std::move(operand));
    }
    return parseFactor();
}

ExprPtr Parser::parseFactor() {
    if (cur.type == TokenType::TK_NUMBER) {
        std::string v = cur.value;
        advance();
        return std::make_unique<NumberExpr>(v);
    }
    if (cur.type == TokenType::TK_STRING) {
        std::string v = cur.value;
        advance();
        return std::make_unique<StringExpr>(v);
    }
    if (cur.type == TokenType::TK_IDENT) {
        std::string n = cur.lexeme;
        advance();
        // function call: IDENT '(' args ')'
        if (cur.type == TokenType::TK_LPAREN) {
            advance();
            std::vector<ExprPtr> args;
            if (cur.type != TokenType::TK_RPAREN) {
                while (true) {
                    args.push_back(parseExpression());
                    if (cur.type == TokenType::TK_COMMA) { advance(); continue; }
                    break;
                }
            }
            expect(TokenType::TK_RPAREN, "expected ')'");
            return std::make_unique<CallExpr>(n, std::move(args));
        }
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
