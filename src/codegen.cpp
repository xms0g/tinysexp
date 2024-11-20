#include "codegen.h"

std::string CodeGen::emit(ExprPtr& ast) {
    std::string code;

    if (dynamic_cast<BinOpExpr*>(ast.get())) {
        emitBinOp(ast, code);
    } else if (dynamic_cast<PrintExpr*>(ast.get())) {
        emitPrint(ast, code);
    } else if (dynamic_cast<DotimesExpr*>(ast.get())) {
        emitDotimes(ast, code);
    } else if (dynamic_cast<LetExpr*>(ast.get())) {
        emitLet(ast, code);
    }

    return code;
}

int CodeGen::emitSExpr(ExprPtr& expr) {
    int lhsi, rhsi;

    if (dynamic_cast<NumberExpr*>(expr.get())) {
        lhsi = dynamic_cast<NumberExpr*>(expr.get())->n;
        return lhsi;
    } else if (dynamic_cast<VarExpr*>(expr.get())) {

    } else if (dynamic_cast<BinOpExpr*>(expr.get())) {
        auto* binop = dynamic_cast<BinOpExpr*>(expr.get());

        lhsi = emitSExpr(binop->lhs);
        rhsi = emitSExpr(binop->rhs);

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

void CodeGen::emitBinOp(ExprPtr& expr, std::string& code) {
    int lhsi, rhsi;

    auto* binop = dynamic_cast<BinOpExpr*>(expr.get());

    lhsi = emitSExpr(binop->lhs);
    rhsi = emitSExpr(binop->rhs);

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
    }
}

void CodeGen::emitDotimes(ExprPtr& expr, std::string& code) {
    auto* dotimes = dynamic_cast<DotimesExpr*>(expr.get());

    int iterCount = emitSExpr(dotimes->iterationCount);

    for (int i = 0; i < iterCount; ++i) {
        code += "+";
    }

    code += "[";
    if (dynamic_cast<PrintExpr*>(dotimes->statement.get())) {
        code += ".";
    } else if (dynamic_cast<DotimesExpr*>(dotimes->statement.get())) {
        emitDotimes(dotimes->statement, code);
    }
    code += "-]";
}

void CodeGen::emitPrint(ExprPtr& expr, std::string& code) {
    auto* print = dynamic_cast<PrintExpr*>(expr.get());

    int sexpr = emitSExpr(print->sexpr);

    for (int i = 0; i < sexpr; ++i) {
        code += "+";
    }
    code += ".";
}

void CodeGen::emitLet(ExprPtr& expr, std::string& code) {
    auto* let = dynamic_cast<LetExpr*>(expr.get());

    for (int i = 0; i < let->variables.size(); ++i) {
        auto* var = dynamic_cast<VarExpr*>(let->variables[i].get());
        auto* value = dynamic_cast<NumberExpr*>(var->value.get());

        for (int j = 0; j < value->n; ++j) {
            code += "+";
        }

        if (let->variables.size() > 1 && i != let->variables.size() - 1)
            code += ">";
    }

    if (dynamic_cast<BinOpExpr*>(let->sexpr.get())) {
        auto* binop = dynamic_cast<BinOpExpr*>(let->sexpr.get());


        switch (binop->opToken.type) {
            case TokenType::PLUS:
                // 3 + 2
                // +++>++[<+>-]
                break;
            case TokenType::MINUS:
                // 3 - 2
                // +++>++[<->-]
                break;
            case TokenType::DIV:

                break;
            case TokenType::MUL:
                // 3 * 2
                // +++>++-[<+++>-]
                break;
        }

    } else if (dynamic_cast<PrintExpr*>(let->sexpr.get())) {

    } else if (dynamic_cast<DotimesExpr*>(let->sexpr.get())) {

    }
}
