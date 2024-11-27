#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    std::string generatedCode;

    ExprPtr next = ast;
    while (next != nullptr) {
        generatedCode += ASTVisitor::getResult(next);
        next = next->child;
    }
    return generatedCode;
}

std::string CodeGen::emitBinOp(const BinOpExpr& binop, std::string(* func)(const ExprPtr&)) {
    uint8_t lhsi, rhsi;
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
        rhsi = rhss.length();
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

std::unordered_set<std::string> ASTVisitor::settedVariables;

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
        store(code += getResult(dotimes.statement));
    }

    if (hasPrint) {
        store(code += PrintEvaluator::getResult(dotimes.statement));
    }

    if (hasRecursive) {
        store(code += getResult(dotimes.statement));
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

void ASTVisitor::visit(const ReadExpr&) {
    store(code += ",");
}

void ASTVisitor::visit(const LetExpr& let) {
    for (const auto& sexpr: let.sexprs) {
        store(code += getResult(sexpr));
    }

    if (!let.sexprs.empty()) return;

    for (int i = 0; i < let.variables.size(); ++i) {
        settedVariables.emplace(StringEvaluator::getResult(let.variables[i]));

        int value = IntEvaluator::getResult(let.variables[i]);
        if (value > 0)
            store(code += std::string(value, '+'));

        if (let.variables.size() > 1 && i != let.variables.size() - 1)
            store(code += ">");
    }
}

void ASTVisitor::visit(const SetqExpr& setq) {
    int value = IntEvaluator::getResult(setq.var);
    if (value) {
        store(code += std::string(value, '+'));
        store(code += ">");
    } else {
        store(code += getResult(setq.var));
    }

    settedVariables.emplace(StringEvaluator::getResult(setq.var));
}

void ASTVisitor::visit(const VarExpr& var) {
    store(code += getResult(var.value));
}

void IntEvaluator::visit(const NumberExpr& num) {
    store(num.n);
}

void IntEvaluator::visit(const VarExpr& var) {
    store(IntEvaluator::getResult(var.value));
}

void IntEvaluator::visit(const ReadExpr& print) {
    store(0);
}

void StringEvaluator::visit(const StringExpr& str) {
    store(str.str);
}

void StringEvaluator::visit(const VarExpr& var) {
    store(StringEvaluator::getResult(var.name));
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
    store(bf += CodeGen::emitBinOp(binop, PrintEvaluator::getResult));
}

void PrintEvaluator::visit(const PrintExpr& print) {
    std::string bf;
    store(bf += PrintEvaluator::getResult(print.sexpr));
    store(bf += ".");
}

void PrintEvaluator::visit(const VarExpr& var) {
    std::string bf;
    if (!ASTVisitor::settedVariables.contains(StringEvaluator::getResult(var.name))) {
        store(bf += std::string(IntEvaluator::getResult(var.value), '+'));
    }
}

