#ifndef TINYSEXP_SEMANTIC_H
#define TINYSEXP_SEMANTIC_H

#include <stack>
#include <unordered_map>
#include "parser.h"

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
};

struct Symbol {
    std::string name;
    ExprPtr value;
    SymbolType sType;
};

namespace SemanticAnalyzer {
void analyze(const char* fn, ExprPtr& ast);
}

/* Scope Operations */
void scopeEnter();

void scopeExit();

void scopeBind(const std::string& name, const Symbol& symbol);

Symbol scopeLookup(const std::string& name);

Symbol scopeLookupCurrent(const std::string& name);

/* Name Resolutions */
void varResolve(ExprPtr& var);

using ScopeType = std::unordered_map<std::string, Symbol>;
static std::stack<ScopeType> symbolTable;
static const char* fileName;

#endif //TINYSEXP_SEMANTIC_H
