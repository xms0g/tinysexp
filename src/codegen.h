#pragma once

#include <string>
#include "parser.h"
#include "visitors.hpp"

namespace CodeGen {
std::string emit(ExprPtr& ast);
}

