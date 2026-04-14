#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"

using namespace csimple;

static std::string readFile(const std::string &path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string getTokenTypeStr(TokenType t) {
    switch (t) {
        case TokenType::TK_EOF: return "EOF";
        case TokenType::TK_NEWLINE: return "NEWLINE";
        case TokenType::TK_NUMBER: return "NUMBER";
        case TokenType::TK_IDENT: return "IDENT";
        case TokenType::TK_LET: return "LET";
        case TokenType::TK_EQ: return "EQ";
        case TokenType::TK_PLUS: return "PLUS";
        case TokenType::TK_MINUS: return "MINUS";
        case TokenType::TK_MUL: return "MUL";
        case TokenType::TK_DIV: return "DIV";
        case TokenType::TK_LPAREN: return "LPAREN";
        case TokenType::TK_RPAREN: return "RPAREN";
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
    while(true) {
        Token t = L.next();
        std::cout << getTokenTypeStr(t.type) << "\t" << t.lexeme;
            if (!t.value.empty()) std::cout << "\t" << t.value;
        std::cout << "\t(line=" << t.line << ")\n";
        if (t.type == TokenType::TK_EOF) break;
    }
    return 0;
}
