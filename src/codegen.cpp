#include "codegen.h"

static const std::unordered_map<Register, std::string> stringRepFromReg = {
        {Register::RAX, "RAX"},
        {Register::RBX, "RBX"},
        {Register::RCX, "RCX"},
        {Register::RDX, "RDX"},
        {Register::RDI, "RDI"},
        {Register::RSI, "RSI"},
        {Register::R8,   "R8"},
        {Register::R9,   "R9"},
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

void ASTVisitor::visit(const DotimesExpr& dotimes) {

}

void ASTVisitor::visit(const PrintExpr& print) {

}

void ASTVisitor::visit(const ReadExpr&) {

}

void ASTVisitor::visit(const LetExpr& let) {

}

void ASTVisitor::visit(const SetqExpr& setq) {

}

void ASTVisitor::visit(const VarExpr& var) {

}

void IntEvaluator::visit(const NumberExpr& num) {
    store(num.n);
}

void IntEvaluator::visit(const VarExpr& var) {
    store(IntEvaluator::getResult(var.value));
}

void StringEvaluator::visit(const StringExpr& str) {
    store(str.data);
}

void StringEvaluator::visit(const VarExpr& var) {
    store(StringEvaluator::getResult(var.name));
}

