#include "lexer.h"
#include <cstring>
#include "exceptions.hpp"

Lexer::Lexer(const char* fn, std::string text) : mFileName(fn), mText(std::move(text)), mPos(-1, 0, -1) {
    advance();
}

void Lexer::process() {
    while (mCurrentChar) {
        if (mCurrentChar[0] == '\t' || mCurrentChar[0] == '\n' || std::isspace(mCurrentChar[0])) {
            advance();
        } else if (!std::strncmp("dotimes", mCurrentChar, 7)) {
            mTokens.emplace_back(TokenType::DOTIMES);
            advance(7);
        } else if (!std::strncmp("let", mCurrentChar, 3)) {
            mTokens.emplace_back(TokenType::LET);
            advance(3);
        } else if (!std::strncmp("setq", mCurrentChar, 4)) {
            mTokens.emplace_back(TokenType::SETQ);
            advance(4);
        } else if (!std::strncmp("if", mCurrentChar, 2)) {
            mTokens.emplace_back(TokenType::IF);
            advance(2);
        } else if (!std::strncmp("when", mCurrentChar, 4)) {
            mTokens.emplace_back(TokenType::WHEN);
            advance(4);
        } else if (!std::strncmp("cond", mCurrentChar, 4)) {
            mTokens.emplace_back(TokenType::COND);
            advance(4);
        } else if (!std::strncmp("defvar", mCurrentChar, 4)) {
            mTokens.emplace_back(TokenType::DEFVAR);
            advance(6);
        } else if (!std::strncmp("defconstant", mCurrentChar, 11)) {
            mTokens.emplace_back(TokenType::DEFCONST);
            advance(11);
        } else if (!std::strncmp("defun", mCurrentChar, 5)) {
            mTokens.emplace_back(TokenType::DEFUN);
            advance(5);
        } else if (!std::strncmp("nil", mCurrentChar, 3)) {
            mTokens.emplace_back(TokenType::NIL);
            advance(3);
        } else if (!std::strncmp("and", mCurrentChar, 3)) {
            mTokens.emplace_back(TokenType::AND);
            advance(3);
        } else if (!std::strncmp("or", mCurrentChar, 2)) {
            mTokens.emplace_back(TokenType::OR);
            advance(2);
        } else if (!std::strncmp("not", mCurrentChar, 3)) {
            mTokens.emplace_back(TokenType::NOT);
            advance(3);
        } else if (mCurrentChar[0] == 't' && mCurrentChar[1] == ' ') {
            mTokens.emplace_back(TokenType::T);
            advance();
        } else if (std::isalpha(mCurrentChar[0])) {
            std::string token;

            while (mCurrentChar && std::isalnum(mCurrentChar[0])) {
                token += mCurrentChar[0];
                advance();
            }

            mTokens.emplace_back(TokenType::VAR, token);
        } else if (std::isdigit(mCurrentChar[0])) {
            std::string token;
            bool isDouble;

            while (mCurrentChar && (std::isalnum(mCurrentChar[0]) || mCurrentChar[0] == '.')) {
                if (std::isalpha(mCurrentChar[0]))
                    throw IllegalCharError(mFileName, (token + mCurrentChar[0]).c_str(), mPos.lineNumber);

                if (!isDouble && mCurrentChar[0] == '.') isDouble = true;

                token += mCurrentChar[0];
                advance();
            }

            mTokens.emplace_back(isDouble ? TokenType::DOUBLE : TokenType::INT, token);
        } else if (mCurrentChar[0] == '+') {
            mTokens.emplace_back(TokenType::PLUS);
            advance();
        } else if (mCurrentChar[0] == '-') {
            mTokens.emplace_back(TokenType::MINUS);
            advance();
        } else if (mCurrentChar[0] == '*') {
            mTokens.emplace_back(TokenType::MUL);
            advance();
        } else if (mCurrentChar[0] == '/') {
            mTokens.emplace_back(TokenType::DIV);
            advance();
        } else if (mCurrentChar[0] == '=') {
            mTokens.emplace_back(TokenType::EQUAL);
            advance();
        } else if (!std::strncmp("/=", mCurrentChar, 2)) {
            mTokens.emplace_back(TokenType::NEQUAL);
            advance(2);
        } else if (mCurrentChar[0] == '>') {
            mTokens.emplace_back(TokenType::GREATER_THEN);
            advance();
        } else if (!std::strncmp(">=", mCurrentChar, 2)) {
            mTokens.emplace_back(TokenType::GREATER_THEN_EQ);
            advance(2);
        } else if (mCurrentChar[0] == '<') {
            mTokens.emplace_back(TokenType::LESS_THEN);
            advance();
        } else if (!std::strncmp("<=", mCurrentChar, 2)) {
            mTokens.emplace_back(TokenType::LESS_THEN_EQ);
            advance(2);
        } else if (mCurrentChar[0] == '(') {
            mTokens.emplace_back(TokenType::LPAREN);
            advance();
        } else if (mCurrentChar[0] == ')') {
            mTokens.emplace_back(TokenType::RPAREN);
            advance();
        } else {
            throw IllegalCharError(mFileName, std::string(1, mCurrentChar[0]).c_str(), mPos.lineNumber);
        }
    }

    mTokens.emplace_back(TokenType::EOF_);
}

void Lexer::advance() {
    mPos.advance(mCurrentChar);

    if (mPos.index < mText.size()) {
        mCurrentChar = &mText[mPos.index];
    } else {
        mCurrentChar = nullptr;
    }
}

void Lexer::advance(int step) {
    for (int i = 0; i < step; ++i) advance();
}
