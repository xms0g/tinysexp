#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "parser.h"
#include "visitors.h"

enum class Register {
    RAX, RBX, RCX,
    RDX, RDI, RSI,
    R8, R9, R10,
    R11, R12, R13,
    R14, R15
};

class ASTVisitor : public Getter<ASTVisitor, ExprPtr, std::string>, public ExprVisitor {
public:
    void visit(BinOpExpr& binop) override;

    void visit(const DotimesExpr& dotimes) override;

    void visit(const LetExpr& let) override;

    void visit(const SetqExpr& setq) override;

    void visit(const DefvarExpr& defvar) override;

private:
    std::string code;
    std::unordered_set<Register> registersInUse;
};

namespace CodeGen {
std::string emit(ExprPtr& ast);

static int stackOffset{8};
static std::unordered_map<std::string, int> stackOffsets;
static std::unordered_map<std::string, std::string> sectionData;
}


MAKE_VISITOR(VarEvaluator, std::string, MAKE_MTHD_BINOP MAKE_MTHD_VAR MAKE_MTHD_STR MAKE_MTHD_INT)

MAKE_VISITOR(PrintEvaluator, std::string, MAKE_MTHD_BINOP)

#endif