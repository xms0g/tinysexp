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
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
            lhs(std::move(ln)),
            rhs(std::move(rn)),
            opToken(std::move(opTok)) {}
};

struct DotimesExpr : public IExpr {
    ExprPtr iterationCount;
    ExprPtr statement;

    DotimesExpr(ExprPtr& iterCount, ExprPtr& statement) :
            iterationCount(std::move(iterCount)),
            statement(std::move(statement)) {}
};

struct PrintExpr : public IExpr {
    ExprPtr sexpr;

    PrintExpr(ExprPtr& expr) : sexpr(std::move(expr)) {}
};

struct LetExpr : public IExpr {
    ExprPtr sexpr;
    std::vector<ExprPtr> variables;

    LetExpr(ExprPtr& expr, std::vector<ExprPtr>& variables) :
            sexpr(std::move(expr)),
            variables(std::move(variables)) {}
};

struct VarExpr : public IExpr {
    std::string name;
    ExprPtr value;

    VarExpr(std::string& name) : name(std::move(name)) {}
};

class Parser {
public:
    Parser(const char* mFileName, Lexer& lexer);

    void setTokens(std::vector<Token>& tokens);

    ExprPtr parse();

private:
    Token advance();

    ExprPtr parseExpr();

    ExprPtr parseSExpr();

    ExprPtr parsePrint();

    ExprPtr parseDotimes();

    ExprPtr parseLet();

    ExprPtr parseVar();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    void consume(TokenType expected);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
};
