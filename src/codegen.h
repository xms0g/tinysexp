#pragma once

#include <string>
#include "parser.h"

class CodeGen {
public:
    std::string emit(ExprPtr& ast);

private:
    void emitExpr(const ExprPtr& expr);

    void emitBinOp(const BinOpExpr& binop);

    void emitDotimes(const DotimesExpr& dotimes);

    void emitPrint(const PrintExpr& print);

    void emitRead(const ReadExpr& read);

    void emitLet(const LetExpr& let);

    void emitSetq(const SetqExpr& let);

    void putVar(const NumberExpr& num);

    std::string generatedCode;

};


