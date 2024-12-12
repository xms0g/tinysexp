#pragma once

#include <unordered_map>
#include <utility>
#include <memory>
#include "lexer.h"

struct IExpr;
using ExprPtr = std::shared_ptr<IExpr>;

#include "visitor.hpp"

#define MAKE_VISITABLE virtual void accept(ExprVisitor& visitor) override { visitor.visit(*this); }

struct IExpr {
    std::shared_ptr<IExpr> child;

    virtual ~IExpr() = default;

    virtual void accept(ExprVisitor& visitor) = 0;
};

struct IntExpr : IExpr {
    int n;

    explicit IntExpr(int n_) : n(n_) {}

    MAKE_VISITABLE
};

struct DoubleExpr : IExpr {
    double n;

    explicit DoubleExpr(double n_) : n(n_) {}

    MAKE_VISITABLE
};

struct StringExpr : IExpr {
    std::string data;

    StringExpr() = default;

    explicit StringExpr(std::string& str) : data(str) {}

    MAKE_VISITABLE
};

struct NILExpr : IExpr {
    NILExpr() = default;

    MAKE_VISITABLE
};

struct BinOpExpr : IExpr {
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
            lhs(std::move(ln)),
            rhs(std::move(rn)),
            opToken(std::move(opTok)) {}

    MAKE_VISITABLE
};

struct DotimesExpr : IExpr {
    ExprPtr iterationCount;
    std::vector<ExprPtr> statements;

    DotimesExpr(ExprPtr& iterCount, std::vector<ExprPtr>& statements) :
            iterationCount(std::move(iterCount)),
            statements(std::move(statements)) {}

    MAKE_VISITABLE
};

struct PrintExpr : IExpr {
    ExprPtr sexpr;

    explicit PrintExpr(ExprPtr& expr) : sexpr(std::move(expr)) {}

    MAKE_VISITABLE
};

struct ReadExpr : IExpr {
    ReadExpr() = default;

    MAKE_VISITABLE
};

struct LetExpr : IExpr {
    std::vector<ExprPtr> sexprs;
    std::vector<ExprPtr> variables;

    LetExpr(std::vector<ExprPtr>& sexprs, std::vector<ExprPtr>& variables) :
            sexprs(std::move(sexprs)),
            variables(std::move(variables)) {}

    MAKE_VISITABLE
};

struct SetqExpr : IExpr {
    ExprPtr var;

    explicit SetqExpr(ExprPtr& var) :
            var(std::move(var)) {}

    MAKE_VISITABLE
};

struct DefvarExpr : IExpr {
    ExprPtr var;

    explicit DefvarExpr(ExprPtr& var) :
            var(std::move(var)) {}

    MAKE_VISITABLE
};

struct VarExpr : IExpr {
    ExprPtr name;
    ExprPtr value;

    VarExpr(ExprPtr& name, ExprPtr& value) : name(std::move(name)), value(std::move(value)) {}

    MAKE_VISITABLE
};

class Parser {
public:
    Parser(const char* mFileName, Lexer& lexer);

    ExprPtr parse();

private:
    Token advance();

    ExprPtr parseExpr();

    ExprPtr parseSExpr();

    ExprPtr parsePrint();

    ExprPtr parseRead();

    ExprPtr parseDotimes();

    ExprPtr parseLet();

    ExprPtr parseSetq();

    ExprPtr parseDefvar();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    void consume(TokenType expected, const char* errorStr);

    ExprPtr checkVarError(ExprPtr& var);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
    std::unordered_map<std::string, ExprPtr> symbolTable;
};
