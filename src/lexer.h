#pragma once

#include <string>
#include <vector>

enum class TokenType {
    INT, PLUS, MINUS,
    DIV, MUL, LPAREN,
    RPAREN, PRINT, DOTIMES,
    VAR, LET, _EOF
};

struct Token {
    TokenType type{};
    std::string value;

    Token() = default;

    explicit Token(TokenType type, std::string value = "") : type(type), value(std::move(value)) {}
};

struct Position {
    int index;
    int lineNumber;
    int columnNumber;

    Position(int idx, int ln, int coln) : index(idx), lineNumber(ln), columnNumber(coln) {}

    void advance(const char* token) {
        ++index;
        ++columnNumber;

        if (token && token[0] == '\n') {
            ++lineNumber;
            columnNumber = 0;
        }
    }
};

class Lexer {
public:
    Lexer(const char* fn, std::string text);

    std::vector<Token> makeTokens();

private:
    void advance();

    std::string mText;
    Position mPos;
    char* mCurrentChar{};
    const char* mFileName;
};
