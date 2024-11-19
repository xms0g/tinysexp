#include "codegen.h"

std::string CodeGen::emit(ExprPtr& ast) {
    std::string code;
    int lhsi, rhsi;

    auto* binop = dynamic_cast<BinOpExpr*>(ast.get());

    if (dynamic_cast<NumberExpr*>(binop->lhs.get())) {
        lhsi = emit1(binop->lhs);
    }

    rhsi = emit1(binop->rhs);

    switch (binop->opToken.type) {
        case TokenType::PLUS:
            for (int i = 0; i < lhsi + rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MINUS:
            for (int i = 0; i < lhsi - rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::DIV:
            for (int i = 0; i < lhsi / rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MUL:
            for (int i = 0; i < lhsi * rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::PRINT:
            for (int i = 0; i < rhsi; ++i) {
                code += "+";
            }
            code += ".";
            break;
    }

    return code;
}

int CodeGen::emit1(ExprPtr& expr) {
    int lhsi, rhsi;

    if (dynamic_cast<NumberExpr*>(expr.get())) {
        lhsi = dynamic_cast<NumberExpr*>(expr.get())->n;
        return lhsi;
    } else {
        auto* binop = dynamic_cast<BinOpExpr*>(expr.get());

        lhsi = emit1(binop->lhs);
        rhsi = emit1(binop->rhs);

        switch (binop->opToken.type) {
            case TokenType::PLUS:
                return lhsi + rhsi;
            case TokenType::MINUS:
                return lhsi - rhsi;
            case TokenType::DIV:
                return lhsi / rhsi;
            case TokenType::MUL:
                return lhsi * rhsi;
        }
    }
    return 0;
}
