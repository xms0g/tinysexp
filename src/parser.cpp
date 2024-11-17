#include "parser.h"

Parser::Parser(): mTokenIndex(-1) {}

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
    if (mCurrentToken.type == TokenType::INT) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<NumberNode>(std::stoi(token.value) - 48);
    }

    return nullptr;
}

NodePtr Parser::term() {
    NodePtr left = factor();

    while (mCurrentToken.type == TokenType::MUL || mCurrentToken.type == TokenType::DIV) {
        Token token = mCurrentToken;
        advance();

        NodePtr right = factor();
        left = std::make_unique<BinOpNode>(left, right, token);
    }

    return left;
}

NodePtr Parser::expr() {
    if (mCurrentToken.type == TokenType::LBRACKET || mCurrentToken.type == TokenType::RBRACKET)
        advance();

    NodePtr left = term();

    while (mCurrentToken.type == TokenType::PLUS || mCurrentToken.type == TokenType::MINUS) {
        Token token = mCurrentToken;
        advance();

        NodePtr right = term();
        left = std::make_unique<BinOpNode>(left, right, token);
    }

    return left;
}


