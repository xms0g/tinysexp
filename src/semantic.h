#ifndef TINYSEXP_SEMANTIC_H
#define TINYSEXP_SEMANTIC_H

#include <stack>
#include <unordered_map>
#include "parser.h"

struct Symbol {
    std::string name;
    ExprPtr value;
    SymbolType sType;
};

class Analyzer : public Getter<Analyzer, ExprPtr, int>, public ExprVisitor {
public:
    void visit(const BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const LetExpr& let) override;

    void visit(const SetqExpr& setq) override;

    void visit(const DefvarExpr& defvar) override;

    void visit(const DefconstExpr&) override;

    void visit(const DefunExpr&) override;

};

namespace SemanticAnalyzer {
    void analyze(ExprPtr& ast);

    static std::stack<std::unordered_map<std::string, Symbol>> symbolTable;
};

#endif //TINYSEXP_SEMANTIC_H
