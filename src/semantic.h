#ifndef TINYSEXP_SEMANTIC_H
#define TINYSEXP_SEMANTIC_H

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
    void enter();

    void exit();

    size_t level();

    void bind(const std::string& name, const Symbol& symbol);

    Symbol lookup(const std::string& name);

    Symbol lookupCurrent(const std::string& name);

private:
    using ScopeType = std::unordered_map<std::string, Symbol>;
    std::stack<ScopeType> mSymbolTable;
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const char* fn);

    void analyze(ExprPtr& ast);

private:
    /* Name Resolutions */

    void exprResolve(const ExprPtr& ast);

    void binopResolve(BinOpExpr& binop);

    void varResolve(ExprPtr& var);

    void dotimesResolve(const DotimesExpr& dotimes);

    void loopResolve(const LoopExpr& loop);

    void letResolve(const LetExpr& let);

    void setqResolve(const SetqExpr& setq);

    void defvarResolve(const DefvarExpr& defvar);

    void defconstResolve(const DefconstExpr& defconst);

    void defunResolve(const DefunExpr& defun);

    void funcCallResolve(const FuncCallExpr& funcCall);

    void ifResolve(const IfExpr& if_);

    void whenResolve(const WhenExpr& when);

    void condResolve(const CondExpr& cond);

    void checkConstantVar(const ExprPtr& var);

    ExprPtr recursiveResolve(ExprPtr& name, ExprPtr& value, SymbolType type);

    ScopeTracker stracker;
    /* File Name */
    const char* mFileName;
};

#endif //TINYSEXP_SEMANTIC_H
