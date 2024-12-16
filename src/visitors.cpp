#include "visitors.h"

void NumberEvaluator::visit(const IntExpr& num) {
    set(num.n);
}

void NumberEvaluator::visit(const DoubleExpr& num) {
    set(num.n);
}

void NumberEvaluator::visit(const VarExpr& var) {
    set(NumberEvaluator::get(var.value));
}

void NumberEvaluator::visit(BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;

    lhs = NumberEvaluator::get(binop.lhs);
    rhs = NumberEvaluator::get(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            VISIT(lhs, rhs,
                  set(var1 + var2);
            );
            break;
        case TokenType::MINUS:
            VISIT(lhs, rhs,
                  set(var1 - var2);
            );
            break;
        case TokenType::DIV:
            VISIT(lhs, rhs,
                  set(var1 / var2);
            );
            break;
        case TokenType::MUL:
            VISIT(lhs, rhs,
                  set(var1 * var2);
            );
            break;
    }
}

void StringEvaluator::visit(const StringExpr& str) {
    set(str.data);
}

void StringEvaluator::visit(const VarExpr& var) {
    set(StringEvaluator::get(var.name));
}
