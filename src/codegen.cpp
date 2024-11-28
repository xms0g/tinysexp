#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    ExprPtr next = ast;

    while (next != nullptr) {
        switch (next->type()) {
            case ExprType::BINOP:
                emitBinOp(next->asBinop());
                break;
            case ExprType::DOTIMES:
                emitDotimes(next->asDotimes());
                break;
            case ExprType::PRINT:
                emitPrint(next->asPrint());
                break;
            case ExprType::LET:
                emitLet(next->asLet());
                break;
            case ExprType::SETQ:
                emitSetq(next->asSetq());
                break;
        }
        next = next->child;
    }
    return generatedCode;
}

void CodeGen::emitBinOp(const BinOpExpr& binop) {
    uint8_t lhsi, rhsi;

    if (binop.lhs->type() == ExprType::INT) {
        lhsi = binop.lhs->asNum().n;
        putVar(binop.lhs->asNum());
        generatedCode += ">";
    }

    if (binop.rhs->type() == ExprType::INT) {
        rhsi = binop.rhs->asNum().n;
        putVar(binop.rhs->asNum());
    } else if (binop.rhs->type() == ExprType::VAR) {
        rhsi = binop.rhs->asVar().value->asNum().n;
        putVar(binop.rhs->asVar().value->asNum());
    } else if (binop.rhs->type() == ExprType::BINOP) {
        emitBinOp(binop.rhs->asBinop());
    }

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            generatedCode += "[<+>-]<";
            break;
        case TokenType::MINUS:
            generatedCode += "[<->-]<";
            break;
        case TokenType::DIV:
            generatedCode += std::format("<[{}>>+<<]>>[-<<+>>]<<", std::string(rhsi, '-'));
            break;
        case TokenType::MUL:
            generatedCode += std::format("-[<{}>-]<", std::string(lhsi, '+'));
            break;
    }
}

void CodeGen::emitDotimes(const DotimesExpr& dotimes) {
    bool hasPrint{false}, hasSExpr{false};

    generatedCode += std::string(dotimes.iterationCount->asNum().n, '+');
    generatedCode += "[>";

    for (auto& statement: dotimes.statements) {

        if (statement->type() == ExprType::PRINT) {
            hasPrint = true;
            emitPrint(statement->asPrint());
        } else if (statement->type() == ExprType::DOTIMES) {
            emitDotimes(statement->asDotimes());
        } else if (statement->type() == ExprType::SETQ) {
            emitSetq(statement->asSetq());
        }else if (statement->type() == ExprType::LET) {
            emitLet(statement->asLet());
        } else if (statement->type() == ExprType::BINOP || statement->type() == ExprType::INT) {
            hasSExpr = true;
            //TODO:Check this out
            emitBinOp(statement->asBinop());
        }
    }

    if (hasPrint || hasSExpr)
        generatedCode += "<->[-]<]";
    else
        generatedCode += "<-]";
}

void CodeGen::emitPrint(const PrintExpr& print) {
    if (print.sexpr->type() == ExprType::INT) {
        putVar(print.sexpr->asNum());
    } else if (print.sexpr->type() == ExprType::VAR) {
        putVar(print.sexpr->asVar().value->asNum());
    } else if (print.sexpr->type() == ExprType::BINOP) {
        emitBinOp(print.sexpr->asBinop());
    }
    generatedCode += ".";
}

void CodeGen::emitRead(const ReadExpr& read) {
    generatedCode += ",";
}

void CodeGen::emitLet(const LetExpr& let) {
    for (const auto& sexpr: let.sexprs) {
        if (sexpr->type() == ExprType::LET) {
            emitLet(sexpr->asLet());
        } else if (sexpr->type() == ExprType::SETQ) {
            emitSetq(sexpr->asSetq());
        } else if (sexpr->type() == ExprType::BINOP) {
            emitBinOp(sexpr->asBinop());
        } else if (sexpr->type() == ExprType::DOTIMES) {
            emitDotimes(sexpr->asDotimes());
        } else if (sexpr->type() == ExprType::PRINT) {
            emitPrint(sexpr->asPrint());
        }
    }

    if (!let.sexprs.empty()) return;

    for (int i = 0; i < let.variables.size(); ++i) {
        putVar(let.variables[i]->asVar().value->asNum());

        if (let.variables.size() > 1 && i != let.variables.size() - 1)
            generatedCode += ">";
    }
}

void CodeGen::emitSetq(const SetqExpr& setq) {
    if (setq.var->type() == ExprType::VAR) {
        putVar(setq.var->asVar().value->asNum());
        generatedCode += ">";
    } else if (setq.var->type() == ExprType::READ) {
        emitRead(setq.var->asRead());
    }
}

void CodeGen::putVar(const NumberExpr& num) {
    generatedCode += std::string(num.n, '+');
}

//void ASTVisitor::visit(const VarExpr& var) {
//    store(code += getResult(var.value));
//}
//
//void IntEvaluator::visit(const NumberExpr& num) {
//    store(num.n);
//}
//
//void IntEvaluator::visit(const VarExpr& var) {
//    store(IntEvaluator::getResult(var.value));
//}
//
//void IntEvaluator::visit(const ReadExpr& print) {
//    store(0);
//}
//
//void StringEvaluator::visit(const StringExpr& str) {
//    store(str.str);
//}
//
//void StringEvaluator::visit(const VarExpr& var) {
//    store(StringEvaluator::getResult(var.name));
//}
//
//void TypeEvaluator::visit(const PrintExpr&) {
//    store(typeid(PrintExpr).hash_code());
//}
//
//void TypeEvaluator::visit(const DotimesExpr&) {
//    store(typeid(DotimesExpr).hash_code());
//}
//
//void TypeEvaluator::visit(const NumberExpr& num) {
//    store(typeid(NumberExpr).hash_code());
//}
//
//void TypeEvaluator::visit(const BinOpExpr& binop) {
//    store(typeid(BinOpExpr).hash_code());
//}
//
//void PrintEvaluator::visit(const NumberExpr& num) {
//    std::string bf;
//    store(bf += std::string(num.n, '+'));
//}
//
//void PrintEvaluator::visit(const BinOpExpr& binop) {
//    std::string bf;
//    store(bf += CodeGen::emitBinOp(binop, PrintEvaluator::getResult));
//}
//
//void PrintEvaluator::visit(const PrintExpr& print) {
//    std::string bf;
//    store(bf += PrintEvaluator::getResult(print.sexpr));
//    store(bf += ".");
//}
//
//void PrintEvaluator::visit(const VarExpr& var) {
//    std::string bf;
//    store(bf += std::string(IntEvaluator::getResult(var.value), '+'));
//}
//
