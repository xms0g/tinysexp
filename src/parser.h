#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

struct INode {
    virtual ~INode() = default;
};

struct NumberNode : public INode {
    int n;

    explicit NumberNode(int n) : n(n) {}

    friend std::ostream& operator<<(std::ostream& os, const NumberNode& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct BinOpNode : public INode {
    std::unique_ptr<INode> leftNode;
    std::unique_ptr<INode> rightNode;
    Token opToken;

    BinOpNode(std::unique_ptr<INode>& ln, std::unique_ptr<INode>& rn, Token opTok) :
        leftNode(std::move(ln)),
        rightNode(std::move(rn)),
        opToken(std::move(opTok)) {}

//    friend std::ostream& operator<<(std::ostream& os, const BinOpNode& bn) {
//        os << std::format("{} {} {}", leftNode, opToken, rightNode);
//        return os;
//    }
};

class Parser {
public:
    Parser();

    void setTokens(std::vector<Token>& tokens);

    std::unique_ptr<INode> parse();

private:
    Token advance();

    std::unique_ptr<INode> factor();

    std::unique_ptr<INode> term();

    std::unique_ptr<INode> expr();

    std::vector<Token> mTokens;
    Token mCurrentToken{};
    int mTokenIndex;
};
