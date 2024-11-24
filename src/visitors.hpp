#pragma once

#include "visitor.hpp"

class ASTVisitor : public GenericVisitor<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
public:
    void visit(const NumberExpr& num) override;

    void visit(const BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const PrintExpr& print) override;

    void visit(const LetExpr& let) override;

private:
    std::string code;
};

MAKE_VISITOR(IntVisitor, int, MAKE_MTHD_NUMBER, MAKE_MTHD_STR, NULL_, NULL_, NULL_, NULL_, MAKE_MTHD_VAR)

MAKE_VISITOR(StringVisitor, std::string, NULL_, MAKE_MTHD_STR, NULL_, NULL_, NULL_, NULL_, NULL_)

MAKE_VISITOR(TypeVisitor, size_t, NULL_, NULL_, NULL_, NULL_, MAKE_MTHD_DOTIMES, MAKE_MTHD_PRINT, NULL_)

MAKE_VISITOR(VarVisitor, std::string, NULL_, NULL_, NULL_, NULL_, NULL_, MAKE_MTHD_PRINT, NULL_)

