#pragma once

#include "visitor.hpp"

MAKE_VISITOR(IntEvaluator, int, MAKE_MTHD_NUMBER, MAKE_MTHD_VAR, NULL_, NULL_)

MAKE_VISITOR(StringEvaluator, std::string, MAKE_MTHD_VAR, MAKE_MTHD_STR, NULL_, NULL_)


