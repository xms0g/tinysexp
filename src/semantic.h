#ifndef TINYSEXP_SEMANTIC_H
#define TINYSEXP_SEMANTIC_H

#include <stack>
#include <unordered_map>
#include "parser.h"

enum class SymbolType {
    LOCAL,
    PARAM,
    GLOBAL
};

struct Symbol {
    std::string name;
    ExprPtr value;
    SymbolType sType;
    bool isConstant{};
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(const char* fn);

    void analyze(ExprPtr& ast);
private:
    /* Name Resolutions */

    void exprResolve(const ExprPtr& ast);

    void binopResolve(BinOpExpr& binop);

    void varResolve(ExprPtr& var);

    void dotimesResolve(const DotimesExpr& dotimes);

    void letResolve(const LetExpr& let);

    void setqResolve(const SetqExpr& setq);

    void defvarResolve(const DefvarExpr& defvar);

    void defconstResolve(const DefconstExpr& defconst);

    void defunResolve(const DefunExpr& defun);

    void funcCallResolve(const FuncCallExpr& funcCall);

    void ifResolve(const IfExpr& if_);

    void whenResolve(const WhenExpr& when);

    void condResolve(const CondExpr& cond);

    void checkUnbindingVar(const ExprPtr& var);

    void checkConstantVar(const ExprPtr& var);

    /* Scope Operations */
    void scopeEnter();

    void scopeExit();

    size_t scopeLevel();

    void scopeBind(const std::string& name, const Symbol& symbol);

    Symbol scopeLookup(const std::string& name);

    Symbol scopeLookupCurrent(const std::string& name);

    /* Symbol Table */
    using ScopeType = std::unordered_map<std::string, Symbol>;
    std::stack<ScopeType> mSymbolTable;
    /* File Name */
    const char* mFileName;
};

#endif //TINYSEXP_SEMANTIC_H
