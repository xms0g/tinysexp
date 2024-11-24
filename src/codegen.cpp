#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    return ASTVisitor::getResult(ast);
}

void ASTVisitor::visit(const NumberExpr& num) {
    store(code += std::string(num.n, '+'));
}

void ASTVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;
    std::string rhss;

    lhsi = IntEvaluator::getResult(binop.lhs);
    rhss = ASTVisitor::getResult(binop.rhs);

    store(code += std::string(lhsi, '+'));
    store(code += ">");
    if (rhss.empty()) {
        rhsi = IntEvaluator::getResult(binop.rhs);
        store(code += std::string(rhsi, '+'));
    } else {
        store(code += rhss);
    }

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(code += "[<+>-]<");
            break;
        case TokenType::MINUS:
            store(code += "[<->-]<");
            break;
        case TokenType::DIV:
            store(code += std::format("<[{}>>+<<]>>[-<<+>>]<<", std::string(rhsi, '-')));
            break;
        case TokenType::MUL:
            store(code += std::format("-[<{}>-]<", std::string(lhsi, '+')));
            break;
    }
}

void ASTVisitor::visit(const DotimesExpr& dotimes) {
    int iterCount = IntEvaluator::getResult(dotimes.iterationCount);
    bool hasPrint{false}, hasRecursive{false};

    if (dotimes.statement) {
        hasPrint = TypeEvaluator::getResult(dotimes.statement) == typeid(PrintExpr).hash_code();
        hasRecursive = TypeEvaluator::getResult(dotimes.statement) == typeid(DotimesExpr).hash_code();
    }

    store(code += std::string(iterCount, '+'));
    store(code += "[>");

    if (hasPrint) {
        store(code += VarEvaluator::getResult(dotimes.statement));
    }

    if (hasRecursive) {
        store(code += ASTVisitor::getResult(dotimes.statement));
    }

    if (!hasPrint)
        store(code += "<-]");
}

void ASTVisitor::visit(const PrintExpr& print) {
    store(code += ASTVisitor::getResult(print.sexpr));
    store(code += ".");
}

void ASTVisitor::visit(const LetExpr& let) {
    if (let.sexpr) {
        store(code += ASTVisitor::getResult(let.sexpr));
        return;
    }

    for (int i = 0; i < let.variables.size(); ++i) {
        int value = IntEvaluator::getResult(let.variables[i]);
        store(code += std::string(value, '+'));

        if (let.variables.size() > 1 && i != let.variables.size() - 1)
            store(code += ">");
    }
}

void IntEvaluator::visit(const NumberExpr& num) {
    store(num.n);
}

void IntEvaluator::visit(const StringExpr& str) {
    store(-1);
}

//TODO:Check this out. Seems Redundant
void IntEvaluator::visit(const VarExpr& var) {
    store(IntEvaluator::getResult(var.value));
}

void StringEvaluator::visit(const StringExpr& str) {
    store(str.str);
}

void TypeEvaluator::visit(const PrintExpr&) {
    store(typeid(PrintExpr).hash_code());
}

void TypeEvaluator::visit(const DotimesExpr&) {
    store(typeid(DotimesExpr).hash_code());
}

void VarEvaluator::visit(const PrintExpr& print) {
    std::string bf;
    store(bf += ASTVisitor::getResult(print.sexpr));
    store(bf += ".");
    store(bf += "<->[-]<]");
}

