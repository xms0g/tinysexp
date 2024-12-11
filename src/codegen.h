#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include "parser.h"
#include "visitor.hpp"

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

using T = std::variant<int, double>;

MAKE_VISITOR(NumberEvaluator, T, MAKE_MTHD_INT MAKE_MTHD_VAR MAKE_MTHD_BINOP MAKE_MTHD_DOUBLE)

