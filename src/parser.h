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
    ExprPtr pair;

    explicit SetqExpr(ExprPtr& pair_) :
            pair(std::move(pair_)) {}

    MAKE_VISITABLE
};

struct DefvarExpr : IExpr {
    ExprPtr pair;

    explicit DefvarExpr(ExprPtr& pair_) :
            pair(std::move(pair_)) {}

    MAKE_VISITABLE
};

struct DefconstExpr : IExpr {
    ExprPtr pair;

    explicit DefconstExpr(ExprPtr& pair_) :
            pair(std::move(pair_)) {}

    MAKE_VISITABLE
};

struct DefunExpr : IExpr {
    ExprPtr name;
    std::vector<ExprPtr> args;
    std::vector<ExprPtr> forms;

    DefunExpr(ExprPtr& name_, std::vector<ExprPtr>& params_, std::vector<ExprPtr>& body_) :
            name(std::move(name_)),
            args(std::move(params_)),
            forms(std::move(body_)) {}

    MAKE_VISITABLE
};

struct FuncCallExpr : IExpr {
    ExprPtr name;
    std::vector<ExprPtr> args;

    FuncCallExpr(ExprPtr& name_, std::vector<ExprPtr>& params_) :
            name(std::move(name_)),
            args(std::move(params_)) {}

    MAKE_VISITABLE
};

struct IfExpr : IExpr {
    ExprPtr test, then, else_;

    IfExpr(ExprPtr& test_, ExprPtr& then_, ExprPtr e = nullptr) :
            test(std::move(test_)),
            then(std::move(then_)),
            else_(std::move(e)) {}

    MAKE_VISITABLE
};

struct WhenExpr : IfExpr {
    WhenExpr(ExprPtr& test, ExprPtr& then, ExprPtr else_ = nullptr) :
            IfExpr(test, then, std::move(else_)) {}
};

struct CondExpr : IExpr {
    std::vector<std::pair<ExprPtr, std::vector<ExprPtr>>> variants;

    explicit CondExpr(std::vector<std::pair<ExprPtr, std::vector<ExprPtr>>>& variants_) :
            variants(std::move(variants_)) {}

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

    void expect(TokenType expected, const char* errorStr);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
};

namespace cast {
inline std::shared_ptr<BinOpExpr> toBinop(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<BinOpExpr>(expr);
}

inline std::shared_ptr<DotimesExpr> toDotimes(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<DotimesExpr>(expr);
}

inline std::shared_ptr<LetExpr> toLet(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<LetExpr>(expr);
}

inline std::shared_ptr<SetqExpr> toSetq(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<SetqExpr>(expr);
}

inline std::shared_ptr<DefvarExpr> toDefvar(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<DefvarExpr>(expr);
}

inline std::shared_ptr<DefconstExpr> toDefconstant(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<DefconstExpr>(expr);
}

inline std::shared_ptr<DefunExpr> toDefun(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<DefunExpr>(expr);
}

inline std::shared_ptr<FuncCallExpr> toFuncCall(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<FuncCallExpr>(expr);
}

inline std::shared_ptr<IfExpr> toIf(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<IfExpr>(expr);
}

inline std::shared_ptr<WhenExpr> toWhen(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<WhenExpr>(expr);
}

inline std::shared_ptr<CondExpr> toCond(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<CondExpr>(expr);
}

inline std::shared_ptr<VarExpr> toVar(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<VarExpr>(expr);
}

inline std::shared_ptr<StringExpr> toString(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<StringExpr>(expr);
}

inline std::shared_ptr<IntExpr> toInt(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<IntExpr>(expr);
}

inline std::shared_ptr<DoubleExpr> toDouble(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<DoubleExpr>(expr);
}

inline std::shared_ptr<TExpr> toT(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<TExpr>(expr);
}

inline std::shared_ptr<NILExpr> toNIL(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<NILExpr>(expr);
}
}

#endif
