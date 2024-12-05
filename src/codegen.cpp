#include "codegen.h"
#include <typeindex>

static const std::unordered_map<Register, std::string> stringRepFromReg = {
        {Register::RAX, "RAX"},
        {Register::RBX, "RBX"},
        {Register::RCX, "RCX"},
        {Register::RDX, "RDX"},
        {Register::RDI, "RDI"},
        {Register::RSI, "RSI"},
        {Register::R8, "R8"},
        {Register::R9, "R9"},
        {Register::R10, "R10"},
        {Register::R11, "R11"},
        {Register::R12, "R12"},
        {Register::R13, "R13"},
        {Register::R14, "R14"},
        {Register::R15, "R15"},
};

std::string CodeGen::emit(ExprPtr& ast) {
    std::string generatedCode =
            "section .bss\n"
            "section .data\n"
            "section. text\n"
            "\tglobal _start\n"
            "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n";

    ExprPtr next = ast;
    while (next != nullptr) {
        generatedCode += ASTVisitor::getResult(next);
        next = next->child;
    }
    return generatedCode;
}


void ASTVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;
    std::string lhss, rhss;

    lhsi = IntEvaluator::getResult(binop.lhs);
    rhsi = IntEvaluator::getResult(binop.rhs);

    Register reg = getAvaliableRegister();
    registersInUse.emplace(reg);
    lhss = stringRepFromReg.at(reg);
    store(code += std::format("mov {}, {}\n", lhss, lhsi));

    rhss = getResult(binop.rhs);

    if (rhss.empty()) {
        rhsi = IntEvaluator::getResult(binop.rhs);
        reg = getAvaliableRegister();
        registersInUse.emplace(reg);
        rhss = stringRepFromReg.at(reg);
        store(code += std::format("mov {}, {}\n", rhss, rhsi));
    }

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(code += std::format("add {} {}\n", lhss, rhss));
            break;
        case TokenType::MINUS:
            store(code += std::format("sub {} {}\n", lhss, rhss));
            break;
        case TokenType::DIV:
            store(code += std::format("idiv {} {}\n", lhss, rhss));
            break;
        case TokenType::MUL:
            store(code += std::format("imul {} {}\n", lhss, rhss));
            break;
    }

}

Register ASTVisitor::getAvaliableRegister() {
    if (!registersInUse.contains(Register::RAX))
        return Register::RAX;
    else if (!registersInUse.contains(Register::RBX))
        return Register::RBX;
    else if (!registersInUse.contains(Register::RCX))
        return Register::RCX;
    else if (!registersInUse.contains(Register::RDX))
        return Register::RDX;
    else if (!registersInUse.contains(Register::RDI))
        return Register::RDI;
    else if (!registersInUse.contains(Register::RSI))
        return Register::RSI;
    else if (!registersInUse.contains(Register::R8))
        return Register::R8;
    else if (!registersInUse.contains(Register::R9))
        return Register::R9;
    else if (!registersInUse.contains(Register::R10))
        return Register::R10;
    else if (!registersInUse.contains(Register::R11))
        return Register::R11;
    else if (!registersInUse.contains(Register::R12))
        return Register::R12;
    else if (!registersInUse.contains(Register::R13))
        return Register::R13;
    else if (!registersInUse.contains(Register::R14))
        return Register::R14;
    else if (!registersInUse.contains(Register::R15))
        return Register::R15;
}

void ASTVisitor::visit(const DotimesExpr& dotimes) {
    int iterCount = IntEvaluator::getResult(dotimes.iterationCount);
    bool hasPrint{false}, hasRecursive{false}, hasSExpr{false};

    store(code += std::string(iterCount, '+'));
    store(code += "[>");

    for (auto& statement: dotimes.statements) {
        hasPrint = TypeEvaluator::getResult(statement) == typeid(PrintExpr).hash_code();
        hasRecursive = TypeEvaluator::getResult(statement) == typeid(DotimesExpr).hash_code();
        hasSExpr = TypeEvaluator::getResult(statement) == typeid(BinOpExpr).hash_code() ||
                   TypeEvaluator::getResult(statement) == typeid(NumberExpr).hash_code();

        if (hasSExpr || hasRecursive) {
            store(code += getResult(statement));
        }

        if (hasPrint) {
            store(code += PrintEvaluator::getResult(statement));
        }
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
    //store(bf += CodeGen::emitBinOp(binop, PrintEvaluator::getResult));
}

void PrintEvaluator::visit(const PrintExpr& print) {
    std::string bf;
    store(bf += PrintEvaluator::getResult(print.sexpr));
    store(bf += ".");
}

void PrintEvaluator::visit(const VarExpr& var) {
    std::string bf;
    store(bf += std::string(IntEvaluator::getResult(var.value), '+'));
}

