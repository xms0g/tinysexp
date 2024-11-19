#pragma once
#include <string>
#include "parser.h"

class CodeGen {
public:
    std::string emit(ExprPtr& ast);

private:
    int emit1(ExprPtr& expr);
};

