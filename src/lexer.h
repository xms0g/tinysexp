#pragma once

#include <string>
#include <vector>

enum class TokenType {
    // Type
    INT, DOUBLE, VAR,
    // Operators
    PLUS, MINUS, DIV, MUL,
    // IO
    PRINT, READ,
    // Loop
    DOTIMES,
    // Condition
    IF,
    // Assignment
    LET, SETQ, DEFVAR,
    // Others
    LPAREN, RPAREN,
    EOF_
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

    void process();

    [[nodiscard]] size_t getTokenSize() const { return mTokens.size(); }

    Token getToken(int index) { return mTokens[index]; }

private:
    void advance();

    void advance(int step);

    std::string mText;
    Position mPos;
    std::vector<Token> mTokens;
    char* mCurrentChar{};
    const char* mFileName;
};
