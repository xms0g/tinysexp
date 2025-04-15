#include "lexer.h"
#include <cstring>
#include "exceptions.hpp"

Lexer::Lexer(const char* fn, std::string text) : text(std::move(text)), pos(-1, 0, -1), fileName(fn) {
    advance();
}

void Lexer::process() {
    while (currentChar) {
        if (currentChar[0] == '\t' || currentChar[0] == '\n' || std::isspace(currentChar[0])) {
            advance();
        } else if (!std::strncmp("dotimes", currentChar, 7)) {
            tokens.emplace_back(TokenType::DOTIMES);
            advance(7);
        } else if (!std::strncmp("return", currentChar, 6)) {
            tokens.emplace_back(TokenType::RETURN);
            advance(6);
        } else if (!std::strncmp("loop", currentChar, 4)) {
            tokens.emplace_back(TokenType::LOOP);
            advance(4);
        } else if (!std::strncmp("let", currentChar, 3)) {
            tokens.emplace_back(TokenType::LET);
            advance(3);
        } else if (!std::strncmp("setq", currentChar, 4)) {
            tokens.emplace_back(TokenType::SETQ);
            advance(4);
        } else if (!std::strncmp("if", currentChar, 2)) {
            tokens.emplace_back(TokenType::IF);
            advance(2);
        } else if (!std::strncmp("when", currentChar, 4)) {
            tokens.emplace_back(TokenType::WHEN);
            advance(4);
        } else if (!std::strncmp("cond", currentChar, 4)) {
            tokens.emplace_back(TokenType::COND);
            advance(4);
        } else if (!std::strncmp("defvar", currentChar, 4)) {
            tokens.emplace_back(TokenType::DEFVAR);
            advance(6);
        } else if (!std::strncmp("defconstant", currentChar, 11)) {
            tokens.emplace_back(TokenType::DEFCONST);
            advance(11);
        } else if (!std::strncmp("defun", currentChar, 5)) {
            tokens.emplace_back(TokenType::DEFUN);
            advance(5);
        } else if (!std::strncmp("nil", currentChar, 3)) {
            tokens.emplace_back(TokenType::NIL);
            advance(3);
        } else if (!std::strncmp("logand", currentChar, 6)) {
            tokens.emplace_back(TokenType::LOGAND);
            advance(6);
        } else if (!std::strncmp("logior", currentChar, 6)) {
            tokens.emplace_back(TokenType::LOGIOR);
            advance(6);
        } else if (!std::strncmp("logxor", currentChar, 6)) {
            tokens.emplace_back(TokenType::LOGXOR);
            advance(6);
        } else if (!std::strncmp("lognor", currentChar, 6)) {
            tokens.emplace_back(TokenType::LOGNOR);
            advance(6);
        } else if (!std::strncmp("and", currentChar, 3)) {
            tokens.emplace_back(TokenType::AND);
            advance(3);
        } else if (!std::strncmp("or", currentChar, 2)) {
            tokens.emplace_back(TokenType::OR);
            advance(2);
        } else if (!std::strncmp("not", currentChar, 3)) {
            tokens.emplace_back(TokenType::NOT);
            advance(3);
        } else if ((currentChar[0] == 't' && currentChar[1] == ' ') ||
                   (currentChar[0] == 't' && currentChar[1] == ')')) {
            tokens.emplace_back(TokenType::T);
            advance();
        } else if (std::isalpha(currentChar[0])) {
            std::string token;

            while (currentChar && (std::isalnum(currentChar[0]) || currentChar[0] == '_'  || currentChar[0] == '-')) {
                token += currentChar[0];
                advance();
            }

            tokens.emplace_back(TokenType::VAR, token);
        } else if (std::isdigit(currentChar[0])) {
            std::string token;
            bool isDouble{false};

            while (currentChar && (std::isalnum(currentChar[0]) || currentChar[0] == '.')) {
                if (std::isalpha(currentChar[0]))
                    throw IllegalCharError(fileName, (token + currentChar[0]).c_str(), pos.lineNumber);

                if (!isDouble && currentChar[0] == '.') isDouble = true;

                token += currentChar[0];
                advance();
            }

            tokens.emplace_back(isDouble ? TokenType::DOUBLE : TokenType::INT, token);
        } else if (currentChar[0] == '"') {
            std::string data;

            advance();
            while (currentChar && currentChar[0] != '"') {
                data += currentChar[0];
                advance();
            }
            advance();
            tokens.emplace_back(TokenType::STRING, data);
        } else if (!std::strncmp("/=", currentChar, 2)) {
            tokens.emplace_back(TokenType::NEQUAL);
            advance(2);
        } else if (!std::strncmp(">=", currentChar, 2)) {
            tokens.emplace_back(TokenType::GREATER_THEN_EQ);
            advance(2);
        } else if (!std::strncmp("<=", currentChar, 2)) {
            tokens.emplace_back(TokenType::LESS_THEN_EQ);
            advance(2);
        } else if (currentChar[0] == '+') {
            tokens.emplace_back(TokenType::PLUS);
            advance();
        } else if (currentChar[0] == '-') {
            tokens.emplace_back(TokenType::MINUS);
            advance();
        } else if (currentChar[0] == '*') {
            tokens.emplace_back(TokenType::MUL);
            advance();
        } else if (currentChar[0] == '/') {
            tokens.emplace_back(TokenType::DIV);
            advance();
        } else if (currentChar[0] == '=') {
            tokens.emplace_back(TokenType::EQUAL);
            advance();
        } else if (currentChar[0] == '>') {
            tokens.emplace_back(TokenType::GREATER_THEN);
            advance();
        } else if (currentChar[0] == '<') {
            tokens.emplace_back(TokenType::LESS_THEN);
            advance();
        } else if (currentChar[0] == '(') {
            tokens.emplace_back(TokenType::LPAREN);
            advance();
        } else if (currentChar[0] == ')') {
            tokens.emplace_back(TokenType::RPAREN);
            advance();
        } else {
            throw IllegalCharError(fileName, std::string(1, currentChar[0]).c_str(), pos.lineNumber);
        }
    }

    tokens.emplace_back(TokenType::EOF_);
}

void Lexer::advance() {
    pos.advance(currentChar);

    if (pos.index < text.size()) {
        currentChar = &text[pos.index];
    } else {
        currentChar = nullptr;
    }
}

void Lexer::advance(const int step) {
    for (int i = 0; i < step; ++i) advance();
}
