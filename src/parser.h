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
    const bool value{false};

    NILExpr() = default;

    MAKE_VISITABLE
};

struct TExpr : IExpr {
    const bool value{true};

    TExpr() = default;

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

struct LoopExpr : IExpr {
    std::vector<ExprPtr> sexprs;

    LoopExpr(std::vector<ExprPtr>& sexprs_): sexprs(std::move(sexprs_)) {}

    MAKE_VISITABLE
};

struct LetExpr : IExpr {
    std::vector<ExprPtr> bindings;
    std::vector<ExprPtr> body;

    LetExpr(std::vector<ExprPtr>& bindings_, std::vector<ExprPtr>& body_) :
            body(std::move(body_)),
            bindings(std::move(bindings_)) {}

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
    ExprPtr name;
    std::vector<ExprPtr> params;
    std::vector<ExprPtr> body;

    DefunExpr(ExprPtr& name_, std::vector<ExprPtr>& params_, std::vector<ExprPtr>& body_) :
            name(std::move(name_)),
            params(std::move(params_)),
            body(std::move(body_)) {}

    MAKE_VISITABLE
};

struct FuncCallExpr : IExpr {
    ExprPtr name;
    std::vector<ExprPtr> params;

    FuncCallExpr(ExprPtr& name_, std::vector<ExprPtr>& params_) :
            name(std::move(name_)),
            params(std::move(params_)) {}

    MAKE_VISITABLE
};

struct IfExpr : IExpr {
    ExprPtr cond, true_, false_;

    IfExpr(ExprPtr& cond_, ExprPtr& t, ExprPtr f = nullptr) :
            cond(std::move(cond_)),
            true_(std::move(t)),
            false_(std::move(f)) {}

    MAKE_VISITABLE
};

struct WhenExpr : IfExpr {
    WhenExpr(ExprPtr& cond, ExprPtr& t, ExprPtr f = nullptr) :
            IfExpr(cond, t, f) {}
};

struct CondExpr : IExpr {
    std::vector<std::pair<ExprPtr, std::vector<ExprPtr>>> body;

    explicit CondExpr(std::vector<std::pair<ExprPtr, std::vector<ExprPtr>>>& body_) :
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

    ExprPtr parseLoop();

    ExprPtr parseLet();

    ExprPtr parseSetq();

    ExprPtr parseDefvar();

    ExprPtr parseDefconst();

    ExprPtr parseDefun();

    ExprPtr parseFuncCall();

    ExprPtr parseIf();

    ExprPtr parseWhen();

    ExprPtr parseCond();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    ExprPtr createVar();

    std::tuple<ExprPtr, ExprPtr, ExprPtr> createCond();

    void consume(TokenType expected, const char* errorStr);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
};

#endif
