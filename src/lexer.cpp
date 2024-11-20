#include "lexer.h"
#include "exceptions.hpp"

Lexer::Lexer(const char* fn, std::string text) : mFileName(fn), mText(std::move(text)), mPos(-1, 0, -1) {
    advance();
}

std::vector<Token> Lexer::makeTokens() {
    std::vector<Token> tokens;

    while (mCurrentChar) {
        if (mCurrentChar[0] == '\t' || mCurrentChar[0] == '\n' || std::isspace(mCurrentChar[0])) {
            advance();
        } else if (isdigit(mCurrentChar[0])) {
            std::string digit;

            while (mCurrentChar && isdigit(mCurrentChar[0])) {
                digit += mCurrentChar;
                advance();
            }
            tokens.emplace_back(TokenType::INT, digit);
        } else if (!strncmp("print", mCurrentChar, 5)) {
            tokens.emplace_back(TokenType::PRINT);

            for (int i = 0; i < 5; ++i) {
                advance();
            }
        } else if (!strncmp("dotimes", mCurrentChar, 7)) {
            tokens.emplace_back(TokenType::DOTIMES);

            for (int i = 0; i < 7; ++i) {
                advance();
            }
        } else if (!strncmp("let", mCurrentChar, 3)) {
            tokens.emplace_back(TokenType::LET);

            for (int i = 0; i < 3; ++i) {
                advance();
            }
        } else if (std::isalpha(mCurrentChar[0])) {
            tokens.emplace_back(TokenType::VAR, std::string(1, mCurrentChar[0]));
            advance();
        } else if (std::isdigit(mCurrentChar[0])) {
            std::string digit;

            while (mCurrentChar && std::isdigit(mCurrentChar[0])) {
                digit += mCurrentChar;
                advance();
            }

            tokens.emplace_back(TokenType::INT, digit);
        } else if (mCurrentChar[0] == '+') {
            tokens.emplace_back(TokenType::PLUS);
            advance();
        } else if (mCurrentChar[0] == '-') {
            tokens.emplace_back(TokenType::MINUS);
            advance();
        } else if (mCurrentChar[0] == '*') {
            tokens.emplace_back(TokenType::MUL);
            advance();
        } else if (mCurrentChar[0] == '/') {
            tokens.emplace_back(TokenType::DIV);
            advance();
        } else if (mCurrentChar[0] == '(') {
            tokens.emplace_back(TokenType::LPAREN);
            advance();
        } else if (mCurrentChar[0] == ')') {
            tokens.emplace_back(TokenType::RPAREN);
            advance();
        } else {
            throw IllegalCharError(mFileName, std::string(1, mCurrentChar[0]).c_str(), mPos.lineNumber);
        }
    }

    tokens.emplace_back(TokenType::_EOF, "EOF");

    return tokens;
}

void Lexer::advance() {
    mPos.advance(mCurrentChar);

    if (mPos.index < mText.size()) {
        mCurrentChar = &mText[mPos.index];
    } else {
        mCurrentChar = 0;
    }
}
