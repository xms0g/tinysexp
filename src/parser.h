#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

struct IExpr {
    virtual ~IExpr() = default;

    virtual std::string codegen() = 0;
};

using ExprPtr = std::unique_ptr<IExpr>;

struct NumberExpr : public IExpr {
    int n;

    explicit NumberExpr(int n) : n(n) {}

    std::string codegen() override {
        std::string code;

        for (int i = 0; i < n; ++i) {
            code += "+";

        }

        return code;
    }

    friend std::ostream& operator<<(std::ostream& os, const NumberExpr& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct BinOpExpr : public IExpr {
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
            lhs(std::move(ln)),
            rhs(std::move(rn)),
            opToken(std::move(opTok)) {}

    std::string codegen() override;

    int codegen1(ExprPtr& expr);
};


class Parser {
public:
    explicit Parser(const char* mFileName);

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
