#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    // Type
    INT, DOUBLE, STRING, VAR, NIL, T,
    // Arithmetic Operators
    PLUS, MINUS, DIV, MUL,
    // Comparison Operators
    EQUAL, NEQUAL, GREATER_THEN, LESS_THEN, GREATER_THEN_EQ, LESS_THEN_EQ,
    // Logical Operators
    AND, OR, NOT,
    // Bitwise Operators
    LOGAND, LOGIOR, LOGXOR, LOGNOR,
    // Loop
    DOTIMES, LOOP,
    // Condition
    IF, WHEN, COND,
    // Assignment
    LET, SETQ, DEFVAR, DEFCONST,
    // Function
    DEFUN,
    // Special function
    RETURN,
    // Others
    LPAREN, RPAREN,
    EOF_
};

struct Token {
    TokenType type{};
    std::string lexeme;

    Token() = default;

    explicit Token(const TokenType type, std::string value = "") : type(type), lexeme(std::move(value)) {}
};

struct Position {
    int index;
    int lineNumber;
    int columnNumber;

    Position(const int idx, const int ln, const int coln) : index(idx), lineNumber(ln), columnNumber(coln) {}

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

    [[nodiscard]] size_t getTokenSize() const { return tokens.size(); }

    Token getToken(const int index) { return tokens[index]; }

private:
    void advance();

    void advance(int step);

    std::string text;
    Position pos;
    std::vector<Token> tokens;
    char* currentChar{};
    const char* fileName;
};

#endif
