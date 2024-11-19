#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

struct INode {
    virtual ~INode() = default;
};

using NodePtr = std::unique_ptr<INode>;

struct NumberNode : public INode {
    int n;

    explicit NumberNode(int n) : n(n) {}

    friend std::ostream& operator<<(std::ostream& os, const NumberNode& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct BinOpNode : public INode {
    NodePtr leftNode;
    NodePtr rightNode;
    Token opToken;

    BinOpNode(NodePtr& ln, NodePtr& rn, Token opTok) :
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

    NodePtr parse();

private:
    NodePtr parseExpr();

    NodePtr parseNumber();

    void parseParen(TokenType expected);

    Token advance();

    std::vector<Token> mTokens;
    Token mCurrentToken{};
    int mTokenIndex;
    int openParanCount;
};
