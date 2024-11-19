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

Token Parser::advance() {
    ++mTokenIndex;

    if (mTokenIndex < mTokens.size()) {
        mCurrentToken = mTokens[mTokenIndex];
    }

    return mCurrentToken;
}

ExprPtr Parser::parseExpr() {
    ExprPtr left, right;

    parseParen(TokenType::LPAREN);

    if (mCurrentToken.type == TokenType::MUL || mCurrentToken.type == TokenType::DIV ||
        mCurrentToken.type == TokenType::PLUS || mCurrentToken.type == TokenType::MINUS) {
        Token token = mCurrentToken;
        advance();
        left = parseNumber();

        if (mCurrentToken.type == TokenType::LPAREN) {
            right = parseExpr();
        } else {
            right = parseNumber();
        }

        left = std::make_unique<BinOpExpr>(left, right, token);
    } else if (mCurrentToken.type == TokenType::PRINT) {
        Token token = mCurrentToken;
        advance();
        left = std::make_unique<PrintExpr>();
        right = parseExpr();
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