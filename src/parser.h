#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

struct IExpr {
    virtual ~IExpr() = default;
};

using ExprPtr = std::unique_ptr<IExpr>;

struct NumberExpr : public IExpr {
    int n;

    explicit NumberExpr(int n) : n(n) {}

    friend std::ostream& operator<<(std::ostream& os, const NumberExpr& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct BinOpExpr : public IExpr {
    ExprPtr leftNode;
    ExprPtr rightNode;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
        leftNode(std::move(ln)),
        rightNode(std::move(rn)),
        opToken(std::move(opTok)) {}

//    friend std::ostream& operator<<(std::ostream& os, const BinOpExpr& bn) {
//        os << std::format("{} {} {}", leftNode, opToken, rightNode);
//        return os;
//    }
};



class Parser {
public:
    Parser(const char* mFileName);

    void setTokens(std::vector<Token>& tokens);

    ExprPtr parse();

private:
    ExprPtr parseExpr();

    ExprPtr parseNumber();

    void parseParen(TokenType expected);

    Token advance();

    std::vector<Token> mTokens;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
};
