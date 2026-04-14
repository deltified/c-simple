#pragma once

#include <string>
#include <cstdint>

namespace csimple {

enum class TokenType {
    TK_EOF,
    TK_NEWLINE,
    TK_NUMBER,
    TK_IDENT,
    TK_LET,
    TK_EQ,
    TK_PLUS,
    TK_MINUS,
    TK_MUL,
    TK_DIV,
    TK_LPAREN,
    TK_RPAREN,
    TK_UNKNOWN,
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::string value; // string value (numbers and other token values)
    int line{};
};

class Lexer {
public:
    explicit Lexer(const std::string &source);
    Token next();

private:
    const std::string src;
    size_t idx{};
    int line{1};

    char peek() const;
    char get();
    void skipWhitespace();
    Token lexNumber();
    Token lexIdent();
};

} 