#pragma once

#include <unordered_set>
#include "visitor.hpp"

class ASTVisitor : public GenericVisitor<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
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
};

MAKE_VISITOR(IntEvaluator, uint8_t, MAKE_MTHD_NUMBER, MAKE_MTHD_VAR, MAKE_MTHD_READ, NULL_)

MAKE_VISITOR(StringEvaluator, std::string, MAKE_MTHD_VAR, MAKE_MTHD_STR, NULL_, NULL_)

MAKE_VISITOR(TypeEvaluator, size_t, MAKE_MTHD_NUMBER, MAKE_MTHD_DOTIMES, MAKE_MTHD_BINOP, MAKE_MTHD_PRINT)

MAKE_VISITOR(PrintEvaluator, std::string, MAKE_MTHD_NUMBER, MAKE_MTHD_BINOP, MAKE_MTHD_PRINT, MAKE_MTHD_VAR)


