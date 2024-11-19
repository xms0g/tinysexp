#include "parser.h"
#include "exceptions.hpp"

Parser::Parser(const char* fn) : mFileName(fn), mTokenIndex(-1) {}

void Parser::setTokens(std::vector<Token>& tokens) {
    mTokens = std::move(tokens);
    advance();
}

ExprPtr Parser::parse() {
    return parseExpr();
}

ExprPtr Parser::parseExpr() {
    ExprPtr left, right;

    parseParen(TokenType::LPAREN);

    if (mCurrentToken.type == TokenType::MUL || mCurrentToken.type == TokenType::DIV ||
        mCurrentToken.type == TokenType::PLUS || mCurrentToken.type == TokenType::MINUS) {
        Token token = mCurrentToken;
        advance();
        left = parseNumber();

        if (mCurrentToken.type != TokenType::INT) {
            right = parseExpr();
        } else {
            right = parseNumber();
        }

        left = std::make_unique<BinOpExpr>(left, right, token);
    } else {
        throw InvalidSyntaxError(mFileName, "Missing Operator: must be +,-,*,/", 0);
    }

    parseParen(TokenType::RPAREN);

    return left;
}

ExprPtr Parser::parseNumber() {
    if (mCurrentToken.type == TokenType::INT) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<NumberExpr>(std::stoi(token.value) - 48);
    }

    throw InvalidSyntaxError(mFileName, "Expected INT", 0);
}

void Parser::parseParen(TokenType expected) {
    if (mCurrentToken.type != expected) {
        throw InvalidSyntaxError(mFileName, "Missing Parenthesis", 0);
    } else advance();
}

Token Parser::advance() {
    ++mTokenIndex;

    if (mTokenIndex < mTokens.size()) {
        mCurrentToken = mTokens[mTokenIndex];
    }

    return mCurrentToken;
}

std::string BinOpExpr::codegen() {
    std::string code;

    int lhsi = codegen1(lhs);
    int rhsi = codegen1(rhs);

    switch (opToken.type) {
        case TokenType::PLUS:
            for (int i = 0; i < lhsi + rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MINUS:
            for (int i = 0; i < lhsi - rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::DIV:
            for (int i = 0; i < lhsi / rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MUL:
            for (int i = 0; i < lhsi * rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::PRINT:
            code += ".";
            break;
    }
    return code;
}

int BinOpExpr::codegen1(ExprPtr& expr) {
    int lhsi, rhsi;

    if (dynamic_cast<NumberExpr*>(expr.get())) {
        lhsi = dynamic_cast<NumberExpr*>(expr.get())->n;

        return lhsi;
    } else {
        auto* binop = dynamic_cast<BinOpExpr*>(expr.get());

        lhsi = codegen1(binop->lhs);
        rhsi = codegen1(binop->rhs);

        switch (binop->opToken.type) {
            case TokenType::PLUS:
                return lhsi + rhsi;
            case TokenType::MINUS:
                return lhsi - rhsi;
            case TokenType::DIV:
                return lhsi / rhsi;
            case TokenType::MUL:
                return lhsi * rhsi;
        }
    }
    return 0;
}


