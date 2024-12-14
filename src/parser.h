#ifndef TINYSEXP_PARSER_H
#define TINYSEXP_PARSER_H

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

    BinOpExpr(ExprPtr& lhs_, ExprPtr& rhs_, Token opTok) :
            lhs(std::move(lhs_)),
            rhs(std::move(rhs_)),
            opToken(std::move(opTok)) {}

    MAKE_VISITABLE
};

struct DotimesExpr : IExpr {
    ExprPtr iterationCount;
    std::vector<ExprPtr> statements;

    DotimesExpr(ExprPtr& iterationCount_, std::vector<ExprPtr>& statements_) :
            iterationCount(std::move(iterationCount_)),
            statements(std::move(statements_)) {}

    MAKE_VISITABLE
};

struct LetExpr : IExpr {
    std::vector<ExprPtr> variables;
    std::vector<ExprPtr> body;

    LetExpr(std::vector<ExprPtr>& variables_, std::vector<ExprPtr>& body_) :
            body(std::move(body_)),
            variables(std::move(variables_)) {}

    MAKE_VISITABLE
};

struct SetqExpr : IExpr {
    ExprPtr var;

    explicit SetqExpr(ExprPtr& var_) :
            var(std::move(var_)) {}

    MAKE_VISITABLE
};

struct DefvarExpr : IExpr {
    ExprPtr var;

    explicit DefvarExpr(ExprPtr& var_) :
            var(std::move(var_)) {}

    MAKE_VISITABLE
};

struct DefconstExpr : IExpr {
    ExprPtr var;

    explicit DefconstExpr(ExprPtr& var_) :
            var(std::move(var_)) {}

    MAKE_VISITABLE
};

struct DefunExpr : IExpr {
    std::vector<ExprPtr> params;
    std::vector<ExprPtr> body;

    DefunExpr(std::vector<ExprPtr> params_, std::vector<ExprPtr> body_) :
            params(std::move(params_)),
            body(std::move(body_)) {}

    MAKE_VISITABLE
};

struct VarExpr : IExpr {
    ExprPtr name;
    ExprPtr value;

    VarExpr(ExprPtr& name_, ExprPtr& value_) :
            name(std::move(name_)),
            value(std::move(value_)) {}

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

    ExprPtr parseDotimes();

    ExprPtr parseLet();

    ExprPtr parseSetq();

    ExprPtr parseDefvar();

    ExprPtr parseDefconst();

    ExprPtr parseDefun();

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

#endif
