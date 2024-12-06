#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "parser.h"
#include "visitors.hpp"

enum class Register {
    RAX, RBX, RCX,
    RDX, RBP, RDI,
    RSI, R8, R9, R10,
    R11, R12, R13,
    R14, R15
};

class ASTVisitor : public ValueGetter<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
public:
    void visit(const BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const PrintExpr& print) override;

    void visit(const ReadExpr&) override;

    void visit(const LetExpr& let) override;

    void visit(const SetqExpr& setq) override;

    void visit(const VarExpr& var) override;

private:
    std::string code;
    std::unordered_set<Register> registersInUse;
};

namespace CodeGen {
std::string emit(ExprPtr& ast);
}

