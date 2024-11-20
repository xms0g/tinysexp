#pragma once
#include <string>
#include "parser.h"

class CodeGen {
public:
    std::string emit(ExprPtr& ast);

private:
    int emitSExpr(ExprPtr& expr);

    void emitBinOp(ExprPtr& expr, std::string& code);

    void emitDotimes(ExprPtr& expr, std::string& code);

    void emitPrint(ExprPtr& expr, std::string& code);

};

