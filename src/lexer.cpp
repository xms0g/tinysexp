#include "lexer.h"
#include <cctype>
#include "exceptions.hpp"

Lexer::Lexer(const char* fn, std::string text) : mFileName(fn), mText(std::move(text)), mPos(-1, 0, -1) {
    advance();
}

void Lexer::advance() {
    mPos.advance(mCurrentChar);

    if (mPos.index < mText.size()) {
        mCurrentChar = mText[mPos.index];
    } else {
        mCurrentChar = 0;
    }
}

std::vector<Token> Lexer::makeTokens() {
    std::vector<Token> tokens;

    while (mCurrentChar) {
        if (mCurrentChar == '\t' || mCurrentChar == '\n' || mCurrentChar == ' ')
            advance();
        else if (isdigit(mCurrentChar)) {
            tokens.emplace_back(TokenType::INT, std::to_string(mCurrentChar));
            advance();
        } else if (mCurrentChar == '+') {
            tokens.emplace_back(TokenType::PLUS);
            advance();
        } else if (mCurrentChar == '-') {
            tokens.emplace_back(TokenType::MINUS);
            advance();
        } else if (mCurrentChar == '*') {
            tokens.emplace_back(TokenType::MUL);
            advance();
        } else if (mCurrentChar == '/') {
            tokens.emplace_back(TokenType::DIV);
            advance();
        } else if (mCurrentChar == '(') {
            tokens.emplace_back(TokenType::LPAREN);
            advance();
        } else if (mCurrentChar == ')') {
            tokens.emplace_back(TokenType::RPAREN);
            advance();
        } else {
            throw IllegalCharError(mFileName, mCurrentChar, mPos.lineNumber);
        }
    }

    return tokens;
}
