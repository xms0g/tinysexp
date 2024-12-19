#include "visitors.h"

void NumberEval::visit(const IntExpr& num) {
    set(num.n);
}

void NumberEval::visit(const DoubleExpr& num) {
    set(num.n);
}

void NumberEval::visit(const VarExpr& var) {
    set(NumberEval::get(var.value));
}

void NumberEval::visit(BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;

    lhs = NumberEval::get(binop.lhs);
    rhs = NumberEval::get(binop.rhs);

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

void StringEval::visit(const StringExpr& str) {
    set(str.data);
}

void StringEval::visit(const VarExpr& var) {
    set(StringEval::get(var.name));
}
