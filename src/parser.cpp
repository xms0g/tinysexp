#include "parser.h"

Parser::Parser(): mTokenIndex(-1) {}

void Parser::setTokens(std::vector<Token>& tokens) {
    mTokens = std::move(tokens);
    advance();
}

std::unique_ptr<INode> Parser::parse() {
    return expr();
}

Token Parser::advance() {
    ++mTokenIndex;

    if (mTokenIndex < mTokens.size()) {
        mCurrentToken = mTokens[mTokenIndex];
    }

    return mCurrentToken;
}

std::unique_ptr<INode> Parser::factor() {
    if (mCurrentToken.type == TokenType::INT) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<NumberNode>(std::stoi(token.value) - 48);
    }

    return nullptr;
}

std::unique_ptr<INode> Parser::term() {

    std::unique_ptr<INode> left = factor();

    while (mCurrentToken.type == TokenType::MUL || mCurrentToken.type == TokenType::DIV) {
        Token token = mCurrentToken;
        advance();

        std::unique_ptr<INode> right = factor();
        left = std::make_unique<BinOpNode>(left, right, token);
    }

    return left;
}

std::unique_ptr<INode> Parser::expr() {
    if (mCurrentToken.type == TokenType::LBRACKET || mCurrentToken.type == TokenType::RBRACKET)
        advance();

    std::unique_ptr<INode> left = term();

    while (mCurrentToken.type == TokenType::PLUS || mCurrentToken.type == TokenType::MINUS) {
        Token token = mCurrentToken;
        advance();

        std::unique_ptr<INode> right = term();
        left = std::make_unique<BinOpNode>(left, right, token);
    }

    return left;
}


