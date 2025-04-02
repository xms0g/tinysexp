#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stack>
#include <unordered_map>
#include "parser.h"

struct Symbol {
    std::string name;
    ExprPtr value;
    SymbolType sType;
    bool isConstant{};
};

class ScopeTracker {
public:
    void enter(const std::string& scopeName);

    void exit();

    [[nodiscard]] std::string& scopeName();

    [[nodiscard]] size_t level() const;

    void bind(const std::string& name, const Symbol& symbol);

    Symbol lookup(const std::string& name);

    Symbol lookupCurrent(const std::string& name);

private:
    using ScopeType = std::unordered_map<std::string, Symbol>;
    std::stack<ScopeType> mSymbolTable;
    std::stack<std::string> mScopeNames;
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const char* fn);

    void analyze(const ExprPtr& ast);

private:
    /* Name Resolutions */

    ExprPtr exprResolve(const ExprPtr& ast);

    ExprPtr binopResolve(BinOpExpr& binop);

    ExprPtr dotimesResolve(const DotimesExpr& dotimes);

    ExprPtr loopResolve(const LoopExpr& loop);

    ExprPtr letResolve(const LetExpr& let);

    ExprPtr setqResolve(const SetqExpr& setq);

    void defvarResolve(const DefvarExpr& defvar);

    void defconstResolve(const DefconstExpr& defconst);

    ExprPtr defunResolve(const ExprPtr& defun);

    ExprPtr funcCallResolve(FuncCallExpr& funcCall, bool isParam = false);

    void returnResolve(const ReturnExpr& return_);

    ExprPtr ifResolve(IfExpr& if_);

    ExprPtr whenResolve(WhenExpr& when);

    ExprPtr condResolve(CondExpr& cond);

    void checkConstantVar(const ExprPtr& var);

    void checkBool(const ExprPtr& var, TokenType ttype) const;

    void checkBitwiseOp(const ExprPtr& n, TokenType ttype);

    std::variant<int, double> getValue(const ExprPtr& num);

    ExprPtr varResolve(ExprPtr& n, TokenType ttype);

    ExprPtr nodeResolve(ExprPtr& n, TokenType ttype);

    void valueResolve(const ExprPtr& var, bool isConstant = false);

    static bool isPrimitive(const ExprPtr& var);

    void setType(VarExpr& var, const ExprPtr& value);

    ScopeTracker symbolTracker;

    struct TypeInferenceContext {
        bool isStarted{false};
        std::string entryPoint;
        std::unordered_map<std::string, ExprPtr> symbolTypeTable;
    };
    TypeInferenceContext tfCtx;
    /* File Name */
    const char* mFileName;
};

inline bool SemanticAnalyzer::isPrimitive(const ExprPtr& var) {
    return cast::toInt(var) ||
           cast::toDouble(var) ||
           cast::toNIL(var) ||
           cast::toT(var) ||
           cast::toString(var);
}

#endif //SEMANTIC_H
