#include "lexer.h"
#include <cctype>
#include <cstdlib>

namespace csimple {

Lexer::Lexer(const std::string &source) : src(source), idx(0), line(1) {}

char Lexer::peek() const {
    if (idx >= src.size()) return '\0';
    return src[idx];
}

char Lexer::get() {
    if (idx >= src.size()) return '\0';
    return src[idx++];
}

void Lexer::skipWhitespace() {
    while(true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            get();
            continue;
        }
        if (c == '\n') return; // let caller handle newline as token
        break;
    }
}

Token Lexer::lexNumber() {
    size_t start = idx;
    bool seenDot = false;
    while (true) {
        char c = peek();
        if (c == '.') {
            if (seenDot) break;
            seenDot = true;
            get();
            continue;
        }
        if (!std::isdigit((unsigned char)c)) break;
        get();
    }
    std::string lex = src.substr(start, idx - start);
    Token t;
    t.type = TokenType::TK_NUMBER;
    t.lexeme = lex;
    t.value = lex;
    t.line = line;
    return t;
}

Token Lexer::lexIdent() {
    size_t start = idx;
    get();
    while (true) {
        char c = peek();
        if (std::isalnum((unsigned char)c) || c == '_') get();
        else break;
    }
    std::string lex = src.substr(start, idx - start);
    Token t;
    t.lexeme = lex;
    t.value = lex;
    t.line = line;
    if (lex == "let") t.type = TokenType::TK_LET;
    else t.type = TokenType::TK_IDENT;
    return t;
}

Token Lexer::next() {
    while(true) {
        if (idx >= src.size()) return Token{TokenType::TK_EOF, "", "", line};
        char c = peek();

        if (c == '\n') {
            get();
            ++line;
            return Token{TokenType::TK_NEWLINE, "\n", "", line - 1};
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            skipWhitespace();
            continue;
        }

        if (std::isdigit((unsigned char)c)) return lexNumber();

        if (std::isalpha((unsigned char)c) || c == '_') return lexIdent();

        // single-char tokens
        get();
        Token t;
        t.lexeme = std::string(1, c);
        t.value = t.lexeme;
        t.line = line;
        switch (c) {
            case '+': t.type = TokenType::TK_PLUS; return t;
            case '-': t.type = TokenType::TK_MINUS; return t;
            case '*': t.type = TokenType::TK_MUL; return t;
            case '/': t.type = TokenType::TK_DIV; return t;
            case '(' : t.type = TokenType::TK_LPAREN; return t;
            case ')' : t.type = TokenType::TK_RPAREN; return t;
            case '=' : t.type = TokenType::TK_EQ; return t;
            default: t.type = TokenType::TK_UNKNOWN; return t;
        }
    }
}

} 
