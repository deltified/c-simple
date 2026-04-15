#include "lexer.h"
#include <cctype>
#include <cstdlib>
#include <deque>
#include <stdexcept>

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

Token Lexer::lexString() {
    Token t;
    t.type = TokenType::TK_STRING;
    t.line = line;

    get(); // opening quote
    std::string decoded;
    while (true) {
        char c = get();
        if (c == '\0') {
            throw std::runtime_error("unterminated string literal");
        }
        if (c == '\n') {
            throw std::runtime_error("newline in string literal");
        }
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            char esc = get();
            if (esc == '\0') throw std::runtime_error("unterminated string escape");
            switch (esc) {
                case 'n': decoded.push_back('\n'); break;
                case 't': decoded.push_back('\t'); break;
                case 'r': decoded.push_back('\r'); break;
                case '"': decoded.push_back('"'); break;
                case '\\': decoded.push_back('\\'); break;
                default: decoded.push_back(esc); break;
            }
            continue;
        }
        decoded.push_back(c);
    }

    t.lexeme = std::string("\"") + decoded + "\"";
    t.value = decoded;
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
    else if (lex == "fn") t.type = TokenType::TK_FN;
    else if (lex == "return") t.type = TokenType::TK_RETURN;
    else if (lex == "if") t.type = TokenType::TK_IF;
    else if (lex == "elif") t.type = TokenType::TK_ELIF;
    else if (lex == "else") t.type = TokenType::TK_ELSE;
    else t.type = TokenType::TK_IDENT;
    return t;
}

Token Lexer::next() {
    if (!pending.empty()) {
        Token t = pending.front(); pending.pop_front();
        return t;
    }

    if (idx >= src.size()) {
        // at EOF, unwind any remaining indents
        while (indents.size() > 1) {
            indents.pop_back();
            pending.push_back(Token{TokenType::TK_DEDENT, "", "", line});
        }
        if (!pending.empty()) { Token t = pending.front(); pending.pop_front(); return t; }
        return Token{TokenType::TK_EOF, "", "", line};
    }

    char c = peek();

    // handle newline + emit INDENT/DEDENT as needed
    if (c == '\n') {
        get();
        ++line;
        // count indentation spaces/tabs
        int count = 0;
        while (true) {
            char nc = peek();
            if (nc == ' ') { ++count; get(); continue; }
            if (nc == '\t') { count += 4; get(); continue; }
            break;
        }
        // ignore empty lines
        char after = peek();
        if (after == '\n' || after == '\0') {
            // push a simple NEWLINE token and return it
            pending.push_back(Token{TokenType::TK_NEWLINE, "\n", "", line - 1});
            Token t = pending.front(); pending.pop_front();
            return t;
        }

        pending.push_back(Token{TokenType::TK_NEWLINE, "\n", "", line - 1});
        int curIndent = indents.back();
        if (count > curIndent) {
            indents.push_back(count);
            pending.push_back(Token{TokenType::TK_INDENT, "", "", line});
        } else if (count < curIndent) {
            while (indents.size() > 1 && count < indents.back()) {
                indents.pop_back();
                pending.push_back(Token{TokenType::TK_DEDENT, "", "", line});
            }
        }
        Token t = pending.front(); pending.pop_front();
        return t;
    }

    // skip whitespace (spaces/tabs) between tokens
    if (c == ' ' || c == '\t' || c == '\r') {
        skipWhitespace();
        return next();
    }

    if (std::isdigit((unsigned char)c)) return lexNumber();
    if (c == '"') return lexString();
    if (std::isalpha((unsigned char)c) || c == '_') return lexIdent();

    // multi-char and single-char operator tokens
    Token t;
    t.line = line;

    // Two-char operators: check before consuming
    if (c == '-' && idx + 1 < src.size() && src[idx + 1] == '>') {
        get();
        get();
        t.lexeme = "->";
        t.value = "->";
        t.type = TokenType::TK_ARROW;
        return t;
    }
    if ((c == '=' || c == '!' || c == '<' || c == '>') && idx + 1 < src.size() && src[idx + 1] == '=') {
        char first = get();
        get(); // consume '='
        if (first == '=') { t.lexeme = "=="; t.value = "=="; t.type = TokenType::TK_EQEQ; return t; }
        if (first == '!') { t.lexeme = "!="; t.value = "!="; t.type = TokenType::TK_NEQ;  return t; }
        if (first == '<') { t.lexeme = "<="; t.value = "<="; t.type = TokenType::TK_LTE;  return t; }
        if (first == '>') { t.lexeme = ">="; t.value = ">="; t.type = TokenType::TK_GTE;  return t; }
    }
    if (c == '&' && idx + 1 < src.size() && src[idx + 1] == '&') {
        get(); get();
        t.lexeme = "&&"; t.value = "&&"; t.type = TokenType::TK_AND; return t;
    }
    if (c == '|' && idx + 1 < src.size() && src[idx + 1] == '|') {
        get(); get();
        t.lexeme = "||"; t.value = "||"; t.type = TokenType::TK_OR; return t;
    }

    // single-char tokens
    get();
    t.lexeme = std::string(1, c);
    t.value = t.lexeme;
    switch (c) {
        case '+': t.type = TokenType::TK_PLUS;  return t;
        case '-': t.type = TokenType::TK_MINUS; return t;
        case '*': t.type = TokenType::TK_MUL;   return t;
        case '/': t.type = TokenType::TK_DIV;   return t;
        case '(': t.type = TokenType::TK_LPAREN; return t;
        case ')': t.type = TokenType::TK_RPAREN; return t;
        case '=': t.type = TokenType::TK_EQ;    return t;
        case ':': t.type = TokenType::TK_COLON; return t;
        case ',': t.type = TokenType::TK_COMMA; return t;
        case '<': t.type = TokenType::TK_LT;    return t;
        case '>': t.type = TokenType::TK_GT;    return t;
        case '!': t.type = TokenType::TK_NOT;   return t;
        default:  t.type = TokenType::TK_UNKNOWN; return t;
    }
    // unreachable
}

} 
