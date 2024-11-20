#include "lexer.h"
#include "exceptions.hpp"

Lexer::Lexer(const char* fn, std::string text) : mFileName(fn), mText(std::move(text)), mPos(-1, 0, -1) {
    advance();
}

std::vector<Token> Lexer::makeTokens() {
    std::vector<Token> tokens;

    while (mCurrentChar) {
        auto sv = std::string_view(mCurrentChar);

        if (sv.starts_with('\t') || sv.starts_with('\n') || sv.starts_with(' '))
            advance();
        else if (isdigit(sv[0])) {
            std::string digit;

            while (mCurrentChar && isdigit(mCurrentChar[0])) {
                digit += mCurrentChar[0];
                advance();
            }
            tokens.emplace_back(TokenType::INT, digit);
        } else if (sv.starts_with("print")) {
            tokens.emplace_back(TokenType::PRINT);

            for (int i = 0; i < 5; ++i) {
                advance();
            }
        } else if (sv.starts_with("dotimes")) {
            tokens.emplace_back(TokenType::DOTIMES);

            for (int i = 0; i < 7; ++i) {
                advance();
            }
        } else if (std::isalpha(sv[0])) {
            tokens.emplace_back(TokenType::VAR, std::string(1, sv[0]));
            advance();
        } else if (std::isdigit(sv[0])) {
            std::string digit;

            while (mCurrentChar && std::isdigit(mCurrentChar[0])) {
                digit += mCurrentChar[0];
                advance();
            }

            tokens.emplace_back(TokenType::INT, digit);
        } else if (sv.starts_with('+')) {
            tokens.emplace_back(TokenType::PLUS);
            advance();
        } else if (sv.starts_with('-')) {
            tokens.emplace_back(TokenType::MINUS);
            advance();
        } else if (sv.starts_with('*')) {
            tokens.emplace_back(TokenType::MUL);
            advance();
        } else if (sv.starts_with('/')) {
            tokens.emplace_back(TokenType::DIV);
            advance();
        } else if (sv.starts_with('(')) {
            tokens.emplace_back(TokenType::LPAREN);
            advance();
        } else if (sv.starts_with(')')) {
            tokens.emplace_back(TokenType::RPAREN);
            advance();
        } else {
            throw IllegalCharError(mFileName, std::string(1, sv[0]).c_str(), mPos.lineNumber);
        }
    }

    tokens.emplace_back(TokenType::_EOF);

    return tokens;
}

void Lexer::advance() {
    mPos.advance(mCurrentChar);

    if (mPos.index < mText.size()) {
        mCurrentChar = &mText[mPos.index];
    } else {
        mCurrentChar = nullptr;
    }
}
