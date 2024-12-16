#ifndef TINYSEXP_VISITORS_H
#define TINYSEXP_VISITORS_H

#include <variant>
#include <string>
#include "parser.h"

#define VISIT(L, R, OP) \
    std::visit([&](auto var1, auto var2) { \
        OP \
    }, L, R)

using IntDoubleT = std::variant<int, double>;

MAKE_VISITOR(NumberEvaluator, IntDoubleT, MAKE_MTHD_INT MAKE_MTHD_BINOP MAKE_MTHD_DOUBLE MAKE_MTHD_VAR)

MAKE_VISITOR(StringEvaluator, std::string, MAKE_MTHD_VAR MAKE_MTHD_STR)

#endif //TINYSEXP_VISITORS_H
