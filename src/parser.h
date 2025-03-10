#ifndef PARSER_H
#define PARSER_H

#include <utility>
#include <memory>
#include "lexer.h"

enum class SymbolType {
    LOCAL,
    PARAM,
    GLOBAL
};

struct IExpr {
    std::shared_ptr<IExpr> child;

    virtual ~IExpr() = default;
};

using ExprPtr = std::shared_ptr<IExpr>;

struct IntExpr final : IExpr {
    int n;

    explicit IntExpr(const int n_) : n(n_) {
    }
};

struct DoubleExpr final : IExpr {
    double n;

    explicit DoubleExpr(const double n_) : n(n_) {
    }
};

struct StringExpr final : IExpr {
    std::string data;

    StringExpr() = default;

    explicit StringExpr(std::string& str) : data(std::move(str)) {
    }
};

struct NILExpr final : IExpr {
    const bool value{false};

    NILExpr() = default;
};

struct TExpr final : IExpr {
    const bool value{true};

    TExpr() = default;
};

struct BinOpExpr final : IExpr {
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& lhs_, ExprPtr& rhs_, Token opTok) : lhs(std::move(lhs_)),
                                                           rhs(std::move(rhs_)),
                                                           opToken(std::move(opTok)) {
    }
};

struct DotimesExpr final : IExpr {
    ExprPtr iterationCount;
    std::vector<ExprPtr> statements;

    DotimesExpr(ExprPtr& iterationCount_, std::vector<ExprPtr>& statements_) : iterationCount(
                                                                                   std::move(iterationCount_)),
                                                                               statements(std::move(statements_)) {
    }
};

struct LoopExpr final : IExpr {
    std::vector<ExprPtr> sexprs;

    explicit LoopExpr(std::vector<ExprPtr>& sexprs_) : sexprs(std::move(sexprs_)) {
    }
};

struct LetExpr final : IExpr {
    std::vector<ExprPtr> bindings;
    std::vector<ExprPtr> body;

    LetExpr(std::vector<ExprPtr>& bindings_, std::vector<ExprPtr>& body_) : bindings(std::move(bindings_)),
                                                                            body(std::move(body_)) {
    }
};

struct SetqExpr final : IExpr {
    ExprPtr pair;

    explicit SetqExpr(ExprPtr& pair_) : pair(std::move(pair_)) {
    }
};

struct DefvarExpr final : IExpr {
    ExprPtr pair;

    explicit DefvarExpr(ExprPtr& pair_) : pair(std::move(pair_)) {
    }
};

struct DefconstExpr final : IExpr {
    ExprPtr pair;

    explicit DefconstExpr(ExprPtr& pair_) : pair(std::move(pair_)) {
    }
};

struct DefunExpr final : IExpr {
    ExprPtr name;
    std::vector<ExprPtr> args;
    std::vector<ExprPtr> forms;

    DefunExpr(ExprPtr& name_, std::vector<ExprPtr>& params_, std::vector<ExprPtr>& body_) : name(std::move(name_)),
        args(std::move(params_)),
        forms(std::move(body_)) {
    }
};

struct FuncCallExpr final : IExpr {
    ExprPtr name;
    ExprPtr returnType;
    std::vector<ExprPtr> args;

    FuncCallExpr(ExprPtr& name_, std::vector<ExprPtr>& params_) : name(std::move(name_)),
                                                                  args(std::move(params_)) {
    }
};

struct ReturnExpr final : IExpr {
    ExprPtr arg;

    explicit ReturnExpr(ExprPtr& arg_) : arg(std::move(arg_)) {
    }
};

struct IfExpr final : IExpr {
    ExprPtr test, then, else_;

    IfExpr(ExprPtr& test_, ExprPtr& then_, ExprPtr e = nullptr) : test(std::move(test_)),
                                                                  then(std::move(then_)),
                                                                  else_(std::move(e)) {
    }
};

struct WhenExpr final : IExpr {
    ExprPtr test;
    std::vector<ExprPtr> then;

    WhenExpr(ExprPtr& test_, std::vector<ExprPtr>& then_) : test(std::move(test_)),
                                                            then(std::move(then_)) {
    }
};

struct CondExpr final : IExpr {
    std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > > variants;

    explicit CondExpr(std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > >& variants_) : variants(
        std::move(variants_)) {
    }
};

struct VarExpr final : IExpr {
    ExprPtr name;
    ExprPtr value;
    SymbolType sType;

    VarExpr(ExprPtr& name_, ExprPtr& value_, const SymbolType type = SymbolType::GLOBAL) : name(std::move(name_)),
        value(std::move(value_)),
        sType(type) {
    }
};

struct Uninitialized final : IExpr {
};

class Parser {
public:
    Parser(const char* fn, Lexer& lexer);

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

    ExprPtr parseReturn();

    ExprPtr parseIf();

    ExprPtr parseWhen();

    ExprPtr parseCond();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    ExprPtr createVar(bool isConstant = false);

    void consume(TokenType expected, const char* errorStr);

    void expect(TokenType expected, const char* errorStr) const;

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

inline std::shared_ptr<LoopExpr> toLoop(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<LoopExpr>(expr);
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

inline std::shared_ptr<ReturnExpr> toReturn(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<ReturnExpr>(expr);
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

inline std::shared_ptr<Uninitialized> toUninitialized(const ExprPtr& expr) {
    return std::dynamic_pointer_cast<Uninitialized>(expr);
}
}

#endif
