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

    int codegen1(ExprPtr& expr) {
        int lhsi, rhsi;

        if (dynamic_cast<NumberExpr*>(expr.get())) {
            lhsi = dynamic_cast<NumberExpr*>(expr.get())->n;
            return lhsi;
        } else {
            auto binop = dynamic_cast<BinOpExpr*>(expr.get());
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
    }

    std::string codegen() override {
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

//    friend std::ostream& operator<<(std::ostream& os, const BinOpExpr& bn) {
//        os << std::format("{} {} {}", lhs, opToken, rhs);
//        return os;
//    }
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
