#pragma once

#include "visitor.hpp"

class ASTVisitor : public GenericVisitor<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
public:
    void visit(const BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const PrintExpr& print) override;

    void visit(const LetExpr& let) override;

    void visit(const SetqExpr& setq) override;

private:
    std::string code;
};

MAKE_VISITOR(IntEvaluator, uint8_t, MAKE_MTHD_NUMBER, NULL_, NULL_, NULL_, NULL_, NULL_, MAKE_MTHD_VAR)

MAKE_VISITOR(StringEvaluator, std::string, NULL_, MAKE_MTHD_STR, NULL_, NULL_, NULL_, NULL_, MAKE_MTHD_VAR)

MAKE_VISITOR(TypeEvaluator, size_t, MAKE_MTHD_NUMBER, NULL_, MAKE_MTHD_BINOP, NULL_, MAKE_MTHD_DOTIMES, MAKE_MTHD_PRINT, NULL_)

MAKE_VISITOR(PrintEvaluator, std::string, MAKE_MTHD_NUMBER, NULL_, MAKE_MTHD_BINOP, NULL_, NULL_, MAKE_MTHD_PRINT, MAKE_MTHD_VAR)


