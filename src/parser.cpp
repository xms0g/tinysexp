#include <cassert>
#include "parser.h"

Parser::Parser() : mTokenIndex(-1), openParanCount(0) {}

void Parser::setTokens(std::vector<Token>& tokens) {
    mTokens = std::move(tokens);
    advance();
}

NodePtr Parser::parse() {
    return expr();
}

Token Parser::advance() {
    ++mTokenIndex;

    if (mTokenIndex < mTokens.size()) {
        mCurrentToken = mTokens[mTokenIndex];
    }

    return mCurrentToken;
}

NodePtr Parser::factor() {
    Token token = mCurrentToken;
    advance();
    return std::make_unique<NumberNode>(std::stoi(token.value) - 48);
}

NodePtr Parser::term() {
    NodePtr left, right;

    checkParen();

    while (mCurrentToken.type == TokenType::MUL || mCurrentToken.type == TokenType::DIV ||
           mCurrentToken.type == TokenType::PLUS || mCurrentToken.type == TokenType::MINUS) {
        Token token = mCurrentToken;
        advance();
        left = factor();

        if (mCurrentToken.type != TokenType::INT) {
            right = term();
        } else
            right = factor();

        left = std::make_unique<BinOpNode>(left, right, token);
    }

    if (!left) {
        left = factor();
    }

    checkParen();

    return left;
}

NodePtr Parser::expr() {
    NodePtr left;

    checkParen();

    while (mCurrentToken.type != TokenType::INT && mCurrentToken.type != TokenType::RPAREN) {
        Token token = mCurrentToken;
        advance();
        left = term();
        NodePtr right = term();
        left = std::make_unique<BinOpNode>(left, right, token);
    }

    if (!left) {
        left = term();
    }

    checkParen();

    return left;
}

void Parser::checkParen() {
    if (mCurrentToken.type == TokenType::LPAREN) {
        ++openParanCount;
        advance();
    } else if (mCurrentToken.type == TokenType::RPAREN) {
        --openParanCount;
        advance();
    }
}


