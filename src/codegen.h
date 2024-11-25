#pragma once

#include <string>
#include "parser.h"
#include "visitors.hpp"

namespace CodeGen {
std::string emit(ExprPtr& ast);
std::string emitBinOp(const BinOpExpr& binop, std::string(*func)(const ExprPtr&));
}

