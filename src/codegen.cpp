#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    return ASTVisitor::getResult(ast);
}

std::string CodeGen::emitBinOp(const BinOpExpr& binop, std::string(* func)(const ExprPtr&)) {
    int lhsi, rhsi;
    std::string rhss, bf;

    lhsi = IntEvaluator::getResult(binop.lhs);
    rhss = func(binop.rhs);

    bf += std::string(lhsi, '+');
    bf += ">";
    if (rhss.empty()) {
        rhsi = IntEvaluator::getResult(binop.rhs);
        bf += std::string(rhsi, '+');
    } else {
        bf += rhss;
    }

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            bf += "[<+>-]<";
            break;
        case TokenType::MINUS:
            bf += "[<->-]<";
            break;
        case TokenType::DIV:
            bf += std::format("<[{}>>+<<]>>[-<<+>>]<<", std::string(rhsi, '-'));
            break;
        case TokenType::MUL:
            bf += std::format("-[<{}>-]<", std::string(lhsi, '+'));
            break;
    }
    return bf;
}

void ASTVisitor::visit(const BinOpExpr& binop) {
    store(code + CodeGen::emitBinOp(binop, ASTVisitor::getResult));
}

void ASTVisitor::visit(const DotimesExpr& dotimes) {
    int iterCount = IntEvaluator::getResult(dotimes.iterationCount);
    bool hasPrint{false}, hasRecursive{false}, hasSExpr{false};

    if (dotimes.statement) {
        hasPrint = TypeEvaluator::getResult(dotimes.statement) == typeid(PrintExpr).hash_code();
        hasRecursive = TypeEvaluator::getResult(dotimes.statement) == typeid(DotimesExpr).hash_code();
        hasSExpr = TypeEvaluator::getResult(dotimes.statement) == typeid(BinOpExpr).hash_code() ||
                   TypeEvaluator::getResult(dotimes.statement) == typeid(NumberExpr).hash_code();
    }

    store(code += std::string(iterCount, '+'));
    store(code += "[>");

    if (hasSExpr) {
        store(code += ASTVisitor::getResult(dotimes.statement));
    }

    if (hasPrint) {
        store(code += PrintEvaluator::getResult(dotimes.statement));
    }

    if (hasRecursive) {
        store(code += ASTVisitor::getResult(dotimes.statement));
    }

    if (hasPrint || hasSExpr)
        store(code += "<->[-]<]");
    else
        store(code += "<-]");
}

void ASTVisitor::visit(const PrintExpr& print) {
    store(code += PrintEvaluator::getResult(print.sexpr));
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

void TypeEvaluator::visit(const NumberExpr& num) {
    store(typeid(NumberExpr).hash_code());
}

void TypeEvaluator::visit(const BinOpExpr& binop) {
    store(typeid(BinOpExpr).hash_code());
}

void PrintEvaluator::visit(const NumberExpr& num) {
    std::string bf;
    store(bf += std::string(num.n, '+'));
}

void PrintEvaluator::visit(const BinOpExpr& binop) {
    std::string bf;
    store(bf + CodeGen::emitBinOp(binop, PrintEvaluator::getResult));
}

void PrintEvaluator::visit(const PrintExpr& print) {
    std::string bf;
    store(bf += PrintEvaluator::getResult(print.sexpr));
    store(bf += ".");
}

