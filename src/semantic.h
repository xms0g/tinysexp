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

class Resolver : public Getter<Resolver, ExprPtr, int>, public ExprVisitor {
public:
    void visit(BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const LetExpr& let) override;

    void visit(const SetqExpr& setq) override;

    void visit(const DefvarExpr& defvar) override;

    void visit(const DefconstExpr& defconst) override;

    void visit(const DefunExpr& defun) override;

    void visit(const FuncCallExpr& funcCall) override;

    void visit(const IfExpr& ifExpr) override;

    void visit(const WhenExpr& when) override;

    void visit(const CondExpr& cond) override;
};

namespace SemanticAnalyzer {
void analyze(const char* fn, ExprPtr& ast);
}

/* Scope Operations */
void scopeEnter();

void scopeExit();

size_t scopeLevel();

void scopeBind(const std::string& name, const Symbol& symbol);

Symbol scopeLookup(const std::string& name);

Symbol scopeLookupCurrent(const std::string& name);

/* Name Resolutions */
void varResolve(ExprPtr& var);

using ScopeType = std::unordered_map<std::string, Symbol>;
static std::stack<ScopeType> symbolTable;

static const char* fileName;

#endif //TINYSEXP_SEMANTIC_H
