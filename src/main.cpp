#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

using namespace csimple;

static std::string readFile(const std::string &path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string getTokenTypeStr(TokenType t) {
    switch (t) {
        case TokenType::TK_EOF:     return "EOF";
        case TokenType::TK_NEWLINE: return "NEWLINE";
        case TokenType::TK_NUMBER:  return "NUMBER";
        case TokenType::TK_IDENT:   return "IDENT";
        case TokenType::TK_LET:     return "LET";
        case TokenType::TK_FN:      return "FN";
        case TokenType::TK_RETURN:  return "RETURN";
        case TokenType::TK_IF:      return "IF";
        case TokenType::TK_ELIF:    return "ELIF";
        case TokenType::TK_ELSE:    return "ELSE";
        case TokenType::TK_COLON:   return "COLON";
        case TokenType::TK_COMMA:   return "COMMA";
        case TokenType::TK_INDENT:  return "INDENT";
        case TokenType::TK_DEDENT:  return "DEDENT";
        case TokenType::TK_EQ:      return "EQ";
        case TokenType::TK_EQEQ:   return "EQEQ";
        case TokenType::TK_NEQ:     return "NEQ";
        case TokenType::TK_LT:      return "LT";
        case TokenType::TK_GT:      return "GT";
        case TokenType::TK_LTE:     return "LTE";
        case TokenType::TK_GTE:     return "GTE";
        case TokenType::TK_NOT:     return "NOT";
        case TokenType::TK_AND:     return "AND";
        case TokenType::TK_OR:      return "OR";
        case TokenType::TK_PLUS:    return "PLUS";
        case TokenType::TK_MINUS:   return "MINUS";
        case TokenType::TK_MUL:     return "MUL";
        case TokenType::TK_DIV:     return "DIV";
        case TokenType::TK_LPAREN:  return "LPAREN";
        case TokenType::TK_RPAREN:  return "RPAREN";
        case TokenType::TK_UNKNOWN: return "UNKNOWN";
    }
    return "?";
}

int main(int argc, char **argv) {
    std::string input;
    if (argc > 1) input = readFile(argv[1]);
    else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    Lexer L(input);

    // Always print tokens
    while(true) {
        Token t = L.next();
        std::cout << getTokenTypeStr(t.type) << "\t" << t.lexeme;
        if (!t.value.empty()) std::cout << "\t" << t.value;
        std::cout << "\t(line=" << t.line << ")\n";
        if (t.type == TokenType::TK_EOF) break;
    }

    // Also run parser and print AST
    Parser P(input);
    try {
        auto stmts = P.parseProgram();
        std::cout << "--- AST ---\n";
        for (auto &s : stmts) std::cout << s->toString() << "\n";
        // generate C code
        Codegen cg;
        std::string csrc = cg.generate(stmts);
        std::cout << "--- C ---\n" << csrc;
    } catch (const std::exception &ex) {
        std::cerr << "Parse error: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
