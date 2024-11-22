#pragma once

#include <string>
#include <typeindex>
#include "parser.h"
#include "visitor.hpp"

#define MAKE_VISITOR(NAME, RVALUE, METHOD_NUMBER, METHOD_BINOP, METHOD_DOTIMES, METHOD_PRINT, METHOD_LET, MAKE_VAR) \
        class NAME : public GenericVisitor<NAME, ExprPtr, RVALUE>, public ExprVisitor { \
        public:             \
            METHOD_NUMBER   \
            METHOD_BINOP    \
            METHOD_DOTIMES  \
            METHOD_PRINT    \
            METHOD_LET      \
            MAKE_VAR        \
            };

#define MAKE_MTHD_NUMBER void visit(const NumberExpr& num) override;
#define MAKE_MTHD_BINOP void visit(const BinOpExpr& binop) override;
#define MAKE_MTHD_DOTIMES void visit(const DotimesExpr& dotimes) override;
#define MAKE_MTHD_PRINT void visit(const PrintExpr& print) override;
#define MAKE_MTHD_LET void visit(const LetExpr& let) override;
#define MAKE_MTHD_VAR void visit(const VarExpr& var) override;
#define NULL

class ASTVisitor : public GenericVisitor<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
public:
    void visit(const BinOpExpr& binop) override;

    void visit(const DotimesExpr&) override;

    void visit(const PrintExpr&) override;

    void visit(const LetExpr&) override;

    void visit(const VarExpr&) override;

private:
    std::string code;
};

MAKE_VISITOR(IntVisitor, int, MAKE_MTHD_NUMBER, MAKE_MTHD_BINOP, NULL, NULL, NULL, MAKE_MTHD_VAR)

MAKE_VISITOR(TypeVisitor, size_t, NULL, NULL, MAKE_MTHD_DOTIMES, MAKE_MTHD_PRINT, NULL, NULL)


namespace CodeGen {
std::string emit(ExprPtr& ast);

}

