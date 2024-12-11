#include "codegen.h"
#include <format>

#define VISIT(L, OP, R) \
    std::visit([&](auto var1, auto var2) { \
        store(var1 OP var2); \
    }, L, R)

static const std::unordered_map<Register, std::string> stringRepFromReg = {
        {Register::RAX, "RAX"},
        {Register::RBX, "RBX"},
        {Register::RCX, "RCX"},
        {Register::RDX, "RDX"},
        {Register::RDI, "RDI"},
        {Register::RSI, "RSI"},
        {Register::R8,  "R8"},
        {Register::R9,  "R9"},
        {Register::R10, "R10"},
        {Register::R11, "R11"},
        {Register::R12, "R12"},
        {Register::R13, "R13"},
        {Register::R14, "R14"},
        {Register::R15, "R15"},
};

std::string CodeGen::emit(ExprPtr& ast) {
    std::string generatedCode =
            //"section .data\n" //for global variables
            "section. text\n"
            "\tglobal _start\n"
            "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n";

    while (ast != nullptr) {
        generatedCode += ASTVisitor::getResult(ast);
        ast = ast->child;
    }
    generatedCode += "\tpop rbp\n"
                     "\tret\n";
    return generatedCode;
}


void ASTVisitor::visit(const BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;
    std::string reg;

    lhs = NumberEvaluator::getResult(binop.lhs);
    rhs = NumberEvaluator::getResult(binop.rhs);

    // unsigned long long l = *((unsigned long long*)&dToUse);

    reg = (std::holds_alternative<double>(lhs) || std::holds_alternative<double>(rhs)) ? "xmm0" : "rax";

    switch (binop.opToken.type) {
        case TokenType::PLUS: {

            store(code += std::format("\tmov {}, {}\n", reg,
                                      (std::holds_alternative<int>(lhs) ? std::get<int>(lhs) : std::get<double>(lhs)) +
                                      (std::holds_alternative<int>(rhs) ? std::get<int>(rhs) : std::get<double>(rhs))
            ));
            break;
        }
        case TokenType::MINUS:
            store(code += std::format("\tmov {}, {}\n",
                                      reg,
                                      (std::holds_alternative<int>(lhs) ? std::get<int>(lhs) : std::get<double>(lhs)) -
                                      (std::holds_alternative<int>(rhs) ? std::get<int>(rhs) : std::get<double>(rhs))
            ));
            break;
        case TokenType::DIV:
            store(code += std::format("\tmov {}, {}\n",
                                      reg,
                                      (std::holds_alternative<int>(lhs) ? std::get<int>(lhs) : std::get<double>(lhs)) /
                                      (std::holds_alternative<int>(rhs) ? std::get<int>(rhs) : std::get<double>(rhs))
            ));
            break;
        case TokenType::MUL:
            store(code += std::format("\tmov {}, {}\n",
                                      reg,
                                      (std::holds_alternative<int>(lhs) ? std::get<int>(lhs) : std::get<double>(lhs)) *
                                      (std::holds_alternative<int>(rhs) ? std::get<int>(rhs) : std::get<double>(rhs))
            ));
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

void NumberEvaluator::visit(const IntExpr& num) {
    store(num.n);
}

void NumberEvaluator::visit(const DoubleExpr& num) {
    store(num.n);
}

void NumberEvaluator::visit(const VarExpr& var) {
    //store(var);
}

void NumberEvaluator::visit(const BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;

    lhs = NumberEvaluator::getResult(binop.lhs);
    rhs = NumberEvaluator::getResult(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            VISIT(lhs, +, rhs);
            break;
        case TokenType::MINUS:
            VISIT(lhs, -, rhs);
            break;
        case TokenType::DIV:
            VISIT(lhs, /, rhs);
            break;
        case TokenType::MUL:
            VISIT(lhs, *, rhs);
            break;
    }
}

void StringEvaluator::visit(const StringExpr& str) {
    store(str.data);
}

void StringEvaluator::visit(const VarExpr& var) {
    store(StringEvaluator::getResult(var.name));
}

