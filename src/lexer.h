#pragma once

#include <string>
#include <string_view>
#include <format>

enum class TokenType {
    INT, PLUS, MINUS,
    DIV, MUL, LPAREN,
    RPAREN, PRINT, _EOF
};

struct Token {
    TokenType type{};
    std::string value;

    Token() = default;
    explicit Token(TokenType type, std::string value = ""): type(type), value(std::move(value)) {}

    friend std::ostream& operator<<(std::ostream& os, const Token& t) {
        os << std::format("{}: {}", std::to_string(static_cast<int>(t.type)), t.value);
        return os;
    }
};

struct Position {
    int index;
    int lineNumber;
    int columnNumber;

    Position(int idx, int ln, int coln): index(idx), lineNumber(ln), columnNumber(coln) {}

    void advance(const char* token) {
        ++index;
        ++columnNumber;

        if (token && std::string_view(token).starts_with('\n')) {
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
